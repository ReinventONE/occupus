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

}

SimpleNetworkClient::SimpleNetworkClient(
		uint8_t radioPin1,
		uint8_t radioPin2) :
				SimpleNetwork(radioPin1, radioPin2) {
}

SimpleNetworkServer::SimpleNetworkServer(
		uint8_t radioPin1,
		uint8_t radioPin2) :
				SimpleNetwork(radioPin1, radioPin2) {
}
