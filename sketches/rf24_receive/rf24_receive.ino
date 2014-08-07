/*
 * This code is based on the example by
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>
 */

/*
 * Connections:
 *
 	GND		GND
  	3V3		3V3
	CE		9
	CSN		10
	SCK		13
	MOSI	11
	MISO	12

 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10

RF24 radio(9, 10);

const uint8_t receiveLed[] = {7,8};
static const uint8_t transmitters = sizeof(receiveLed) / sizeof(uint8_t);
static bool status[transmitters];

//
// Topology
//

const uint64_t pipes[] 		= { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL	};
const uint8_t states[] 		= { 0x0, 			0x1 			}; // false or true
const uint8_t senders[] 	= { 0x010, 			0x020 			}; // on, off

void blink() {
	for (int i = 0; i < transmitters; i++) {
		digitalWrite(receiveLed[i], status[i] ? HIGH : LOW);
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

	radio.openReadingPipe(1, pipes[0]);
	radio.openReadingPipe(2, pipes[1]);

	radio.startListening();
	radio.printDetails();

	for (int i = 0; i < transmitters; i++) {
		pinMode(receiveLed[i], OUTPUT);
		status[i] = false;
	}

}
void loop(void) {

	// if there is data ready
	if (radio.available()) {
		unsigned long data = 0;
		bool done = false;
		while (!done) {
			// Fetch the payload, and see if this was the last one.
			done = radio.read(&data, sizeof(data));
			printf("got data [%d]\n", data);
			for (int i = 0; i < transmitters; i++) {
				if ((data & senders[i]) == senders[i]) {
					status[i] = data & 1;
					printf("status for %d is %d", i, status[i]);
				}
			}
		}
		printf("\n");
		blink();
	}
}
