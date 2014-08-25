/*
 * MotionSensor.cpp
 *
 *  Created on: Aug 9, 2014
 *      Author: Konstantin Gredeskoul
 *
 *  (c) 2014 All rights reserved.  Please see LICENSE.
 *
 * Portions of this code are from the following example:
 *
 * Switches a LED according to the state of the sensors output pin.
 * Determines the beginning and end of continuous motion sequences.
 *
 * @author: Kristian Gohlke / krigoo (_) gmail (_) com / http://krx.at
 * @date:   3. September 2006
 *
 * The sensor's output pin goes to HIGH if motion is present.
 * However, even if motion is present it goes to LOW from time to time,
 * which might give the impression no motion is present.
 * This program deals with this issue by ignoring LOW-phases shorter than a given time,
 * assuming continuous motion is present during these phases.
 */

#include "MotionSensor.h"

MotionSensor::MotionSensor(uint8_t pirPin, long unsigned int pause) {
	_pirPin = pirPin;
	_pause = pause;
	_lastDetection = false;

}

void MotionSensor::init(int calibrationWaitMillis) {
	pinMode(_pirPin, INPUT);
	digitalWrite(_pirPin, LOW);
	printf("Calibrating %d sec..\n", calibrationWaitMillis / 1000);
	delay(calibrationWaitMillis);
}

void MotionSensor::init() {
	return init(CALIBRATION_SECONDS);
}

void MotionSensor::setPause(int pause) {
	_pause = constrain(pause, 100, 10000);
}

int MotionSensor::getPause() {
	return _pause;
}

bool MotionSensor::detected() {
	_lastDetection = (digitalRead(_pirPin) == HIGH);
	return _lastDetection;
}

bool MotionSensor::detectedNonRetriggering() {
	if (digitalRead(_pirPin) == HIGH) {
		if (_lockLow) {
			_lastDetection = true;
			//makes sure we wait for a transition to LOW before any further output is made:
			_lockLow = false;
			delay(50);
		}
		_takeLowTime = true;
	}

	if (digitalRead(_pirPin) == LOW) {
		if (_takeLowTime) {
			_lowIn = millis(); 		// save the time of the transition from high to LOW
			_takeLowTime = false; 	// make sure this is only done at the start of a LOW phase
		}

		//if the sensor is low for more than the given pause,
		//we assume that no more motion is going to happen
		if (!_lockLow && millis() - _lowIn > _pause) {
			// makes sure this block of code is only executed again after
			// a new motion sequence has been detected
			_lastDetection = false;
			_lockLow = true;
			delay(50);
		}
	}
	return _lastDetection;
}

