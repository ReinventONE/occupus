/*
 * Configuration.h
 *
 *  Created on: Aug 23, 2014
 *      Author: Konstantin Gredeskoul
 *
 *  (c) 2014 All rights reserved.  Please see LICENSE.
 */

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#if defined(ARDUINO) && ARDUINO >= 100
	#include <Arduino.h>
#else
	#include <WProgram.h>
	#include <pins_arduino.h>
#endif

#include <SimpleTimer.h>
#include <RotaryEncoderWithButton.h>

typedef enum {
	NORMAL = 0,
	LIGHT,
	MOTION,
	SONAR,
	GRACE } modeType;

typedef void(*configStatusCallback)(void);


// Various thresholds that trigger sensors
typedef struct configStruct {
	uint16_t lightThreshold;
	uint32_t motionTolerance;
	uint16_t sonarThreshold;
	uint32_t occupancyGracePeriod;
} configType;


class Configuration {
public:
	Configuration(configType *config, RotaryEncoderWithButton *rotary);
	void writeToEPROM();
	bool readFromEPROM();
	bool configure(configStatusCallback callback);
	configType *cfg;
	modeType mode;
private:
	void nextMode(configStatusCallback callback);
	void enterConfiguration(configStatusCallback callback);
	RotaryEncoderWithButton *_rotary;
};

#endif /* CONFIGURATION_H_ */
