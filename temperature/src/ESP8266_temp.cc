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

#define ONE_WIRE_BUS D1
Influxdb influx(INFLUXDB_HOST);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
WiFiServer server(80);

const char* ssid = "cottage"; //your WiFi Name
const char* password = WIFIPASSWORD;  //Your Wifi Password
int adc = A0;
char temperatureString[6];
int interval = 0;

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

  if (!MDNS.begin("dallas")) {
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
  pinMode(adc,INPUT);


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
  interval = 2362100;
  
}

void send_measurement(float value)
{
   InfluxData measurement("temperature");
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

//TMP36 code
float tmp36()                     // run over and over again
{
 //getting the voltage reading from the temperature sensor
 int reading = analogRead(adc);  
 
 // converting that reading to voltage, for 3.3v arduino use 3.3
 float voltage = reading * 3.3;
 voltage /= 1024.0; 
 
 // print out the voltage
 //Serial.print(voltage); Serial.println(" volts");
 
 // now print out the temperature
 float temperatureC = (voltage - 0.5) * 100 ;  //converting from 10 mv per degree wit 500 mV offset
                                               //to degrees ((voltage - 500mV) times 100)
 Serial.print(temperatureC); Serial.println(" degrees C");
 
 // now convert to Fahrenheit
 //float temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;
 //Serial.print(temperatureF); Serial.println(" degrees F");

  return temperatureC;
}

void processWebRequest(WiFiClient client)
{
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); 
  client.println("Temperature is : ");
  client.print(current_temp);
  client.println(""); 
  client.flush(); 
}

void loop() {
  float temperatureC;

  MDNS.update();
  
  WiFiClient client = server.available();
  if (client) {  
    Serial.println("new client ");
    processWebRequest(client);
  }
  interval = interval + 1;

  if ( interval > 2362140 ) // 60 second interval
  {
    //temp();  // read the temp and publish to Influxdb
    temperatureC = getTemperature();
    send_measurement(temperatureC);
    current_temp = temperatureC;
    interval = 0;
  }
 
}
