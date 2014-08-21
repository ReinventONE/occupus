/*
 * HTTPServer.h
 *
 *  Created on: Aug 13, 2014
 *      Author: Konstantin Gredeskoul
 *
 *  (c) 2014 All rights reserved.  Please see LICENSE.
 */

#ifndef HTTPSERVER_H_
#define HTTPSERVER_H_

#define HTTP_PORT 80

#include <Ethernet.h>
#include "ObserverInfo.h"

class HTTPServer {
public:
	HTTPServer(uint8_t *mac);
	void begin();
	void serveJSON(observerInfo observers[], int numObservers);

	EthernetServer *server;
private:
	char _buffer[128];
	uint8_t *_mac;
	char *jsonField(const char *field, int value, bool last);
	char *jsonField(const char *field, const char *value, bool last);
	char *jsonField(const char *field, bool value, bool last);
};

#endif /* HTTPSERVER_H_ */
