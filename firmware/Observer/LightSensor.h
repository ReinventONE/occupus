/*
 * LightSensor.h
 *
 *  Created on: Aug 9, 2014
 *      Author: Konstantin Gredeskoul
 *
 *  (c) 2014 All rights reserved.  Please see LICENSE.
 */

#ifndef LIGHTSENSOR_H_
#define LIGHTSENSOR_H_

#if defined(ARDUINO) && ARDUINO >= 100
	#include <Arduino.h>
#else
	#include <WProgram.h>
	#include <pins_arduino.h>
#endif

class LightSensor {
public:
	LightSensor(
			// pin sensor is attached to
			uint8_t pin,
			// threshold between 0 and 1023 below which we are considered dark
			unsigned int threshold);
	void init();
	bool lightsOn();
	uint16_t getLightReading();
	void setThreshold(unsigned int threshold);
	unsigned int getThreshold();
private:
	unsigned int _threshold; // time of the transition from high to LOW
	uint8_t _pin;
	bool _inverted;
};

#endif /* LIGHTSENSOR_H_ */
