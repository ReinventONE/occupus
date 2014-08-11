/*
 * SimpleNetwork.h
 *
 * Supports up to 5 clients sending data to a single collector,
 * typically sensor data. Data packet exchanged is a single
 * 32-bit value of type "unsigned long int". First byte
 * is used to id the sender, and the remaining 7 bytes can be
 * used by the client to send data.
 *
 * RF24-based network of up to 5 senders, and 1 receiver.
 *
 * To configure RF24 with Arduino use the following connections:
 *
 *      GND  GND
 *      3V3  3V3
 *      CE     9
 *      CSN   10
 *      SCK   13
 *      MOSI  11
 *      MISO  12
 *
 *  Created on: Aug 10, 2014
 *      Author: Konstantin Gredeskoul
 *
 *  (c) 2014 All rights reserved.  Please see LICENSE.
 */

#ifndef SIMPLENETWORK_H_
#define SIMPLENETWORK_H_

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

typedef struct clientInfoStruct {
	bool isConnected;
	uint64_t pipeId;
	uint8_t senderId;
} client;


const client CLIENTS[5] = {
	{ false, 0xF0F0F0F0E0LL, 0x0 },
	{ false, 0xF0F0F0F0E1LL, 0x1 },
	{ false, 0xF0F0F0F0E2LL, 0x2 },
	{ false, 0xF0F0F0F0E3LL, 0x3 },
	{ false, 0xF0F0F0F0E4LL, 0x4 }
};

typedef void(*callback)(uint8_t clientId, uint64_t data);

class SimpleNetwork {
public:
	SimpleNetwork(uint8_t radioPin1, uint8_t radioPin2);
	void begin();
	RF24 *radio;
};

class SimpleNetworkClient : SimpleNetwork {
public:
	SimpleNetworkClient(uint8_t radioPin1, uint8_t radioPin2);
	void send(uint64_t data);
};

class SimpleNetworkServer : SimpleNetwork {
public:
	SimpleNetworkServer(uint8_t radioPin1, uint8_t radioPin2);
	void listen(callback* callback);
};

#endif /* SIMPLENETWORK_H_ */
