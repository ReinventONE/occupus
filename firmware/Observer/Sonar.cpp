/*
 * Sonar.cpp
 *
 *  Created on: Aug 9, 2014
 *      Author: Konstantin Gredeskoul
 *
 *  (c) 2014 All rights reserved.  Please see LICENSE.
 */

#include "Sonar.h"

Sonar::Sonar(
		uint8_t triggerPin,
		uint8_t echoPin,
		uint16_t maxDistance,
		uint16_t distanceThreshold) {

	_sonar = new NewPing(triggerPin, echoPin, maxDistance);

	_distanceThreshold = distanceThreshold;
	_maxDistance = maxDistance;

	_lastDistance = 0;
	_lastDetected = false;

	_lastCheckMs = 0;
}

uint16_t Sonar::getDistance() {
	unsigned long now = millis();
	if (now - _lastCheckMs > SONAR_MIN_FREQUENCY) {
		_lastCheckMs = now;
		_lastDistance = _sonar->ping() / US_ROUNDTRIP_CM;
		if (_lastDistance > 0 && _lastDistance < _distanceThreshold) {
			_lastDetected = true;
		} else {
			_lastDetected = false;
		}
	}
	return _lastDistance;
}

bool Sonar::detected() {
	getDistance();
	return _lastDetected;
}

void Sonar::setDistanceThreshold(uint16_t distance) {
	_distanceThreshold = constrain(distance, 0, _maxDistance);
}
uint16_t Sonar::getDistanceThreshold() {
	return _distanceThreshold;
}
