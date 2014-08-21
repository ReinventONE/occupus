/*
 * HTTPServer.cpp
 *
 *  Created on: Aug 13, 2014
 *      Author: Konstantin Gredeskoul
 *
 *  (c) 2014 All rights reserved.  Please see LICENSE.
 */

#include "HTTPServer.h"

HTTPServer::HTTPServer(uint8_t *mac) {
	server = new EthernetServer(HTTP_PORT);
	_mac = mac;
}

void HTTPServer::begin() {
	// start the Ethernet connection and the server:
	Ethernet.begin(_mac);
	server->begin();
	printf("\n\nStarting Web Server, connect to http://");
	Serial.println(Ethernet.localIP());
}

void HTTPServer::serveJSON(observerInfo observers[], int numObservers) {
	// listen for incoming clients
	EthernetClient client = server->available();
	if (client) {
		boolean currentLineIsBlank = true;
		while (client.connected()) {
			if (client.available()) {
				char c = client.read();
				Serial.write(c);
				// if you've gotten to the end of the line (received a newline
				// character) and the line is blank, the http request has ended,
				// so you can send a reply
				if (c == '\n' && currentLineIsBlank) {
					// send a standard http response header
					client.println("HTTP/1.1 200 OK");
					client.println("Content-Type: application/json");
					client.println("Accept: application/json");
					client.println("Connection: close"); // the connection will be closed after completion of the response
					client.println("Refresh: 3"); // refresh the page automatically every 5 sec
					client.println();
					client.println("[{");
					for (int i = 0; i < numObservers; i++) {
						client.print(jsonField("id", i + 1, false));
						client.print(jsonField("name", observers[i].name, false));
						client.print(jsonField("secondsSinceLastChange", (int)((millis() - observers[i].lastStateChangeAt) / 1000), false));
						client.print(jsonField("connected", observers[i].connected, !observers[i].connected));
						if (observers[i].connected) {
							client.print(jsonField("occupied", observers[i].status, true));
						}
						if (i < (numObservers - 1)) {
							client.println("}, {");
						}
					}
					client.println("}]");
					break;
				}
				if (c == '\n') {
					// you're starting a new line
					currentLineIsBlank = true;
				} else if (c != '\r') {
					// you've gotten a character on the current line
					currentLineIsBlank = false;
				}
			}
		}
		// give the web browser time to receive the data
		delay(1);
		// close the connection:
		client.stop();
	}
}


char *HTTPServer::jsonField(const char *field, int value, bool last) {
	sprintf(_buffer, "\t\"%s\": %d%s\n", field, value, last ? "" : ",");
	return _buffer;
}

char *HTTPServer::jsonField(const char *field, const char *value, bool last) {
	sprintf(_buffer, "\t\"%s\": \"%s\"%s\n", field, value, last ? "" : ",");
	return _buffer;
}

char *HTTPServer::jsonField(const char *field, bool value, bool last) {
	sprintf(_buffer, "\t\"%s\": %s%s\n", field, (value ? "true":"false"), (last ? "" : ","));
	return _buffer;
}
