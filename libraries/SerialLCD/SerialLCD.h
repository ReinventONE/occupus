/*
 * SerialLCD.h
 *
 * Sparkfun LCD Serial Display
 *
 *  Created on: Sep 1, 2014
 *      Author: Konstantin Gredeskoul
 *
 *  (c) 2014 All rights reserved.  Please see LICENSE.
 */

#ifndef SERIALLCD_H_
#define SERIALLCD_H_

#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#include <pins_arduino.h>
#endif

#include <SoftwareSerial.h>

#define SERIAL_LCD_DEFAULT_WIDTH  16
#define SERIAL_LCD_DEFAULT_HEIGHT 2

class SerialLCD {
public:
	SerialLCD(uint8_t serialPin);
	void setDimensions(uint8_t width, uint8_t height);
	void init();
	void print(const char *msg);
	void print(const char *line1, const char *line2);
	void printAt(int x, int y, const char *msg);
	void clear();
	SoftwareSerial *serial();
private:
	SoftwareSerial display();
	SoftwareSerial *_display;
	uint8_t _w, _h;
	uint8_t _pinTx;
};

#endif /* SERIALLCD_H_ */
