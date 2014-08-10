/*
 * Connections for RF24:
 *
	 GND		GND
	 3V3		3V3
	 CE			9
	 CSN		10
	 SCK		13
	 MOSI		11
	 MISO		12

 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

#include "Sonar.h"
#include "MotionSensor.h"
#include <RotaryEncoderWithButton.h>
#include <SimpleTimer.h>

#define SENDER_0

#ifdef SENDER_0
// pinA, pinB, pinButton
RotaryEncoderWithButton rotary(2,3,4);

//// Sender #0
uint8_t pinStatusLed = 6,
		pinMotionLed = 7,
		pinSonarLed = 5,

		pinIRInput = A1,
		pinPhotoCell = A0,
		pinSonarTrigger = A2,
		pinSonarEcho = A3;

const uint8_t me 		= 0; // 0 or 1 (offset into senders[]) and pipes[]
#endif

#ifdef SENDER_1

// Sender #1
uint8_t pinStatusLed = 2,
		pinMotionLed = 3,
		pinSonarLed = 7,

		pinIRInput = 6,
		pinPhotoCell = A0,
		pinSonarTrigger = 5,
		pinSonarEcho = 4;

const uint8_t me 		= 1; // 0 or 1 (offset into senders[]) and pipes[]

#endif

Sonar sonar(
		pinSonarTrigger,
		pinSonarEcho,
		200,  // Maximum distance we want to ping for (in centimeters).
			  // Maximum sensor distance is rated at 400-500cm.
		90); // Recognize objects within this many centimeters

MotionSensor motion(pinIRInput, 5000);

RF24 radio(9, 10);
SimpleTimer timer(1);

typedef struct OccupancyStruct {
	bool occupied;
	unsigned long occupiedAt;
	unsigned long vacatedAt;
	unsigned long wasOccupiedAt;
	bool lightDetected;
	bool motionDetected;
	bool sonarDetected;
} occupancyStatus;

unsigned int occupancyGracePeriod = 5000; // keep status as "occupied" after it goes off to avoid flickering
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

//____________________________________________________________________________
//
// Timers

void showStatus(int timerId) {
//	digitalWrite(pinStatusLed, occupancy.occupied ? HIGH : LOW);
	digitalWrite(pinMotionLed, occupancy.motionDetected ? HIGH : LOW);
	digitalWrite(pinSonarLed, occupancy.sonarDetected ? HIGH : LOW);
}

void sendStatus(int timerId) {
	unsigned long data = occupancy.occupied | mySender.senderId;
	bool ok = radio.write(&data, sizeof(data));
	if (!ok)
		printf("error sending data over RF24\n");
}

void detectLight(int timerId) {
	int photoCellReading = analogRead(pinPhotoCell);
	occupancy.lightDetected = (photoCellReading > 250);
}


void detectMotion(int timerId) {
	occupancy.motionDetected = motion.detectedNonRetriggering();
}

void detectSonar(int timerId) {
	occupancy.sonarDetected = sonar.detected();
}

void analyzeOccupancy(int timerId) {
	printf("INFO: status: Lights On?: %s, Motion?: %s, Sonar?: %s\n",
			(occupancy.lightDetected ? "YES" : "NO"),
			(occupancy.motionDetected ? "YES" : "NO"),
			(occupancy.sonarDetected ? "YES" : "NO"));
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

//–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

void setup(void) {
	Serial.begin(57600);

	printf_begin();
	printf("\nBathroom Occupancy Notification Module: Transmit #%d!\n", me);

	radio.begin();
	radio.setRetries(15, 15);
	radio.setPayloadSize(8);
	radio.openWritingPipe(mySender.pipe);
	radio.printDetails();

	pinMode(pinStatusLed, OUTPUT);
	pinMode(pinMotionLed, OUTPUT);
	pinMode(pinSonarLed, OUTPUT);

	motion.init();

	resetOccupancy();

	timer.setInterval(990,  &showStatus);
	timer.setInterval(2000,  &sendStatus);

	timer.setInterval(110, 	&detectLight);
	timer.setInterval(220,  &detectMotion);
	timer.setInterval(330,  &detectSonar);

	timer.setInterval(1000, &analyzeOccupancy);
}


void loop(void) {
	timer.run();
}
