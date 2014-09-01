#include "Configuration.h"

Configuration::Configuration(
		configType *pConfig,
		RotaryEncoderWithButton *pRotary) {
	cfg = pConfig;
	mode = NORMAL;
	_rotary = pRotary;
}

void Configuration::init() {
	if (_rotary != NULL)
		readFromEPROM();
}

void Configuration::readFromEPROM() {
	// start reading from position memBase (address 0) of the EEPROM. Set maximumSize to EEPROMSizeUno
	// Writes before membase or beyond EEPROMSizeUno will only give errors when _EEPROMEX_DEBUG is set
	EEPROM.setMemPool(EEPROM_MEM_BASE, EEPROMSizeNano);

	uint32_t secret = EEPROM.readLong(EEPROM_SECRET_ADDRESS);
	if (secret == EEPROM_SECRET_VALUE) {
		EEPROM.readBlock(EEPROM_CONFIG_ADDRESS, _eePromConfig);
		printf("EEPROM: L=%d M=%d D=%d G=%d\n",
				(int) _eePromConfig.lightThreshold,
				(int) _eePromConfig.motionTolerance,
				(int) _eePromConfig.sonarThreshold,
				(int) _eePromConfig.occupancyGracePeriod);
		constrainConfig(&_eePromConfig);
		memcpy(cfg, &_eePromConfig, sizeof(_eePromConfig));
	} else {
		printf("Blank EEPROM, secret=%d\n", secret);
	}
}

void Configuration::saveToEPROM() {
	memcpy(&_eePromConfig, cfg, sizeof(_eePromConfig));
	EEPROM.writeLong(EEPROM_SECRET_ADDRESS, EEPROM_SECRET_VALUE);
	EEPROM.updateBlock(EEPROM_CONFIG_ADDRESS, _eePromConfig);
}


bool Configuration::configure(configStatusCallback callback) {
	if (_rotary->buttonClicked() && mode == NORMAL) {
		if (enterConfiguration(callback)) {
			saveToEPROM();
		}
		return true;
	} else {
		return false;
	}
}

void Configuration::nextMode(configStatusCallback callback) {
	mode = (mode == GRACE) ? NORMAL : (modeType) ((int)mode + 1);
	if (callback != NULL) {	callback(); }
}

void Configuration::constrainConfig(configType *config) {
	config->sonarThreshold = constrain(config->sonarThreshold, 10, 500);
	config->motionTolerance = constrain(config->motionTolerance, 100, 30000);
	config->lightThreshold = constrain(config->lightThreshold, 0, 1023);
	config->occupancyGracePeriod = constrain(config->occupancyGracePeriod, 1, 60);
}


bool Configuration::enterConfiguration(configStatusCallback callback) {
	nextMode(callback);
	bool changed = false;
	while (mode != NORMAL) {
		int delta = _rotary->rotaryDelta();
		if (delta != 0) {
			changed = true;
			switch (mode) {
			case SONAR:
				cfg->sonarThreshold += delta;
				break;
			case MOTION:
				cfg->motionTolerance += 100 * delta;
				break;
			case LIGHT:
				cfg->lightThreshold += delta;
				break;
			case GRACE:
				if (cfg->occupancyGracePeriod < 10) {
					delta = (delta / abs(delta));
				}
				cfg->occupancyGracePeriod += delta;
				break;
			default:
				mode = NORMAL;
			}
			constrainConfig(cfg);
			if (callback != NULL) {	callback(); }
		}

		if (_rotary->buttonClicked()) {
			nextMode(callback);
		}
	}
	return changed;
}
