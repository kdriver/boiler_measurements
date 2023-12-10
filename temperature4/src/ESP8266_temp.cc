
//#include <Arduino.h>
//  define battery if this is montoring the 12V battery voltage as well as the temp
//#define BATTERY 1
#define BLUE_LED 2

#ifndef ESP32
// ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#define ONE_WIRE_BUS D2
#else
//ESP32
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

#define NAME "freezer"
#define US_5_MINUTES 300*1000000

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
IPAddress logging_server;

bool reset = false;
unsigned long int reset_time ;

const char* ssid = "cottage"; //your WiFi Name
const char* password = WIFIPASSWORD;  //Your Wifi Password

UDPLogger *loggit;

int loop_counter = 2;


// to measure the 12V battery voltage
unsigned int analog = A0;
float input_voltage;
float battery_voltage;
float resistor_ratio = 5.7;


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
    delay(10);
#ifdef ESP32
   if (!MDNS.begin(NAME)) {
        Serial.println("Error setting up MDNS responder!");
    }
    else
      Serial.println("mDNS responder started");

    // Start TCP (HTTP) server
    Serial.println("TCP server started");
    delay(10);
    // Add service to MDNS-SD
    MDNS.addService("http", "tcp", 80);

    delay(10);
    logging_server = MDNS.queryHost("piaware");
#else
    logging_server =  IPAddress(192,168,0,3);
#endif
  
    loggit = new UDPLogger(logging_server.toString().c_str(),(unsigned short int)8788);
    loggit->init(NAME);
}



void send_measurement(float value)
{
   temperature.clearFields();
  // Report RSSI of currently connected network
  temperature.addField("rssi", WiFi.RSSI());
  temperature.addField("temp",value);
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
  int counter=0;

  do {
    DS18B20.requestTemperatures(); 
    temp = DS18B20.getTempCByIndex(0);
    delay(100);
    counter = counter + 1;
  } while (temp == 85.0 || temp == (-127.0));
  Serial.print(temp); 
  Serial.println(" degrees C");
  return temp;
}

void flash_the_LED() {
  digitalWrite(BLUE_LED,LOW);
  delay(100);
  digitalWrite(BLUE_LED,HIGH);
}

void setup() {
 
  Serial.begin(9600);
  delay(10); 
  pinMode(ONE_WIRE_BUS,INPUT_PULLUP);
  pinMode(BLUE_LED,OUTPUT);

//  unsigned char devs;
//  bool devices = oneWire.search(&devs);

//  Serial.println("Search I2C devices , found : " + String(devices) + " : number of devs : " + String(devs));

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

 
  DS18B20.begin();

  float temperatureC = getTemperature();
  send_measurement(temperatureC);

#ifdef BATTERY
    float reading = analogRead(analog)*1.0;
    loggit->send("raw read " + String(reading)+ "\n");
    input_voltage = reading/1023.0 * 3.3;
    battery_voltage = input_voltage * resistor_ratio;
    send_measurement_voltage(battery_voltage);
#endif

  flash_the_LED();
  // end of code
 
}
  

unsigned long old_time=0;
void loop() {
#ifdef ESP32
    esp_sleep_enable_timer_wakeup(US_5_MINUTES);
    Serial.flush();
    esp_deep_sleep_start();
#else
    ESP.deepSleep(US_5_MINUTES);
#endif
}
