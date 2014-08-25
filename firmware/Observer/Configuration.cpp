#include "Configuration.h"

Configuration::Configuration(
		configType *pConfig,
		RotaryEncoderWithButton *pRotary) {
	cfg = pConfig;
	mode = NORMAL;
	_rotary = pRotary;
}

bool Configuration::configure(configStatusCallback callback) {
	if (_rotary->buttonClicked() && mode == NORMAL) {
		enterConfiguration(callback);
		return true;
	} else {
		return false;
	}
}

void Configuration::nextMode(configStatusCallback callback) {
	mode = (mode == GRACE) ? NORMAL : (modeType) ((int)mode + 1);
	if (callback != NULL) {	callback(); }
}

void Configuration::enterConfiguration(configStatusCallback callback) {
	nextMode(callback);
	while (mode != NORMAL) {
		int delta = _rotary->rotaryDelta();
		if (delta != 0) {
			printf("=> delta value is %d\n", delta);
			switch (mode) {
			case SONAR:
				cfg->sonarThreshold += delta;
				printf("Set SONAR threshold to %d\n", cfg->sonarThreshold);
				break;
			case MOTION:
				cfg->motionTolerance += delta;
				printf("Set MOTION pause to %d\n", (int) cfg->motionTolerance);
				break;
			case LIGHT:
				cfg->lightThreshold += delta;
				printf("Set LIGHT threshold to %d\n", cfg->lightThreshold);
				break;
			case GRACE:
				cfg->occupancyGracePeriod += 10 * delta;
				printf("Set Occupancy Grace Period to %d\n", (int) cfg->occupancyGracePeriod);
				break;
			default:
				printf("Unknown mode\n");
				mode = NORMAL;
			}
			if (callback != NULL) {	callback(); }
		}

		if (_rotary->buttonClicked()) {
			nextMode(callback);
		}
	}
}
