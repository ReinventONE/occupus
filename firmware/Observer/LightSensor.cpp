/*
 * LightSensor.cpp
 *
 *  Created on: Aug 9, 2014
 *      Author: Konstantin Gredeskoul
 *
 *  (c) 2014 All rights reserved.  Please see LICENSE.
 */

#include "LightSensor.h"

LightSensor::LightSensor(uint8_t pin, unsigned int threshold, bool inverted) {
	_pin = pin;
	_threshold = threshold;
	_inverted = inverted;
}

void LightSensor::init() {
	pinMode(_pin, INPUT);
}

void LightSensor::setThreshold(unsigned int threshold) {
	_threshold = constrain(threshold, 1, 1023);
}

unsigned int LightSensor::getThreshold() {
	return _threshold;
}

bool LightSensor::lightsOn() {
	int photoCellReading = analogRead(_pin);
	return (_inverted ? (photoCellReading < 1023 - _threshold) : (photoCellReading > _threshold));
}
