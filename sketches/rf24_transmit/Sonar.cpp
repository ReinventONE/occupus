/*
 * Sonar.cpp
 *
 *  Created on: Aug 9, 2014
 *      Author: Konstantin Gredeskoul
 *
 *  (c) 2014 All rights reserved.  Please see LICENSE.
 */

#include "Sonar.h"

Sonar::Sonar(uint8_t triggerPin,
		uint8_t echoPin,
		int maxDistance,
		uint16_t detectDistance) {
	_sonar = new NewPing(triggerPin, echoPin, maxDistance);
	_lastCheckMs = 0;
	_lastDetectedValue = false;
	_detectDistance = detectDistance;
}

bool Sonar::detected() {
	unsigned long now = millis();
	if (now - _lastCheckMs > SONAR_MIN_FREQUENCY) {
		_lastCheckMs = now;
		unsigned int value = _sonar->ping() / US_ROUNDTRIP_CM;
		if (value > 0 && value < _detectDistance) {
			_lastDetectedValue = true;
		} else {
			_lastDetectedValue = false;
		}
	}
	return _lastDetectedValue;
}
