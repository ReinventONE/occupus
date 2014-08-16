/*
 * SimpleNetwork.cpp
 *
 *  Created on: Aug 10, 2014
 *      Author: Konstantin Gredeskoul
 *
 *  (c) 2014 All rights reserved.  Please see LICENSE.
 */

#include "SimpleNetwork.h"

SimpleNetwork::SimpleNetwork(uint8_t radioPin1, uint8_t radioPin2) {
	radio = new RF24(radioPin1, radioPin2);
}

void SimpleNetwork::begin() {
	radio->begin();
	radio->setRetries(15, 15);
	radio->setPayloadSize(8);  // sending 8 bytes, or 64 bits
}

//–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

SimpleNetworkClient::SimpleNetworkClient(
		uint8_t radioPin1,
		uint8_t radioPin2,
		uint8_t clientId) :
				SimpleNetwork(radioPin1, radioPin2) {

	id = clientId;
}

void SimpleNetworkClient::begin() {
	SimpleNetwork::begin();
	printf("%d >> opening for writing pipe: [%X]\n", id, clients[id].pipeId);
	radio->openWritingPipe(clients[id].pipeId);
	radio->printDetails();

}

bool SimpleNetworkClient::send(uint64_t data) {
	// 4 bytes for id, then the rest is from the data.
	data = (data << 4) | id;
	printf("%d >> sending data [%d]\n", id, data);
	return radio->write(&data, sizeof(uint64_t));
}

//–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

SimpleNetworkServer::SimpleNetworkServer(
		uint8_t radioPin1,
		uint8_t radioPin2) :
				SimpleNetwork(radioPin1, radioPin2) {

	for (int i = 0; i < MAX_OBSERVERS; i++ ){
		radio->openReadingPipe(i + 1, clients[i].pipeId);
	}

	radio->printDetails();
}

void SimpleNetworkServer::listen(callback callback) {
	// if there is data ready
	if (radio->available()) {
		uint64_t data = 0;
		bool done = false;
		while (!done) {
			// Fetch the payload, and see if this was the last one.
			done = radio->read(&data, sizeof(data));

			byte id = data & ID_MASK;
			clients[id].isConnected = true;

			// tell library user we got some data.
			callback(id, data >> 4);
		}
	}
}
