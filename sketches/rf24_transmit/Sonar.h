/*
 * Sonar.h
 *
 *  Created on: Aug 9, 2014
 *      Author: Konstantin Gredeskoul
 *
 *  (c) 2014 All rights reserved.  Please see LICENSE.
 */

#ifndef SONAR_H_
#define SONAR_H_

#if defined(ARDUINO) && ARDUINO >= 100
	#include <Arduino.h>
#else
	#include <WProgram.h>
	#include <pins_arduino.h>
#endif

#include <NewPing.h>

#define SONAR_MIN_FREQUENCY 30

class Sonar {
public:
	Sonar(	uint8_t triggerPin,
			uint8_t echoPin,
			int maxDistance,
			uint16_t detectDistance);
	bool detected(void);
private:
	NewPing *_sonar;
	unsigned long _lastCheckMs;
	bool _lastDetectedValue;
	uint16_t _detectDistance;
};

#endif /* SONAR_H_ */
