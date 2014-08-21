/*
 * ObserverInfo.h
 *
 *  Created on: Aug 13, 2014
 *      Author: Konstantin Gredeskoul
 *
 *  (c) 2014 All rights reserved.  Please see LICENSE.
 */

#ifndef OBSERVERINFO_H_
#define OBSERVERINFO_H_


typedef struct observerInfoStruct {
	uint8_t led;
	bool status;
	bool connected;
	uint64_t pipe;
	uint8_t senderId;
	unsigned long lastTransmissionAt;
	unsigned long lastStateChangeAt;
	uint8_t errorBlinkState;
	const char *name;
} observerInfo;



#endif /* OBSERVERINFO_H_ */
