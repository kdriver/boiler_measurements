
//#include <Arduino.h>
#define BATTERY 1
#ifndef ESP32
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#define ONE_WIRE_BUS D2
#define BLUE_LED 2
#else
#define BLUE_LED 2
#include <WiFi.h>
#include <ESPmDNS.h>
//#define ONE_WIRE_BUS GPIO_NUM_4
#define ONE_WIRE_BUS 4
#define DEVICE "ESP32"

#define Influxdb InfluxDBClient
#endif 


#include <OneWire.h>
#include <DallasTemperature.h>
#include "wifi_password.h"
#include "UDPLogger.h"

#include <InfluxDbClient.h>
#define NAME "garage"
Point temperature(NAME);

const char compile_date[] = __DATE__ " " __TIME__;

#define INFLUXDB_HOST "piaware.local"
#define INFLUXDB_PORT "1337"
#define INFLUXDB_DATABASE "boiler_measurements"
 //if used with authentication
#define INFLUXDB_USER 
#define INFLUXDB_PASS

InfluxDBClient *influx;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
WiFiServer server(80);
IPAddress logging_server;

bool reset = false;
unsigned long int reset_time ;

const char* ssid = "cottage"; //your WiFi Name
const char* password = WIFIPASSWORD;  //Your Wifi Password

#ifdef BATTERY
unsigned int analog = A0;
float input_voltage;
float battery_voltage;
float resistor_ratio = 5.7;
#endif
UDPLogger *loggit;

void connect_to_wifi()
{
  WiFi.mode(WIFI_STA);
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

  if (!MDNS.begin(NAME)) {
        Serial.println("Error setting up MDNS responder!");
  }
    else{
      Serial.println("mDNS " + String(NAME) + ".local responder started OK");
    }
#ifdef ESP32
    logging_server = MDNS.queryHost("piaware");
#else
    logging_server =  IPAddress(192,168,0,3);
#endif
    Serial.println(logging_server.toString());
    loggit = new UDPLogger(logging_server.toString().c_str(),(unsigned short int)8788);
    loggit->init(NAME);
}

void setup() {
 
  Serial.begin(9600);
  delay(10); 
  pinMode(ONE_WIRE_BUS,INPUT_PULLUP);
  pinMode(BLUE_LED,OUTPUT);

  unsigned char devs;
  bool devices = oneWire.search(&devs);

  Serial.println("Search I2C devices , found : " + String(devices) + " : number of devs : " + String(devs));
  String influx_url;
  //influx_url = "http://" + logging_server.toString() + ":8086";
  //influx = new InfluxDBClient(influx_url.c_str(),INFLUXDB_DATABASE);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid); 
  
  connect_to_wifi();
  
  influx_url = "http://" + logging_server.toString() + ":8086";
  influx = new InfluxDBClient(influx_url.c_str(),INFLUXDB_DATABASE);

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

void send_measurement(float the_temp)
{
   temperature.clearFields();
  // Report RSSI of currently connected network
  temperature.addField("rssi", WiFi.RSSI());
  temperature.addField("temp",the_temp);
  influx->writePoint(temperature);

  loggit->send(temperature.toLineProtocol() + "\n");
}
void send_measurement_voltage(float battery_v)
{
  temperature.clearFields();


  temperature.addField("voltage",battery_v);
  influx->writePoint(temperature);

  loggit->send(temperature.toLineProtocol() + "\n");
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
  Serial.print(request);
  Serial.print(" from ");
  Serial.println(client.remoteIP().toString());
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); 
  client.println("Temperature is : ");
  client.print(current_temp=getTemperature());
  client.println("<p>WiFi signal strength RSSI ");
  client.println(WiFi.RSSI());
  client.println("</p>") ;
  client.flush(); 
}
void flash_the_LED(void) {
  digitalWrite(BLUE_LED,LOW);
  delay(100);
  digitalWrite(BLUE_LED,HIGH);
}
unsigned long old_time=0;

void loop() {
  unsigned long int the_time;
  float temperatureC;

#ifndef ESP32
  MDNS.update();
#endif
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
  #ifdef BATTERY
    float reading = analogRead(analog)*1.0;
    loggit->send("raw read " + String(reading)+ "\n");
    input_voltage = reading/1023.0 * 3.3;
    battery_voltage = input_voltage * resistor_ratio;
    send_measurement_voltage(battery_voltage);
  #endif
    flash_the_LED();
  }
  if (WiFi.status() != WL_CONNECTED)
  { 
            reset = true;
            reset_time = the_time;
  }   
  if ( reset == true )
  { 
      // wait 5 seconds to restart.
      if (( the_time - reset_time ) > 5000)
            ESP.restart();                        
  }
}
