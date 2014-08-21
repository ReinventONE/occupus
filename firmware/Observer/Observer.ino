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
 *   Encoder, NewPing, RF24, RotaryEncoderWithButton, SimpleTimer, SPI
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

// define either SENDER_DOWNSTAIRS or SENDER_UPSTAIRS depending on which unit is being
// worked on
#define SENDER_DOWNSTAIRS

#ifdef SENDER_DOWNSTAIRS
// big sensor with rotary control for main floor bathroom
#define ROTARY_CONTROL
#define SONAR_DISTANCE    130

//// Sender #0
uint8_t pinLedBlue 		= 7,
		pinLedGreen 	= 6            ,
		pinLedRed 		= 5,

		pinIRInput 		= A1,
		pinPhotoCell	= A0,
		pinSonarTrigger = A2,
		pinSonarEcho 	= A3;

const uint8_t me 		= 0; // 0 or 1 (offset into senders[]) and pipes[]
#endif

#ifdef SENDER_UPSTAIRS
// Sender #1
#define SONAR_DISTANCE    100

uint8_t pinLedBlue 		= 7,
		pinLedGreen		= 2,
		pinLedRed 		= 3,

		pinIRInput 		= 6,
		pinPhotoCell 	= A0,
		pinSonarTrigger = 5,
		pinSonarEcho 	= 4;

const uint8_t me 		= 1; // 0 or 1 (offset into senders[]) and pipes[]
#endif

Sonar sonar(
		pinSonarTrigger,
		pinSonarEcho,
		SONAR_DISTANCE * 2,// Maximum distance we want to ping for (in centimeters).
			  // Maximum sensor distance is rated at 400-500cm.
		SONAR_DISTANCE);   // Recognize objects within this many centimeters

MotionSensor motion(pinIRInput, 5000);
#ifdef SENDER_DOWNSTAIRS
LightSensor light(pinPhotoCell, 250, true);
#elif defined(SENDER_UPSTAIRS)
LightSensor light(pinPhotoCell, 300, false);
#endif

RF24 radio(9, 10);
SimpleTimer timer(1);

#ifdef ROTARY_CONTROL
// pinA, pinB, pinButton
RotaryEncoderWithButton rotary(2,3,4);
SimpleTimer  adjustmentTimer(2);
#endif

typedef struct OccupancyStruct {
	bool occupied;
	unsigned long occupiedAt;
	unsigned long vacatedAt;
	unsigned long wasOccupiedAt;
	bool lightDetected;
	bool motionDetected;
	bool sonarDetected;
} occupancyStatus;

unsigned int occupancyGracePeriod = 10000; // keep status as "occupied" after it goes off to avoid flickering
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

void resetOccupancy() {
	memset(&occupancy, 0x0, sizeof(occupancy));
}

void setOccupancyGracePeriod(unsigned int period) {
	occupancyGracePeriod = constrain(period, 1000, 60000);
}
//____________________________________________________________________________
//
// Timers

void showStatus(int timerId) {
	if (mySender.connected) {
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
		printf("error sending data over RF24\n");
		// try sending again
		delay(20);
		if (radio.write(&data, sizeof(data))) {
			mySender.connected = true;
			printf("RF24 connection has been restored\n");
		}
	} else {
		mySender.connected = true;
	}
}

void detectLight(int timerId) {
	occupancy.lightDetected = light.lightsOn();
}

void detectMotion(int timerId) {
	occupancy.motionDetected = motion.detectedNonRetriggering();
}

void detectSonar(int timerId) {
	occupancy.sonarDetected = sonar.detected();
}

void analyzeOccupancy(int timerId) {
	printf("INFO: status: Lights On?: %s, Motion?: %s, Sonar?: %s, Light Reading: %d\n",
			(occupancy.lightDetected ? "YES" : "NO"),
			(occupancy.motionDetected ? "YES" : "NO"),
			(occupancy.sonarDetected ? "YES" : "NO"),
			analogRead(pinPhotoCell                                                                                                                                                                                                                   ));
	bool isSomeoneThere = occupancy.lightDetected
			&& (occupancy.motionDetected || occupancy.sonarDetected);
	unsigned long now = millis();
	if (isSomeoneThere) {
		if (!occupancy.occupied) {
			printf(
					"INFO: detected new occupancy at %d, Lights On?: %s, Motion?: %s, Sonar?: %s\n",
					(int) (now / 1000),
					(occupancy.lightDetected ? "YES" : "NO"),
					(occupancy.motionDetected ? "YES" : "NO"),
					(occupancy.sonarDetected ? "YES" : "NO"));
			occupancy.occupiedAt = now;
			occupancy.vacatedAt = 0;
		}
		occupancy.occupied = true;
		occupancy.wasOccupiedAt = now;
	} else {
		if (occupancy.occupied
				&& ((now - occupancy.wasOccupiedAt) > occupancyGracePeriod)) {
			printf("INFO: end of occupancy at %d, duration: %d seconds\n",
					(int) (now / 1000),
					(int) ((now - occupancy.occupiedAt) / 1000));
			resetOccupancy();
			occupancy.vacatedAt = now - occupancyGracePeriod;
		}
	}
}

