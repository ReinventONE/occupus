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

#define ETHERNET_SHIELD
#define SERIAL_LCD

#ifdef ETHERNET_SHIELD
#include <Ethernet.h>
#include "HTTPServer.h"
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
HTTPServer server(mac);
#endif

uint8_t pinSerialLcdRX = 5;

// how many milliseconds should we wait for a dead client to come back
#define SENSOR_CONNECTIVITY_TIMEOUT 5000

#ifdef SERIAL_LCD
SoftwareSerial LcdSerialDisplay(3, pinSerialLcdRX); // pin 8 = TX, pin 0 = RX (unused)
#endif


RF24 radio(7,8);
SimpleTimer timer(1);

observerInfo observers[] = {
		{ 6, false, false, 0xF0F0F0F0E1LL, 0x10, 0, 0, 0, "downstairs", 0x10 },
		{ 9, false, false, 0xF0F0F0F0D2LL, 0x20, 0, 0, 0, "upstairs"  , 0x11 }
};

char buf[36]; // for LCD

static const uint8_t numObservers = sizeof(observers) / sizeof(observerInfo);

void lcdPrintAt(int x, int y, const char *msg) {
	lcdPrintAt(x,y,msg,true);
}

// X is in [ 1, 16 ] and Y is in [1, 2]
void lcdPrintAt(int x, int y, const char *msg, bool clearScreen) {
#ifdef SERIAL_LCD
	if (clearScreen) lcdClearScreen();

	LcdSerialDisplay.write(254);
	int starting = (y == 1) ? 128 : 192;
	LcdSerialDisplay.write(starting + x - 1);

	LcdSerialDisplay.write(msg);
#endif
}

void lcdClearScreen() {
#ifdef SERIAL_LCD
	LcdSerialDisplay.write(254);
	LcdSerialDisplay.write(128);

	LcdSerialDisplay.write("                ");
	LcdSerialDisplay.write("                ");
#endif
}

void updateLEDMatrix(int address, byte data) {
	Wire.beginTransmission(address);
	Wire.write(data);
	Wire.endTransmission();
}

void showStatus(int timerId) {
	lcdClearScreen();
	for (int i = 0; i < numObservers; i++) {
		if (observers[i].connected) {
			digitalWrite(observers[i].led, observers[i].status ? HIGH : LOW);
			updateLEDMatrix(observers[i].ic2addressLED, observers[i].status ? DISP_OCCUPIED : DISP_FREE);
			sprintf(buf, "SNDR %d: %s", i + 1, (observers[i].status ? "Occupied" : "Availabl") );
			lcdPrintAt(1, i + 1, buf, false);
		} else {
			observers[i].errorBlinkState++;
			observers[i].errorBlinkState %= 5;
			digitalWrite(observers[i].led, observers[i].errorBlinkState == 0 ? HIGH : LOW);
			updateLEDMatrix(observers[i].ic2addressLED, DISP_ERROR);
			sprintf(buf, "SNDR %d: %s", i + 1, "Offline");
			lcdPrintAt(1, i + 1, buf, false);
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


void serveJSON(int timerId) {
	server.serveJSON(observers, numObservers);
}

void setup(void) {
	Serial.begin(57600);

#ifdef SERIAL_LCD
    LcdSerialDisplay.begin(9600); // set up serial port for 9600 baud
	pinMode(pinSerialLcdRX, OUTPUT);
	lcdPrintAt(1, 1, "BORAT v1 Server " "Booting....");
	delay(1000);
#endif

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
	lcdPrintAt(1, 1, "Booting HTTPD...", true);
	server.begin();
	timer.setInterval( 980,  &serveJSON);
	lcdPrintAt(1, 2, "IP: ", false);
#ifdef SERIAL_LCD
	LcdSerialDisplay.print(server.ipAddress());
#endif
	delay(2000);
#endif

	timer.setInterval(2000,  &resetDeadRadios);
	timer.setInterval(1000,  &showStatus);

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
