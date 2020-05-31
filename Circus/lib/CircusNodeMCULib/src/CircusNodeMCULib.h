/*
  	CircusNodeMCULib.h  (Version 1.0.0)

	Implements the circusofthings.com API in NodeMCU boards.

  	Created by Jaume Miralles Isern, June 19, 2019.
*/


#ifndef CircusNodeMCULib_h
#define CircusNodeMCULib_h

#include "Arduino.h"
//#include "WiFi.h"
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

 
class CircusNodeMCULib
{
  	public:
		CircusNodeMCULib(char *server, char *ssid, char *pass);
		void begin();
		void write(char *key, double value, char *token);
                double read(char *key, char *token);
	private:
		void getCertificate();
		char* parseServerResponse(char *r, char *label, int offset);
                char* waitResponse(int timeout, WiFiClientSecure *client);
		void waitCertificate(int timeout, WiFiClient *client);
		int count(char *text);
		unsigned char _charBuf_rootCA[1250];
		char *_server;
		char *_ssid;
                char *_pass;
};

#endif
