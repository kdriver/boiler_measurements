
#include <Arduino.h>
#include <ESP8266WiFi.h> 
#include <ESP8266mDNS.h>
#include <CircusNodeMCULib.h>
#include "wifi_password.h"

const char compile_date[] = __DATE__ " " __TIME__;

const char token[] = "eyJhbGciOiJIUzI1NiJ9.eyJqdGkiOiIxNDA2In0.KGxtn9QBdhYvsH7KLk8ph7u3zf_pgxbZj8i6NTL1JA8";
const char key[] = "30356";

int TXPinForWifiModule = 3;               // IO port in your arduino you will use as TX for serial communication with the wifi module
int RXPinForWifiModule = 2;               // IO port in your arduino you will use as RX for serial communication with the wifi module
SoftwareSerial ss(TXPinForWifiModule,RXPinForWifiModule);
CircusWifiLib circus(Serial,&ss,SSID,WIFI_PASSWORD,DEBUG_DEEP,KEEP_ALIVE);

void connect_to_wifi()
{
  WiFi.begin(ssid, password);
  int counter=0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    counter=counter+1;
    if ( counter > 20 )
    {
      counter = 0 ;
      WiFi.disconnect();
      WiFi.begin(ssid, password);
      Serial.println("\nretry Wifi\n");
    }
  }

loggit.init();

  if (!MDNS.begin("dallas")) {
        Serial.println("Error setting up MDNS responder!");
        loggit.send("failed to start mDNS responder\n");
  }
    else{}
      Serial.println("mDNS dallas.local responder started OK");
      loggit.send("mDNS dallas.local started OK\n");
      
}
void setup() {
  Serial.begin(9600);
  ss.begin(wifiSerialBaudRate);
  circus.begin();

  delay(10); 
  pinMode(adc,INPUT);
  Serial.println("Built on " + String(compile_date)+ "\n");
  
  
}
void loop()
{
    delay(5000);
     circus.write(key,50,token); 
}