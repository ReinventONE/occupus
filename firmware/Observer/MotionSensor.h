/*
 * MotionSensor.h
 *
 *  Created on: Aug 9, 2014
 *      Author: Konstantin Gredeskoul
 *
 *  (c) 2014 All rights reserved.  Please see LICENSE.
 */

#ifndef MOTIONSENSOR_H_
#define MOTIONSENSOR_H_

#if defined(ARDUINO) && ARDUINO >= 100
	#include <Arduino.h>
#else
	#include <WProgram.h>
	#include <pins_arduino.h>
#endif

// recommended values are 10-60s
#define CALIBRATION_SECONDS 		10

class MotionSensor {
public:
	MotionSensor(
			// pin IR sensor is attached to
			uint8_t pin,
			// The amount of milliseconds the sensor has to be low
			// before we assume all motion has stopped
			long unsigned int pause);

	void init(int calibrationWaitMillis);
	void init();

	bool detectedNonRetriggering();
	bool detected();
	void setPause(int pause);
	int getPause();
private:
	long unsigned int _lowIn, _pause; // time of the transition from high to LOW
	boolean _lockLow, _lastDetection,_takeLowTime;
	uint8_t _pirPin;
};

#endif /* MOTIONSENSOR_H_ */
