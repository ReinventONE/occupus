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

uint8_t pinStatusLed = 2,
		pinPIRLed = 3,
		pinSonarLed = 7,
		pinIRInput = 6,
		pinPhotoCell = A0,
		pinSonarTrigger = 5,
		pinSonarEcho = 4;


Sonar sonar(
		pinSonarTrigger,
		pinSonarEcho,
		300,  // Maximum distance we want to ping for (in centimeters).
			  // Maximum sensor distance is rated at 400-500cm.
		150); // Recognize objects within this many centimeters

MotionSensor motion(pinIRInput, 5000);

RF24 radio(9, 10);

typedef struct OccupancyStruct {
	bool occupied;
	unsigned long occupiedAt;
	unsigned long vacatedAt;
	unsigned long wasOccupiedAt;
} occupancyStatus;

unsigned int occupancyGracePeriod = 5000; // keep status as "occupied" after it goes off to avoid flickering
occupancyStatus occupancy;

const uint8_t states[] 	= { 0x000, 0x001 }; // VACANT, OCCUPIED, ...

// Radio pipe addresses for the 2 nodes to communicate.
// 							client1         client2
const uint64_t pipes[] 	= { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
const uint8_t senders[] = { 0x010, 			0x020 }; // first sender, second, etc.
const uint8_t me 		= 1; // 0 or 1 (offset into senders[]) and pipes[]
const unsigned int sendPeriod = 500;
unsigned long lastSent 	= 0;


void showStatus() {
	digitalWrite(pinStatusLed, occupancy.occupied ? HIGH : LOW);
}

void sendStatus() {
	if (millis() - lastSent > sendPeriod) {
		lastSent = millis();

		unsigned long data = occupancy.occupied | senders[me];
//		printf("transmitting [%s], me=[%d], data=[%d]\n",
//				occupancy.occupied ? "OCCIPIED" : "VACANT", me, (int) data);

		bool ok = radio.write(&data, sizeof(data));
		if (!ok)
			printf("error sending data over RF24\n");
	}
}

bool isRoomDark() {
	int photoCellReading = analogRead(pinPhotoCell);
	return (photoCellReading < 250);
}

void resetOccupancy() {
	memset(&occupancy, 0x0, sizeof(occupancy));
}

//–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

void setup(void) {
	Serial.begin(57600);

	printf_begin();
	printf("\nBathroom Occupancy Notification Module: Transmit #%d!\n", me);

	radio.begin();
	radio.setRetries(15, 15);
	radio.setPayloadSize(8);
	radio.openWritingPipe(pipes[me]);
	radio.printDetails();

	pinMode(pinStatusLed, OUTPUT);
	pinMode(pinPIRLed, OUTPUT);
	pinMode(pinSonarLed, OUTPUT);

	motion.init();

	resetOccupancy();
}

void loop(void) {
	bool roomDark = isRoomDark();
	bool motionDetected = motion.detectedNonRetriggering();
	bool sonarDetected = sonar.detected();

	printf("INFO: status: Lights On?: %s, Motion?: %s, Sonar?: %s\n",
			(roomDark ? "NO" : "YES"),
			(motionDetected ? "YES" : "NO"),
			(sonarDetected ? "YES" : "NO")
			);

	bool isSomeoneThere = !roomDark && (motionDetected || sonarDetected);
	unsigned long now = millis();
	if (isSomeoneThere) {
		if (!occupancy.occupied) {
			printf("INFO: detected new occupancy at %d, Lights On?: %s, Motion?: %s, Sonar?: %s\n",
					(int) (now / 1000),
					(roomDark ? "NO" : "YES"),
					(motionDetected ? "YES" : "NO"),
					(sonarDetected ? "YES" : "NO")
					);
			occupancy.occupiedAt = now;
			occupancy.vacatedAt = 0;
		}
		occupancy.occupied = true;
		occupancy.wasOccupiedAt = now;
	} else {
		if (occupancy.occupied && ((now - occupancy.wasOccupiedAt) > occupancyGracePeriod)) {
			printf("INFO: end of occupancy at %d, duration: %d seconds\n",
					(int) (now / 1000),
					(int) ((now - occupancy.occupiedAt) / 1000)
					);
			resetOccupancy();
			occupancy.vacatedAt = now - occupancyGracePeriod;
		}

	}

	showStatus();
	sendStatus();

	delay(500);
}
