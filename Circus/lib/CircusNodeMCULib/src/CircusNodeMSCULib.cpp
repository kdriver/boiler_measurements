/*
  	CircusNodeMCULib.cpp  (Version 1.0.0)

	Implements the circusofthings.com API in NodeMCU boards.

  	Created by Jaume Miralles Isern, June 19, 2019.
*/


#include "CircusNodeMCULib.h"

CircusNodeMCULib::CircusNodeMCULib(char *server, char *ssid, char *pass)
{
	_server = server; 
        _ssid = ssid;
        _pass = pass;
    
}

void CircusNodeMCULib::begin(){
	WiFi.begin(_ssid, _pass); // Connect to your Wifi with the paremeters you provided at the begining
    	while (WiFi.status() != WL_CONNECTED) {
		delay(1000);        
    	}
    	if ((WiFi.status() == WL_CONNECTED)) {
        	Serial.println("Connected to WiFi");
        	getCertificate(); // Don't bother for SSL certificates. This method will fetch it for you
    	}
}

char *x;

void CircusNodeMCULib::getCertificate() {
	WiFiClient client;
	const char* host = "circusofthings.com"; 
	const int httpPort = 8021;
	if (!client.connect(host, httpPort)) {
	    Serial.println("connection failed");
	    return;
  	}
	client.println("GET /rootCA.txt HTTP/1.1");
	client.println("Host: www.circusofthings.com");
	client.println("User-Agent: CircusNodeMCULib-1.0.0");
	client.println("Connection: close");
	Serial.print("Waiting for response ");
	waitCertificate(5000,&client);
}

void CircusNodeMCULib::write(char *key, double value, char *token) {
	WiFiClientSecure client;
  	client.setInsecure();
  	client.setCACert(_charBuf_rootCA,1250);
    	Serial.println("SSL connection attempt");
    	if (!client.connect(_server, 443)){
		Serial.println("Connection failed!");
		begin();
    	} else {
        	Serial.println("Connected to server!");
        	char bufValue[15]; 
        	dtostrf(value,1,4,bufValue);
		char body[150];
        	for( int i = 0; i < sizeof(body);  ++i )
                	body[i] = (char)0;
        	strcat(body, "{\"Key\":\""); strcat(body, key);
        	strcat(body, "\",\"Value\":"); strcat(body, bufValue);
        	strcat(body, ",\"Token\":\""); strcat(body, token); strcat(body, "\"}");
        	String strBody(body);
        	client.println("PUT /WriteValue HTTP/1.0");
        	client.println("Host: www.circusofthings.com");
        	client.println("User-Agent: CircusNodeMCULib-1.0.0");
        	client.println("Content-Type:application/json");
        	client.print("Content-Length: ");
        	client.println(strBody.length());
        	client.println();
        	client.println(body);

		Serial.println("[Circus]");
		while (client.available()) {
		    	char c = client.read();
		    	Serial.write(c);
		}
		if (!client.connected()) {
		    	Serial.println();
		    	Serial.println("Server disconnected");
		    	client.stop();
		}
    	}
}

double CircusNodeMCULib::read(char *key, char *token) {
	WiFiClientSecure client;
	client.setInsecure();
  	client.setCACert(_charBuf_rootCA,1250);
    	Serial.println("SSL connection attempt");
    	if (!client.connect(_server, 443)){
		Serial.println("Connection failed!");
    	} else {
        	Serial.println("Connected to server!");
		char q[250]; // nou
    		sprintf_P(q, PSTR("GET /ReadValue?Key=%s&Token=%s HTTP/1.1"), key, token); // nou
		client.println(q); // nou
        	client.println("Host: www.circusofthings.com");
        	client.println("User-Agent: CircusNodeMCULib-1.0.0");
        	client.println("Content-Type:application/json");
        	client.println("Connection: close");
        	client.println();

		Serial.println("Waiting for response ");
		while (!client.available()){
		    	delay(50); //
		    	Serial.print(".");
		}  
		if (!client.connected()) {
		    	Serial.println();
		    	Serial.println("Server disconnected");
		    	client.stop();
		}
		
		char *responsebody = waitResponse(5000,&client);
		if (responsebody!=(char)0) {
			char labelk[] = "Key";
			char *key = parseServerResponse(responsebody, labelk, 3);
			char labelm[] = "Message";
			char *message = parseServerResponse(responsebody, labelm, 3);
			char labelv[] = "\"Value";// Double quote because can be confused with LastValueIp
			char *value = parseServerResponse(responsebody, labelv, 2); 
			return(atof(value));
	    	} else {
			Serial.println("No connection");
	    	}
    	}
}

char* CircusNodeMCULib::waitResponse(int timeout, WiFiClientSecure *client) {
	static char responsebody[200];
	for( int i = 0; i < sizeof(responsebody);  ++i )
        	responsebody[i] = (char)0;
	int j = 0;
	int pick = 0;
    	while (client->available()) {
                char c = client->read();
		Serial.write(c);
                if (c=='{') {pick=1;}
                if(pick){responsebody[j]=c;j++;}
                if (c=='}') {responsebody[j]='\0';pick=0;}                        
	}
	Serial.println("");
    	return responsebody;
}

void CircusNodeMCULib::waitCertificate(int timeout, WiFiClient *client) {
	static char responsebody[100];
	for( int i = 0; i < sizeof(_charBuf_rootCA);  ++i )
        	_charBuf_rootCA[i] = (char)0;
	int j = 0;
	int pick = 0;
    	while (client->available()) {
                char c = client->read();
		Serial.write(c);
                _charBuf_rootCA[j]=c;j++;                     
	}
	Serial.println("");
}

char* CircusNodeMCULib::parseServerResponse(char *r, char *label, int offset) {
    int labelsize = count(label);
    char *ini = strstr(r,label) + labelsize + offset;
    static char content[100];
    int i=0;
    while (*ini!='\0'){ // may truncated occur
        if((*ini!='\"')&&(*ini!=','))
            content[i]=*ini;
        else {
            content[i]='\0';
            return content;
        }
        i++; ini++;
    }
	
    return (char)0;
}

int CircusNodeMCULib::count(char *text) {
    for(int i=0;i<300;i++){
        if(text[i]=='\0'){
            return(i);
        }
    }
    return -1;
}


