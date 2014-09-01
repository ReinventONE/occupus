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
		"XXXXXXXX"
		"XXXXXXXX"
		"XXXXXXXX"
		"XXXXXXXX"
		"XXXXXXXX"
		"XXXXXXXX"
		"XXXXXXXX"
		"XXXXXXXX";

SimpleTimer timer(1);

int i = 0;

const RGBColor RED 		= {{ 128,   0,   0 }};
const RGBColor GREEN 	= {{   0, 128,   0 }};
const RGBColor BLUE		= {{   0,   0, 128 }};
const RGBColor BLACK	= {{   0,   0,   0 }};

const uint8_t  primaryColorVariationAmount = 64,
			   secondaryColorVariationAmount = 64;

typedef enum {
		UNKNOWN 	= 0,
		UNOCCUPIED,
		OCCUPIED } state;

const state 	states[] 		= {UNKNOWN, UNOCCUPIED, OCCUPIED};
const RGBColor  stateColor[] 	= {BLUE,    GREEN,       RED};

const uint8_t   numStates = (sizeof(stateColor) / sizeof(RGBColor));

byte previous = -1, current = 0;
float variationCoefficient = 0, lastTimeCoefficient = 0;
bool goingUp = false;

int addresses[] = { 0x10, 0x11 };

void receiveEvent(int howMany) {
	while(Wire.available() > 0) {
		byte value = Wire.read();
		if (value < numStates) {
			current = value;
		}
	}
}

RGBColor newColor(uint8_t x, uint8_t y, char value, RGBColor previousColor) {
	float currentTimeCoefficient = (millis() % 5000) / 5000.0; // 0 .. 1
	if (currentTimeCoefficient < lastTimeCoefficient) {
		goingUp = !goingUp;
	}

	lastTimeCoefficient = currentTimeCoefficient;
	variationCoefficient = goingUp ? currentTimeCoefficient : (1 - currentTimeCoefficient);

	RGBColor c = previousColor;
	for (int i = 0; i < 3; i++) {
		if (c.rgb[i] > 0) {
			c.rgb[i] = c.rgb[i] +
					(rand() % primaryColorVariationAmount -
							(primaryColorVariationAmount / 4));
		} else {
			c.rgb[i] =  variationCoefficient * (rand() % secondaryColorVariationAmount);
		}
	}
	return c;
}

void showStatus (int timerId ) {
	if (current != UNKNOWN) {
		matrix.frame(SIGN, stateColor[current], newColor);
		previous = current;
	}
}

void showError(int timerId ) {
	if (current == UNKNOWN) {
		i = i % (sizeof(errorFrames) / sizeof(char *));
		matrix.frame(errorFrames[i], stateColor[i % numStates]);
		i++;
	}
}

void setup() {
	Serial.begin(57600);

	matrix.init();
	Wire.begin(addresses[me]);
	Wire.onReceive(receiveEvent);
	timer.setInterval(100, &showStatus);
	timer.setInterval(200, &showError);
}

void loop() {
	timer.run();
}
