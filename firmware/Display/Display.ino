/*
 * RF24 radio:
 *
 *  GND  = GND    VCC  = 5V
 *  CE   = 7(9)   CSN  = 8 (normally 10)
 *  SCK  = 13     MOSI = 11
 *  MISO = 12     IRQ
 *
 *
 * Ethernet Shield:
 * 		10, 11, 12, 13
 *
 * Display LEDs:
 * 		4, 5
 *
 */
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include <SimpleTimer.h>
#include <Wire.h>
#include <SoftwareSerial.h>

#include "printf.h"
#include "ObserverInfo.h"

#define DISP_ERROR 		0
#define DISP_FREE 		1
#define DISP_OCCUPIED   2

#include <SparkfunSerialLCD.h>

#define ETHERNET_SHIELD
#define SERIAL_LCD

#ifdef ETHERNET_SHIELD
#include <Ethernet.h>
#include "HTTPServer.h"
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
HTTPServer server(mac);
#endif

uint8_t pinSerialLcdRX = 5;

SparkfunSerialLCD debugLCD(pinSerialLcdRX);

// how many milliseconds should we wait for a dead client to come back
#define SENSOR_CONNECTIVITY_TIMEOUT 5000

#ifdef ETHERNET_SHIELD
RF24 radio(7,8); // ethernet shield uses 9 and 10
#else
RF24 radio(9,10);
#endif

SimpleTimer timer(1);

observerInfo observers[] = {
		{ 6, false, false, 0xF0F0F0F0E1LL, 0x10, 0, 0, 0, "downstairs", 0x10 },
		{ 9, false, false, 0xF0F0F0F0D2LL, 0x20, 0, 0, 0, "upstairs"  , 0x11 }
};

char buf[36]; // for LCD

static const uint8_t numObservers = sizeof(observers) / sizeof(observerInfo);

void updateLEDMatrix(int address, byte data) {
	Wire.beginTransmission(address);
	Wire.write(data);
	Wire.endTransmission();
}

void showStatus(int timerId) {
	debugLCD.clear();
	for (int i = 0; i < numObservers; i++) {
		if (observers[i].connected) {
			digitalWrite(observers[i].led, observers[i].status ? HIGH : LOW);
			updateLEDMatrix(observers[i].ic2addressLED, observers[i].status ? DISP_OCCUPIED : DISP_FREE);
			sprintf(buf, "Room %d: %s", i + 1, (observers[i].status ? "Occupied" : "Vacant") );
			debugLCD.printAt(1, i+1, buf);
		} else {
			observers[i].errorBlinkState++;
			observers[i].errorBlinkState %= 5;
			digitalWrite(observers[i].led, observers[i].errorBlinkState == 0 ? HIGH : LOW);
			updateLEDMatrix(observers[i].ic2addressLED, DISP_ERROR);
			sprintf(buf, "Room %d: %s", i + 1, "Down");
			debugLCD.printAt(1, i+1, buf);
		}
		printf("transmitter %d is %7s, status = %3s\n", i + 1,
				observers[i].connected ? "online" : "offline",
				observers[i].status ? "ON" : "OFF");
	}
	printf("\n");
}

void resetDeadRadios(int timerId) {
	for (int i = 0; i < numObservers; i++) {
		if (millis() - observers[i].lastTransmissionAt > SENSOR_CONNECTIVITY_TIMEOUT
				&& observers[i].status) {
			printf("resetting dead radio %d: last heard %d seconds ago\n",
					i+1,
					(int)((millis() - observers[i].lastTransmissionAt) / 1000));
			observers[i].status = false;
			observers[i].connected = false;
		}
	}
}

void showMyIP(int timerId) {
#ifdef ETHERNET_SHIELD
	debugLCD.print("My IP Address:", "IP: ");
	debugLCD.serial()->print(server.ipAddress());
#endif
}

void serveJSON(int timerId) {
#ifdef ETHERNET_SHIELD
	server.serveJSON(observers, numObservers);
#endif
}

void setup(void) {
	Serial.begin(57600);

    debugLCD.init();

	debugLCD.print("PooCast Ver 1.0", "Booting....");
	delay(1000);
	printf_begin();

	radio.begin();
	// optionally, increase the delay between retries & # of retries
	radio.setRetries(15, 15);

	// optionally, reduce the payload size.  seems to improve reliability
	radio.setPayloadSize(8);

	printf("opening for reading: ");
	for (int i = 0; i < numObservers; i++) {
		printf("#%d => [%X], ", i+1, (unsigned int) observers[i].pipe);
		radio.openReadingPipe(i + 1, observers[i].pipe);
		pinMode(observers[i].led, OUTPUT);
	}
	printf("\n");

	radio.startListening();
	radio.printDetails();

#ifdef ETHERNET_SHIELD
	debugLCD.print("Network Setup", "Qry IP via DHCP");
	server.begin();
	timer.setInterval(980,  &serveJSON);
	debugLCD.printAt(1, 2, "IP: ");
	debugLCD.serial()->print(server.ipAddress());
	delay(2000);
	debugLCD.print("Get Ready for..","the PooCast!");
	delay(500);
#endif

	timer.setInterval(2000,   &resetDeadRadios);
	timer.setInterval(1000,   &showStatus);
	timer.setInterval(5000,   &showMyIP);

	Wire.begin();
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
