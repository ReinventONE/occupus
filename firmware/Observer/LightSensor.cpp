/*
 * LightSensor.cpp
 *
 *  Created on: Aug 9, 2014
 *      Author: Konstantin Gredeskoul
 *
 *  (c) 2014 All rights reserved.  Please see LICENSE.
 */

#include "LightSensor.h"

LightSensor::LightSensor(uint8_t pin, unsigned int threshold) {
	_pin = pin;
	_threshold = threshold;
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
	return (getLightReading() > _threshold);
}

uint16_t LightSensor::getLightReading() {
	uint16_t photoCellReading = constrain(analogRead(_pin), 0, 1023);
	return photoCellReading;
}
