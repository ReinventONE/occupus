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
 *
 * Arduino Libs:
 *   SoftwareSerial             (output state and options to Serial LCD)
 *   SPI                        (communications between components)
 *
 * 3rd Party Libs:
 *   EEPROMEx					(saving configuration to non-volatile RAM)
 *   Encoder                    (using rotary encoder for config)
 *   NewPing                    (HC-SR04)
 *   RF24                       (Wireless RF24 Radio)
 *   SimpleTimer                (for timers and callbacks)
 *
 * Kiguino Libs                 (https://github.com/kigster/Kiguino/)
 *   RotaryEncoderWithButton    (high level rotary encoder abstraction)
 *   Sonar
 *   LightSensor
 *   MotionSensor
 *   SparkfunSerialLCD
 */


#define ERR_COUNT_FOR_RADIO_RESET 50

#ifdef SENDER_DOWNSTAIRS

#define ENABLE_SERIAL_LCD
#define ENABLE_ROTARY_KNOB
#define ENABLE_RADIO

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

const uint8_t me 		= 0; // offset into senders[] array
#endif

#ifdef SENDER_UPSTAIRS

#define ENABLE_SERIAL_LCD
#define ENABLE_ROTARY_KNOB
#define ENABLE_RADIO

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

const uint8_t me 		= 1; // offset into senders[] array
#endif

#ifdef SENDER_THIRDBOX

#define ENABLE_SERIAL_LCD
#define ENABLE_ROTARY_KNOB
// #define ENABLE_RADIO

uint8_t
		pinPhotoCell	= A0,
		pinIRInput 		= A1,
		pinSonarTrigger = A2,
		pinSonarEcho 	= A3,

		pinSerialLcdRX	= 8,

		pinLedBlue 		= 6,
		pinLedGreen 	= 5,
		pinLedRed 		= 7,

		pinRotaryButton = 4,
		pinRotaryLeft   = 3,
		pinRotaryRight  = 2
		;

const uint8_t me 		= 2; // offset into senders[] array
#endif

#include <SPI.h>
#ifdef ENABLE_RADIO
#include "nRF24L01.h"
#include "RF24.h"
#endif

#ifdef ENABLE_ROTARY_KNOB
#include <RotaryEncoderWithButton.h>
#endif

#ifdef EANBLE_SERIAL_LCD
#include <SoftwareSerial.h>
#include <SparkfunSerialLCD.h>
#endif
#include <SimpleTimer.h>

#include "printf.h"

#include "Sonar.h"
#include "MotionSensor.h"
#include "LightSensor.h"
#include "Configuration.h"
#include "Modular.h"

// default values if EEPROM does have anything
configType cfg = {
	 200, // light (0 - 1023)
	5000, // (ms) motion pause (once detected, ignore during next 5s), max 30,000ms.
	 100, // (cm) sonar distance to recognized objects within, up to 500
	  15  // (s)  activity timeout, if activity no longer detected but light is on
	      //      consider room occupied for this many seconds still. Helps if there is a spot
	      //      that is not accessible in the bathroom, so observer does not see the person.
};

#ifdef ENABLE_ROTARY_KNOB
// pinA, pinB, pinButton
RotaryEncoderWithButton rotary(pinRotaryLeft, pinRotaryRight, pinRotaryButton);
Configuration configuration(&cfg, &rotary);
#else
Configuration configuration(&cfg, NULL);
#endif

#ifdef ENABLE_SERIAL_LCD
SparkfunSerialLCD debugLCD(pinSerialLcdRX);
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
int sendErrorCnt = 0;

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
		{ false, 0xF0F0F0F0D2LL, 0x020 },
		{ false, 0xF0F0F0F0F2LL, 0x030 },
		{ false, 0xF0F0F0F0A2LL, 0x040 },
		{ false, 0xF0F0F0F0C2LL, 0x050 }
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

#ifdef ENABLE_ROTARY_KNOB
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
	#ifdef ENABLE_RADIO
		unsigned long data = occupancy.occupied | mySender.senderId;
		bool ok = radio.write(&data, sizeof(data));
		if (!ok) {
			mySender.connected = false;
			sendErrorCnt ++;
			Serial.println(F("Error sending data over RF24"));
			if (sendErrorCnt % ERR_COUNT_FOR_RADIO_RESET == 5) {
				#ifdef ENABLE_SERIAL_LCD
					sprintf(buffer, "%d send errors", sendErrorCnt);
					debugLCD.print(buffer, "resetting radio");
				#endif
				delay(1000);
				radioReset();
			}
		} else {
			sendErrorCnt = 0;
			mySender.connected = true;
		}
	#endif
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

#ifdef ENABLE_SERIAL_LCD
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

void radioStart() {
	#ifdef ENABLE_RADIO
		#ifdef ENABLE_SERIAL_LCD
			debugLCD.print("Initializing", "radio...");
		#endif
		radio.begin();
		radio.setRetries(15, 15);
		radio.setPayloadSize(8);

		Serial.print(F("creating a pipe "));
		printf("x#%d => [%X], ", me, (unsigned int) mySender.pipe);
		radio.openWritingPipe(mySender.pipe);
		radio.printDetails();
	#endif
}

void radioReset() {
	#ifdef ENABLE_RADIO
		#ifdef ENABLE_SERIAL_LCD
			debugLCD.print("Radio reset", "Powering down");
		#endif

		radio.powerDown();
		delay(100);
		radio.powerUp();

		#ifdef ENABLE_SERIAL_LCD
		debugLCD.print("Powering up...");
		#endif
		radioStart();
	#endif
}

void setup(void) {
	Serial.begin(9600);

	#ifdef ENABLE_ROTARY_KNOB
		rotary.begin();
	#endif

	printf_begin();

	Serial.println();
	Serial.print(F("Bathroom Occupancy Notification Module: Transmit "));
	printf("#%d!\n", me);

	#ifdef ENABLE_RADIO
		radioStart();
	#endif

	pinMode(pinLedBlue, OUTPUT);
	pinMode(pinLedGreen, OUTPUT);
	pinMode(pinSerialLcdRX, OUTPUT);
	pinMode(pinLedRed, OUTPUT);

	#ifdef ENABLE_SERIAL_LCD
		debugLCD.init();
		delay(200); // wait for display to boot up
		debugLCD.print("PooCast v1.0", "Booting...");
		delay(1000);
		debugLCD.print("Calibrating", "motion sensor...");
	#endif

	motion.init(5000);

	#ifdef ENABLE_ROTARY_KNOB
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
	timer.setInterval(50,   &detectSonar);

	timer.setInterval(502,  &analyzeOccupancy);
}


void loop(void) {
	timer.run();
	#ifdef ENABLE_ROTARY_KNOB
		if (configuration.configure(&showConfigStatus)) {
			debugLCD.print("Saving Config","to the EEPROM.");
			delay(1000);
			applyConfig();
		}
	#endif
	delay(1);
}



