/*
 * Observer.ino
 *
 * Created on: Aug 9, 2014
 *      Author: Konstantin Gredeskoul
 *
 * (c) 2014 All rights reserved.  Please see LICENSE.
 *
 * Connections for RF24:
 *
 *      GND  GND
 *      3V3  3V3
 *      CE     9
 *      CSN   10
 *      SCK   13
 *      MOSI  11
 *      MISO  12
 *
 * Dependencies:
 *   EEPROMEx					(saving configuration to non-volatile RAM)
 *   Encoder                    (using rotary encoder for config)
 *   NewPing                    (Sonar)
 *   RF24                       (Wireless RF24 Radio)
 *   RotaryEncoderWithButton    (high level rotary encoder abstraction)
 *   SimpleTimer                (use timers and callbacks)
 *   SoftwareSerial             (output state and options to Serial LCD)
 *   SPI                        (communications between components)
 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

#include "Sonar.h"
#include "MotionSensor.h"
#include "LightSensor.h"

#include <RotaryEncoderWithButton.h>
#include <SimpleTimer.h>
#include <SoftwareSerial.h>
#include <SerialLCD.h>
#include "Configuration.h"
#include "Modular.h"


#ifdef SENDER_DOWNSTAIRS
//// Sender #0
uint8_t pinSerialLcdRX	= 8,
		pinLedBlue 		= 7,
		pinLedGreen 	= 6,
		pinLedRed 		= 5,

		pinPhotoCell	= A0,
		pinIRInput 		= A1,
		pinSonarTrigger = A2,
		pinSonarEcho 	= A3,

		pinRotaryLeft   = 2,
		pinRotaryRight  = 3,
		pinRotaryButton = 4
		;

const uint8_t me 		= 0; // 0 or 1 (offset into senders[]) and pipes[]
#endif

#ifdef SENDER_UPSTAIRS
//// Sender #1
uint8_t pinSerialLcdRX	= A1,
		pinLedBlue 		= 7,
		pinLedGreen 	= A4,
		pinLedRed 		= A2,

		pinPhotoCell	= A0,
		pinIRInput 		= 6,
		pinSonarTrigger = 5,
		pinSonarEcho 	= 4,

		pinRotaryLeft   = 3,
		pinRotaryRight  = 2,
		pinRotaryButton = A3
		;

const uint8_t me 		= 1; // 0 or 1 (offset into senders[]) and pipes[]
#endif

// default values if EEPROM does have anything
configType cfg = {
	 200, // light (0 - 1023)
	5000, // (ms) motion pause (once detected, ignore during next 5s), max 30,000ms.
	 100, // (cm) sonar distance to recognized objects within, up to 500
	  15  // (s)  activity timeout, if activity no longer detected but light is on
	      //      consider room occupied for this many seconds still. Helps if there is a spot
	      //      that is not accessible in the bathroom, so observer does not see the person.
};

#ifdef HAVE_ROTARY_KNOB
// pinA, pinB, pinButton
RotaryEncoderWithButton rotary(pinRotaryLeft, pinRotaryRight, pinRotaryButton);
Configuration configuration(&cfg, &rotary);
#else
Configuration configuration(&cfg, NULL);
#endif

#ifdef SERIAL_LCD
SerialLCD debugLCD(pinSerialLcdRX);
#endif

Sonar sonar(
		pinSonarTrigger,
		pinSonarEcho,
		500,				   // Maximum distance we want to ping for (in centimeters).
			  	  	  	  	   // Maximum sensor distance is rated at 400-500cm.
		cfg.sonarThreshold);   // Recognize objects within this many centimeters

MotionSensor motion(pinIRInput, cfg.motionTolerance);
LightSensor light(pinPhotoCell, cfg.lightThreshold);

RF24 radio(9, 10);
SimpleTimer timer(1);

char buffer[90];
bool flagStatusLCD = false;

typedef struct OccupancyStruct {
	bool occupied;
	unsigned long occupiedAt;
	unsigned long vacatedAt;
	unsigned long wasOccupiedAt;
	bool lightOn;
	bool motionDetected;
	bool sonarDetected;
	unsigned long sonarDistanceDetected;
} occupancyStatus;

occupancyStatus occupancy;

typedef struct senderInfoStruct {
	bool connected;
	uint64_t pipe;
	uint8_t senderId;
} senderInfo;

senderInfo senders[] = {
		{ false, 0xF0F0F0F0E1LL, 0x010 },
		{ false, 0xF0F0F0F0D2LL, 0x020 }
};

senderInfo mySender = senders[me];

// combinations of LEDs to turn on for each mode
uint8_t configLedModes[][3] = {
	{0,			 	0,				0}, // normal
	{pinLedRed, 	0,				0},
	{pinLedGreen, 	0,				0},
	{pinLedBlue, 	0,				0},
	{pinLedRed, 	pinLedGreen,	0}
};

void resetOccupancy() {
	memset(&occupancy, 0x0, sizeof(occupancy));
}

// Push configuration from the local config class to each individual module.
void applyConfig() {
	sonar.setDistanceThreshold(cfg.sonarThreshold);
	motion.setPause(cfg.motionTolerance);
	light.setThreshold(cfg.lightThreshold);
}

#ifdef HAVE_ROTARY_KNOB
void showConfigStatus() {
	digitalWrite(pinLedRed, LOW);
	digitalWrite(pinLedGreen, LOW);

	if (configuration.mode != NORMAL) {
		digitalWrite(pinLedBlue, LOW);

		uint8_t *leds;
		leds = configLedModes[configuration.mode];
		for (int i = 0; leds[i] != 0; i++) {
			digitalWrite(leds[i], HIGH);
		}
	} else {
		return;
	}

	// print status of our configuration
	switch (configuration.mode) {
	case SONAR:
		sprintf(buffer, "CFG: Sonar Thres" "Dist (cm) :%4d ", cfg.sonarThreshold);
		break;
	case MOTION:
		sprintf(buffer, "CFG: Motion Sens" "Pause(ms) :%4d ", (unsigned int) cfg.motionTolerance);
		break;
	case LIGHT:
		sprintf(buffer, "CFG: Light Sens " "Threshold :%4d ", cfg.lightThreshold);
		break;
	case GRACE:
		sprintf(buffer, "CFG: ExitTimeout" "(Seconds) :%4d ", (unsigned int) cfg.occupancyGracePeriod);
		break;
	default:
		sprintf(buffer,"                 " "             ");
		configuration.mode = NORMAL;
	}
	debugLCD.print(buffer);
	short offset = 16;
	char f = buffer[offset]; buffer[offset] = '\0';
	Serial.print(buffer);
	Serial.print(" ");
	buffer[offset] = f;
	Serial.println(buffer + offset);
}
#endif

//____________________________________________________________________________
//
// Timers
void showStatus(int timerId) {
	if (mySender.connected) {
		digitalWrite(pinLedRed, LOW);
		digitalWrite(pinLedBlue, occupancy.occupied ? HIGH : LOW);
		digitalWrite(pinLedGreen, occupancy.occupied ? LOW : HIGH);
	} else {
		digitalWrite(pinLedBlue, LOW);
		digitalWrite(pinLedGreen, LOW);
		digitalWrite(pinLedRed, HIGH);
		delay(200);
		digitalWrite(pinLedRed, LOW);
	}
}

void sendStatus(int timerId) {
	unsigned long data = occupancy.occupied | mySender.senderId;
	bool ok = radio.write(&data, sizeof(data));
	if (!ok) {
		mySender.connected = false;
		Serial.println(F("Error sending data over RF24"));
	} else {
		mySender.connected = true;
	}
}

void detectLight(int timerId) {
	occupancy.lightOn = light.lightsOn();
}

void detectMotion(int timerId) {
	occupancy.motionDetected = motion.detectedNonRetriggering();
}

void detectSonar(int timerId) {
	occupancy.sonarDetected = sonar.detected();
	if (occupancy.sonarDetected) {
		occupancy.sonarDistanceDetected = sonar.getDistance();
	}
}

void logCurrentState() {
	printf("Occupied? %s | Lights? %s (%4d/%4d) | Motion? %s | Sonar? %s (%4d/%4d)\n",
			(occupancy.occupied  	  ? "YES" : " NO"),
			(occupancy.lightOn  ? "YES" : " NO"),
				light.getLightReading(),
				light.getThreshold(),
			(occupancy.motionDetected ? "YES" : " NO"),
			(occupancy.sonarDetected  ? "YES" : " NO"),
				sonar.getDistance(),
				sonar.getDistanceThreshold()
  			);

#ifdef SERIAL_LCD
	if (mySender.connected || flagStatusLCD) {
		sprintf(buffer, "%s %s(%3d:%3d)",
				(occupancy.occupied  	  ? "Occp": "Vcnt"),
				(occupancy.lightOn  	  ? "+L"  : "-L"), light.getLightReading(), light.getThreshold());
		debugLCD.print(buffer);
		sprintf(buffer, "  %s %s(%3d:%3d)",
				(occupancy.motionDetected ? "+M"  : "-M"),
				(occupancy.sonarDetected  ? "+D"  : "-D"), sonar.getDistance(), sonar.getDistanceThreshold()
				);
		debugLCD.printAt(1, 2, buffer);
	} else {
		debugLCD.print("Error connecting", "to display unit");
	}
	flagStatusLCD = !flagStatusLCD;

#endif
}

void analyzeOccupancy(int timerId) {
	// all the logic follows
	bool someoneMaybeThere = occupancy.motionDetected || occupancy.sonarDetected;
	unsigned long now = millis();
	if (someoneMaybeThere && occupancy.lightOn) {
		if (!occupancy.occupied) {
			occupancy.occupiedAt = now;
			occupancy.vacatedAt = 0;
		}
		occupancy.occupied = true;
		occupancy.wasOccupiedAt = now;
	} else {
		if (occupancy.occupied	&&
				(((now - occupancy.wasOccupiedAt) > cfg.occupancyGracePeriod * 1000) ||
				   !occupancy.lightOn)) {
			printf("%d seconds inside\n",  (int) ((now - occupancy.occupiedAt) / 1000));
			resetOccupancy();
			occupancy.vacatedAt = now - cfg.occupancyGracePeriod * 1000;
		}
	}

	logCurrentState();
}
//–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

void setup(void) {
	Serial.begin(9600);

#ifdef  HAVE_ROTARY_KNOB
	rotary.begin();
#endif

	printf_begin();

	Serial.println();
	Serial.print(F("Bathroom Occupancy Notification Module: Transmit "));
	printf("#%d!\n", me);

	radio.begin();
	radio.setRetries(15, 15);
	radio.setPayloadSize(8);

	Serial.print(F("creating a pipe "));
	printf("x#%d => [%X], ", me, (unsigned int) mySender.pipe);
	radio.openWritingPipe(mySender.pipe);
	radio.printDetails();

	pinMode(pinLedBlue, OUTPUT);
	pinMode(pinLedGreen, OUTPUT);
	pinMode(pinSerialLcdRX, OUTPUT);
	pinMode(pinLedRed, OUTPUT);

#ifdef SERIAL_LCD
	debugLCD.init();
    delay(200); // wait for display to boot up
    debugLCD.print("PooCast v1.0", "Booting...");
    delay(1000);
    debugLCD.print("Calibrating", "motion sensor...");
#endif

	motion.init(5000);

#ifdef HAVE_ROTARY_KNOB
	debugLCD.print("Reading config ", "from EEPROM...");
	delay(1000);
#endif
	configuration.init();
	applyConfig();

	resetOccupancy();
	memset(buffer, 0x0, sizeof(buffer));

	timer.setInterval(501,  &showStatus);
	timer.setInterval(500,  &sendStatus);

	timer.setInterval(110, 	&detectLight);
	timer.setInterval(220,  &detectMotion);
	timer.setInterval(330,  &detectSonar);

	timer.setInterval(502,  &analyzeOccupancy);
}


void loop(void) {
	timer.run();
#ifdef HAVE_ROTARY_KNOB
	if (configuration.configure(&showConfigStatus)) {
		debugLCD.print("Saving config","to the EEPROM.");
		delay(1000);
		applyConfig();
	}
#endif
	delay(1);
}



