/*
 * Connections:
 *
 GND		GND
 3V3		3V3
 CE		    9
 CSN		10
 SCK		13
 MOSI	    11
 MISO	    12

 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include <NewPing.h>

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10

RF24 radio(9, 10);
uint8_t pinStatusLed = 7, pinIRInput = 3, pinPhotoCell = A0;

int photoCellReading;
bool occupied = false;
const uint8_t states[] = { 0x000, 0x001 }; // VACANT, OCCUPIED, ...

// Radio pipe addresses for the 2 nodes to communicate.
// read 1          read 2
const uint64_t pipes[] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
const uint8_t senders[] = { 0x010, 0x020 }; // first sender, second, etc.

const uint8_t me = 1; // 0 or 1 (offset into senders[]) and pipes[]

#define TRIGGER_PIN       5  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN    	  4  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 	500  // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
#define CHECK_DISTANCE 	200

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

static bool didSonarDetectObject = false;
static unsigned long lastSonarAtMs = 0;
static const uint8_t sonarCheckPeriodMs = 30; // don't check more often than that

unsigned long lastSent = 0;
const unsigned int sendPeriod = 300;

unsigned long lastOccupiedMs = 0;

bool isObjectDetected() {
	if (millis() > sonarCheckPeriodMs + lastSonarAtMs) {
		lastSonarAtMs = millis();
		unsigned int value = sonar.ping() / US_ROUNDTRIP_CM;
		if (value > 0 && value < CHECK_DISTANCE) {
			printf("spaceAhead is %d, less than %d\n", value, CHECK_DISTANCE);
			didSonarDetectObject = true;
		} else {
			didSonarDetectObject = false;
		}
	}
	return didSonarDetectObject;
}

void showStatus() {
	digitalWrite(pinStatusLed, occupied ? HIGH : LOW);
}

void sendStatus() {
	if (millis() - lastSent > sendPeriod) {
		lastSent = millis();

		unsigned long data = occupied | senders[me];
		printf("transmitting [%s], me=[%d], data=[%d]\n",
				occupied ? "OCCIPIED" : "VACANT", me, data);

		bool ok = radio.write(&data, sizeof(data));
		if (!ok)
			printf("error sending\n\r");
	}
}

bool isItBright() {
	photoCellReading = analogRead(pinPhotoCell);
	return (photoCellReading > 250);
}

bool isMotionRecentlyDetected() {
	return digitalRead(pinIRInput) == HIGH;
}

void setup(void) {
	Serial.begin(9600);

	printf_begin();
	printf("\nRF24: transmit module #%d!\n", me);

	radio.begin();
	radio.setRetries(15, 15);
	radio.setPayloadSize(8);
	radio.openWritingPipe(pipes[me]);
	radio.printDetails();

	pinMode(pinStatusLed, OUTPUT);

	printf("calibrating PIR sensor, please wait... ");
	pinMode(pinIRInput, INPUT);
	digitalWrite(pinIRInput, LOW);
	delay(10000);
	printf("done.\n");

}

void loop(void) {
	bool isSomeoneThere;
	isSomeoneThere = isItBright() && (isMotionRecentlyDetected() || isObjectDetected());
	if (isSomeoneThere) {
		occupied = true;
		lastOccupiedMs = millis();
	} else {
		if (occupied && (millis() - lastOccupiedMs) > 10000) {
			occupied = false;
		}
	}

	showStatus();

	sendStatus();
}
