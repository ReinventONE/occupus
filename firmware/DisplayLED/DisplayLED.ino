/*
 * DisplayLED for Rainbowduino to be used over I2C connection
 * with the display unit of the PooCast.
 *
 * Created on: Aug 28, 2014
 * Author: Konstantin Gredeskoul
 *
 * Dependencies:
 *   - Wire
 *   - AsciiDuino
 *   - Rainbowduino
 *   - SimpleTimer
 *
 * © 2014 All rights reserved.  Please see LICENSE.
 *
 */

#include <Rainbowduino.h>
#include <AsciiDuino.h>
#include <SimpleTimer.h>
#include <Wire.h>

// set to 0 or 1
int me = 1;

AsciiDuino matrix(8);

const char *errorFrames[] = {
		"........"
		"........"
		"........"
		"X......."
		"X......."
		"........"
		"........"
		"........",

		"........"
		"........"
		"........"
		"XX......"
		"XX......"
		"........"
		"........"
		"........",

		"........"
		"........"
		"........"
		".XX....."
		".XX....."
		"........"
		"........"
		"........",

		"........"
		"........"
		"........"
		"..XX...."
		"..XX...."
		"........"
		"........"
		"........",

		"........"
		"........"
		"........"
		"...XX..."
		"...XX..."
		"........"
		"........"
		"........",

		"........"
		"........"
		"........"
		"....XX.."
		"....XX.."
		"........"
		"........"
		"........",

		"........"
		"........"
		"........"
		".....XX."
		".....XX."
		"........"
		"........"
		"........",

		"........"
		"........"
		"........"
		"......XX"
		"......XX"
		"........"
		"........"
		"........",

		"........"
		"........"
		"........"
		".......X"
		".......X"
		"........"
		"........"
		"........",

		"........"
		"........"
		"........"
		"........"
		"........"
		"........"
		"........"
		"........"

};

const char *SIGN =
		"XXXXXXX."
		"XXXXXXX."
		"XXXXXXX."
		"XXXXXXX."
		"XXXXXXX."
		"XXXXXXX."
                                                    		"XXXXXXX."
		"........";

const uint32_t
		UNOCCUPIED 	= 0x008000,
		OCCUPIED 	= 0x800000,
		UNKNOWN 	= 0x00080;

SimpleTimer timer(1);

int i = 0;


const uint32_t states[] = {UNOCCUPIED, OCCUPIED, UNKNOWN };
byte previous = -1, current = 2;

int addresses[] = { 0x10, 0x11 };

void receiveEvent(int howMany) {
	while(Wire.available() > 0) {
		byte value = Wire.read();
		if (value < (sizeof(states) / sizeof(uint32_t))) {
			current = value;
		}
	}

}

void showStatus (int timerId ) {
	uint32_t c = states[current];
	if (c != UNKNOWN) {
		if (current != previous)
			matrix.frame(SIGN, c);
		previous = current;
	} else {
		i = i % (sizeof(errorFrames) / sizeof(char *));
		matrix.frame(errorFrames[i], states[i % 4]);
		i++;
	}
}

void setup() {
	Serial.begin(57600);

	matrix.init();
	Wire.begin(addresses[me]);
	Wire.onReceive(receiveEvent);
	timer.setInterval(100, &showStatus);
}

void loop() {
	timer.run();
}
