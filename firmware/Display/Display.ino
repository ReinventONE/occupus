/*
 * RF24 radio:
 *
 *      GND   GND
 *      3V3   3V3
 *      CE    4
 *      CSN   5
 *      SCK   13
 *      MOSI  11
 *      MISO  12
 *
 * Ethernet Shield:
 * 		10, 11, 12, 13
 *
 * Display LEDs:
 * 		6, 7
 *
 */
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include <SimpleTimer.h>
#include "ObserverInfo.h"

// comment this out if you do not have Ethernet Shield
#define ETHERNET_SHIELD

#ifdef ETHERNET_SHIELD
#include <Ethernet.h>
#include "HTTPServer.h"
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
HTTPServer server(mac);
#endif

#define INACTIVITY_TIMEOUT 5000

RF24 radio(4, 5);
SimpleTimer timer(1);

observerInfo observers[] = {
		{ 6, false, false, 0xF0F0F0F0E1LL, 0x10, 0, 0, 0, "downstairs"},
		{ 7, false, false, 0xF0F0F0F0D2LL, 0x20, 0, 0, 0, "upstairs"}
};

static const uint8_t numObservers = sizeof(observers) / sizeof(observerInfo);

void blinkError(int timerId) {
	for (int i = 0; i < numObservers; i++) {
		if (!observers[i].connected) {
			observers[i].errorBlinkState++;
			observers[i].errorBlinkState %= 5;
			digitalWrite(observers[i].led,
					observers[i].errorBlinkState == 0 ? HIGH : LOW);
		}
	}
}

void showStatus(int timerId) {
	for (int i = 0; i < numObservers; i++) {
		if (observers[i].connected) {
			digitalWrite(observers[i].led, observers[i].status ? HIGH : LOW);
		}
		printf("transmitter %d is %7s, status = %3s\n", i + 1,
				observers[i].connected ? "online" : "offline",
				observers[i].status ? "ON" : "OFF");
	}
	printf("\n");
}

void resetDeadRadios(int timerId) {
	for (int i = 0; i < numObservers; i++) {
		if (millis() - observers[i].lastTransmissionAt > INACTIVITY_TIMEOUT
				&& observers[i].status) {
			printf("resetting dead radio %d: last heard %d seconds ago\n",
					i+1,
					(int)((millis() - observers[i].lastTransmissionAt) / 1000));
			observers[i].status = false;
			observers[i].connected = false;
		}
	}
}


void serveJSON(int timerId) {
	server.serveJSON(observers, numObservers);
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

	printf("Opening for reading: ");
	for (int i = 0; i < numObservers; i++) {
		printf("#%d => [%X], ", i+1, (unsigned int) observers[i].pipe);
		radio.openReadingPipe(i + 1, observers[i].pipe);
		pinMode(observers[i].led, OUTPUT);
	}
	printf("\n");

	radio.startListening();
	radio.printDetails();

#ifdef ETHERNET_SHIELD
	server.begin();
	timer.setInterval( 980,  &serveJSON);
#endif

	timer.setInterval(2000,  &resetDeadRadios);
	timer.setInterval( 990,  &showStatus);
	timer.setInterval( 200,  &blinkError);
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
			for (int i = 0; i < numObservers; i++) {
				if ((data & observers[i].senderId) == observers[i].senderId) {
					observers[i].connected = true;
					bool status = (data & 1);
					if (observers[i].status != status) {
						observers[i].lastStateChangeAt = millis();
						observers[i].status = status;
					}
					observers[i].lastTransmissionAt = millis();
					// printf("status for %d is %d", i, publisher[i].status);
				}
			}
			timer.run();
		}
	}
	timer.run();
}
