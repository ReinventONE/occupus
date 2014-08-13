/*
 * To configure RF24 with Arduino use the following connections:
 *
 *      GND  GND
 *      3V3  3V3
 *      CE     9
 *      CSN   10
 *      SCK   13
 *      MOSI  11
 *      MISO  12
 *
 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include <SimpleTimer.h>

#define INACTIVITY_TIMEOUT 5000
//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10

RF24 radio(9, 10);
SimpleTimer timer(1);
typedef struct publisherInfoStruct {
	uint8_t led;
	bool status;
	bool connected;
	uint64_t pipe;
	uint8_t senderId;
	unsigned long lastTransmissionAt;
	uint8_t errorBlinkState;
} publisherInfo;

publisherInfo publisher[] = {
		{ 7, false, false, 0xF0F0F0F0E1LL, 0x10, 0 },
		{ 8, false, false, 0xF0F0F0F0D2LL, 0x20, 0 }
};

static const uint8_t transmitters = sizeof(publisher) / sizeof(publisherInfo);

void blinkError(int timerId) {
	for (int i = 0; i < transmitters; i++) {
		if (!publisher[i].connected) {
			publisher[i].errorBlinkState++;
			publisher[i].errorBlinkState %= 5;
			digitalWrite(publisher[i].led,
					publisher[i].errorBlinkState == 0 ? HIGH : LOW);
		}
	}
}

void showStatus(int timerId) {
	for (int i = 0; i < transmitters; i++) {
		if (publisher[i].connected) {
			digitalWrite(publisher[i].led, publisher[i].status ? HIGH : LOW);
		}
		printf("transmitter %d is %7s, status = %3s\n", i + 1,
				publisher[i].connected ? "online" : "offline",
				publisher[i].status ? "ON" : "OFF");
	}
	printf("\n");
}

void resetDeadRadios(int timerId) {
	for (int i = 0; i < transmitters; i++) {
		if (millis() - publisher[i].lastTransmissionAt > INACTIVITY_TIMEOUT
				&& publisher[i].status) {
			printf("resetting dead radio %d: last heard %d seconds ago\n",
					i+1,
					(int)((millis() - publisher[i].lastTransmissionAt) / 1000));
			publisher[i].status = false;
			publisher[i].connected = false;
		}
	}
}

void setup(void) {
	Serial.begin(57600);

	printf_begin();
	printf("\n\rRF24: receive module!\n\n");

	radio.begin();
	// optionally, increase the delay between retries & # of retries
	radio.setRetries(15, 15);

	// optionally, reduce the payload size.  seems to improve reliability
	radio.setPayloadSize(8);

	printf("opening for reading: ");
	for (int i = 0; i < transmitters; i++) {
		printf("#%d => [%X], ", i+1,
				publisher[i].pipe);
		radio.openReadingPipe(i + 1, publisher[i].pipe);
		pinMode(publisher[i].led, OUTPUT);
	}
	printf("\n");

	radio.startListening();
	radio.printDetails();

	timer.setInterval(2000, &resetDeadRadios);
	timer.setInterval(990,  &showStatus);
	timer.setInterval(200,  &blinkError);
}
void loop(void) {

	// if there is data ready
	if (radio.available()) {
		unsigned long data = 0;
		bool done = false;
		while (!done) {
			// Fetch the payload, and see if this was the last one.
			done = radio.read(&data, sizeof(data));
			// printf("got data [%d]\n", data);
			for (int i = 0; i < transmitters; i++) {
				if ((data & publisher[i].senderId) == publisher[i].senderId) {
					publisher[i].connected = true;
					publisher[i].status = (data & 1);
					publisher[i].lastTransmissionAt = millis();
					// printf("status for %d is %d", i, publisher[i].status);
				}
			}
			timer.run();
		}
	}
	timer.run();
}
