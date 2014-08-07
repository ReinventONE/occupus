
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
uint8_t sendLed = 6;


//
// Topology
//

// Radio pipe addresses for the 2 nodes to communicate.
                              // read 1          read 2
const uint64_t pipes[] 		= { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL};
const uint8_t states[] 		= { 0x0, 			0x1 }; // false or true
const uint8_t senders[] 	= { 0x010, 			0x020 }; // on, off

const uint8_t me = 1; // or 1

bool status = false;

void blink() {
	status = !status;
	digitalWrite(sendLed, status ? HIGH : LOW);
}

void setup(void) {
	Serial.begin(57600);

	printf_begin();
	printf("\n\rRF24: transmit module #%d!\n", me);

	radio.begin();
	// optionally, increase the delay between retries & # of retries
	radio.setRetries(15, 15);
	// optionally, reduce the payload size.  seems to
	// improve reliability
	radio.setPayloadSize(8);
	radio.openWritingPipe(pipes[me]);
	radio.printDetails();
	pinMode(sendLed, OUTPUT);
}

void loop(void) {
	blink();
	unsigned long data = status | senders[me];
	printf("transmitting [%s], me=[%d], data=[%d]\n", status ? "YES" : "NO", me, data);
	bool ok = radio.write(&data, sizeof(data));

	if (!ok)
		printf("error sending\n\r");

	delay(1000);
}
