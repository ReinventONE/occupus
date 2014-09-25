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

typedef enum {
	NORMAL = 0,
	ROOM,
	LIGHT,
	MOTION,
	SONAR,
	GRACE,
	SAVING,
	RADIO_TOGGLE
}	modeType;

#define LAST_MODE RADIO_TOGGLE

typedef void(*configStatusCallback)(void);

// start reading from the first byte (address 120) of the EEPROM
const int EEPROM_MEM_BASE = 0;
// if this value was written we've saved before
const uint32_t EEPROM_SECRET_VALUE 		= 0x1998ffac;
const uint32_t EEPROM_SECRET_ADDRESS 	= 0;
const uint32_t EEPROM_CONFIG_ADDRESS 	= 8;

typedef struct nonPersistedSettings {
	bool shouldSaveSettings;
	bool shouldToggleRadioState;
	bool settingsChanged;
} unPersistedType;

// Various thresholds that trigger sensors
typedef struct configStruct {
	uint16_t lightThreshold;
	uint32_t motionTolerance;
	uint16_t sonarThreshold;
	uint32_t occupancyGracePeriod;
	uint8_t  mySenderIndex;
} configType;

typedef struct senderInfoStruct {
	bool connected;
	uint64_t pipe;
	uint8_t senderId;
	char name[11 ];
} senderInfo;

const senderInfo senders[] = {
		{ false, 0xF0F0F0F0E1LL, 0x010, "Downstairs"},
		{ false, 0xF0F0F0F0D2LL, 0x020, "Upstairs"},
		{ false, 0xF0F0F0F0F2LL, 0x030, "Room 3"},
		{ false, 0xF0F0F0F0A2LL, 0x040, "Room 4"},
		{ false, 0xF0F0F0F0C2LL, 0x050, "Room 5"}
};

class Configuration {
public:
	Configuration(configType *config, RotaryEncoderWithButton *rotary);
	void init();
	void saveToEPROM();
	void readFromEPROM();
	bool configure(configStatusCallback callback);
	unPersistedType *session();
	configType *cfg;
	modeType mode;
private:
	void constrainConfig(configType *config);
	configType _eePromConfig;
	unPersistedType _session;
	void nextMode(configStatusCallback callback);
	bool enterConfiguration(configStatusCallback callback);
	RotaryEncoderWithButton *_rotary;
	bool _isRadioOn;
	bool _wasRadioOn;
};

#endif /* CONFIGURATION_H_ */