#ifdef ROTARY_CONTROL

typedef enum {
	NORMAL = 0,
	SONAR,
	MOTION,
	LIGHT,
	DELAY } modeType;

uint8_t adjustmentLedModes[][3] = {
	{0,0,0},
	{pinLedRed, 	0,0},
	{pinLedGreen, 	0,0},
	{pinLedBlue, 	0,0},
	{pinLedRed, pinLedGreen,	0}
};

modeType mode = NORMAL;
bool modeLedOn = false;

void modeLightsOn(int timerId) {
	modeLedOn = !modeLedOn;

	digitalWrite(pinLedRed, LOW);
	digitalWrite(pinLedBlue, LOW);
	digitalWrite(pinLedGreen, LOW);

	if (!modeLedOn) return;

	uint8_t *leds;
	leds = adjustmentLedModes[mode];
	for (int i = 0; leds[i] != 0; i++) {
		digitalWrite(leds[i], HIGH);
	}
}

void nextMode() {
	mode = (mode == DELAY) ? NORMAL : (modeType) ((int)mode + 1);
}

//–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

void adjustParameters() {
	nextMode();
	int timerId = adjustmentTimer.setInterval(250, &modeLightsOn);
	printf("Entering Parameter Adjustment MENU, timer created %d\n", timerId);
	while (mode != NORMAL) {
		int delta = rotary.rotaryDelta();
		if (delta != 0) {
			printf("adjusting by %d ", delta);
			switch (mode) {
			case SONAR:
				sonar.setDistance(sonar.getDistance() + delta);
				printf("SONAR distance to %d\n", sonar.getDistance());
				break;
			case MOTION:
				motion.setPause(motion.getPause() + delta * 10);
				printf("Adjusting MOTION pause to %d\n", motion.getPause());
				break;
			case LIGHT:
				light.setThreshold(light.getThreshold() + delta);
				printf("Adjusting LIGHT threshold to %d\n", light.getThreshold());
				break;
			case DELAY:
				setOccupancyGracePeriod(occupancyGracePeriod + delta*10);
				printf("Adjusting Occupancy Grace Period to %d\n", occupancyGracePeriod);
				break;
			default:
				printf("Unknown mode\n");
				mode = NORMAL;
			}
		}

		if (rotary.buttonClicked()) {
			nextMode();
		}

		adjustmentTimer.run();
	}
	adjustmentTimer.deleteTimer(timerId);
}

#endif


//–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

void setup(void) {
	Serial.begin(9600);

#ifdef ROTARY_CONTROL
	rotary.begin();
#endif

	printf_begin();

	printf("\nBathroom Occupancy Notification Module: Transmit #%d!\n", me);

	radio.begin();
	radio.setRetries(15, 15);
	radio.setPayloadSize(8);

	printf("opening for writing pipe: #%d => [%X], ", me, (unsigned int) mySender.pipe);
	radio.openWritingPipe(mySender.pipe);
	radio.printDetails();

	pinMode(pinLedBlue, OUTPUT);
	pinMode(pinLedGreen, OUTPUT);
	pinMode(pinLedRed, OUTPUT);

	motion.init();

	resetOccupancy();

	timer.setInterval(990,  &showStatus);
	timer.setInterval(2000, &sendStatus);

	timer.setInterval(110, 	&detectLight);
	timer.setInterval(220,  &detectMotion);
	timer.setInterval(330,  &detectSonar);

	timer.setInterval(1000, &analyzeOccupancy);
}


void loop(void) {
	timer.run();

#ifdef ROTARY_CONTROL
	if (rotary.buttonClicked() && mode == NORMAL) {
		adjustParameters();
	}
#endif
}
