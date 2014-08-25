/*
 * Configuration class for the Observer sensors
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
#include <EEPROMex.h>
#include <EEPROMvar.h>

typedef enum {
	NORMAL = 0,
	LIGHT,
	MOTION,
	SONAR,
	GRACE } modeType;

typedef void(*configStatusCallback)(void);

// start reading from the first byte (address 120) of the EEPROM
const int EEPROM_MEM_BASE = 0;
// if this value was written we've saved before
const uint32_t EEPROM_SECRET_VALUE 		= 0x19d7e41d;
const uint32_t EEPROM_SECRET_ADDRESS 	= 0;
const uint32_t EEPROM_CONFIG_ADDRESS 	= 8;

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
	void init();
	void saveToEPROM();
	void readFromEPROM();
	bool configure(configStatusCallback callback);
	configType *cfg;
	modeType mode;
private:
	void constrainConfig(configType *config);
	configType _eePromConfig;
	void nextMode(configStatusCallback callback);
	bool enterConfiguration(configStatusCallback callback);
	RotaryEncoderWithButton *_rotary;
};

#endif /* CONFIGURATION_H_ */
