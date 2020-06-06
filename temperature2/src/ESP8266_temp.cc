#include <InfluxDb.h>
#include <ESP8266WiFi.h> 
#include <OneWire.h>
#include <DallasTemperature.h>
#include "wifi_password.h"
#include "UDPLogger.h"
#include <ESP8266mDNS.h>

const char compile_date[] = __DATE__ " " __TIME__;

#define INFLUXDB_HOST "piaware.local"
#define INFLUXDB_PORT "1337"
#define INFLUXDB_DATABASE "boiler_measurements"
 //if used with authentication
#define INFLUXDB_USER 
#define INFLUXDB_PASS 

#define ONE_WIRE_BUS D2
Influxdb influx(INFLUXDB_HOST);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
WiFiServer server(80);

const char* ssid = "cottage"; //your WiFi Name
const char* password = WIFIPASSWORD;  //Your Wifi Password

UDPLogger loggit("piaware.local",8787);

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

  if (!MDNS.begin("lounge")) {
        Serial.println("Error setting up MDNS responder!");
        loggit.send("failed to start mDNS responder\n");
  }
    else{}
      Serial.println("mDNS dallas.local responder started OK");
      loggit.send("mDNS dallas.local started OK\n");
      
}

void setup() {
  influx.setDb(INFLUXDB_DATABASE);
  Serial.begin(9600);
  delay(10); 

  pinMode(D2,INPUT_PULLUP);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid); 
  connect_to_wifi();
  Serial.println("Built on " + String(compile_date)+ "\n");
  Serial.print("Connected to cottage : IP Address ");

  server.begin();
  Serial.println("Server started");
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/\n"); 
 
  DS18B20.begin();
  
}

void send_measurement(float value)
{
   InfluxData measurement("lounge");
   measurement.addValue("temp",value);

   influx.write(measurement);

   loggit.send(measurement.toString() + "\n");
}

float current_temp = 0.0;

//DS18B20 code
float getTemperature() {
  float temp;
  do {
    DS18B20.requestTemperatures(); 
    temp = DS18B20.getTempCByIndex(0);
    delay(100);
  } while (temp == 85.0 || temp == (-127.0));
  Serial.print(temp); Serial.println(" degrees C");
  return temp;
}

void processWebRequest(WiFiClient client)
{
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); 
  client.println("Temperature is : ");
  client.print(current_temp=getTemperature());
  client.println(""); 
  client.flush(); 
}
unsigned long old_time=0;
void loop() {
  unsigned long int the_time;
  float temperatureC;

  MDNS.update();
  
  WiFiClient client = server.available();
  if (client) {  
    Serial.println("new client ");
    processWebRequest(client);
  }
  the_time = millis();
  if ( (the_time - old_time ) > 60000 ) // 60 second interval
  {
    old_time = the_time;
    temperatureC = getTemperature();
    send_measurement(temperatureC);
    current_temp = temperatureC;
  }
}
