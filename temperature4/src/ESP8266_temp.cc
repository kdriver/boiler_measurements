
#define NAME "study"

//  define battery if this is montoring the 12V battery voltage as well as the temp
// #define BATTERY 1
#define BLUE_LED 2

#if defined(ESP32)
#define ONE_WIRE_BUS 4
#include <WiFiMulti.h>
#include <ESPmDNS.h>

WiFiMulti wifiMulti;
#define DEVICE "ESP32"

#elif defined(ESP8266)

#define ONE_WIRE_BUS D2
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <mDNSResolver.h>
ESP8266WiFiMulti wifiMulti;
WiFiUDP udp;
mDNSResolver::Resolver resolver(udp);

#define DEVICE "ESP8266"
#endif


#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "wifi_password.h"
#include "UDPLogger.h"
#include <InfluxDbClient.h>
#include <ArduinoJson.h>

String CONFIG_HOST = "http://piaware.local:9090";
String CONFIG_FILE = "/sensors.json";
HTTPClient http;

bool deep_sleep = true;
#define US 1000000
unsigned int sleep_for = 300 * US; // 300 seconds - 5 minutes.

Point temperature(NAME);

const char compile_date[] = __DATE__ " " __TIME__;

#include "influx_stuff.h"

InfluxDBClient *influx;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
IPAddress logging_server;

bool reset = false;
unsigned long int reset_time;

const char *ssid = "cottage";        // your WiFi Name
const char *password = WIFIPASSWORD; // Your Wifi Password

UDPLogger *loggit;

// to measure the 12V battery voltage
unsigned int analog = A0;
float input_voltage;
float battery_voltage;
float resistor_ratio = 5.7;

void connect_to_wifi()
{
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(ssid, password);
  // WiFi.begin(ssid, password);
  int counter = 0;
  while (wifiMulti.run() != WL_CONNECTED)
  {
    Serial.println(".");
    counter = counter + 1;
    /*
    if (counter > 20)
    {
      counter = 0;
      WiFi.disconnect();
      WiFi.begin(ssid, password);
      Serial.println("\nretry Wifi\n");
    }
    */
    delay(500);
  }
  delay(10);

  if (!MDNS.begin(NAME))
    Serial.println("Error setting up MDNS responder!");
  else
    Serial.println("mDNS responder started");

  delay(10);
  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);

  delay(10);
#ifdef ESP32
  logging_server = MDNS.queryHost("piaware");
#else
  logging_server = resolver.search("piaware.local");
#endif

  Serial.println(logging_server.toString());
  // logging_server =  IPAddress(192,168,0,3);
  loggit = new UDPLogger(logging_server, (unsigned short int)8788);
  loggit->init(NAME);
}

void send_measurement(float value)
{
  temperature.clearFields();
  // Report RSSI of currently connected network
  temperature.addField("rssi", WiFi.RSSI());
  temperature.addField("temp", value);
  bool answer = influx->writePoint(temperature);

  if (!answer)
  {
    Serial.print(influx->getLastErrorMessage());
    Serial.print("Failed to send temp to influx");
    loggit->send("error sending temp to influx");
  }
  else
    loggit->send(temperature.toLineProtocol());
}

void send_measurement_voltage(float battery_v)
{
  temperature.clearFields();

  temperature.addField("voltage", battery_v);
  influx->writePoint(temperature);

  loggit->send(temperature.toLineProtocol());
}

float current_temp = 0.0;

// DS18B20 code
float getTemperature()
{
  float temp;
  int counter = 0;

  do
  {
    DS18B20.requestTemperatures();
    temp = DS18B20.getTempCByIndex(0);
    delay(100);
    counter = counter + 1;
  } while (temp == 85.0 || temp == (-127.0));
  Serial.print(temp);
  Serial.println(" degrees C");
  return temp;
}

void flash_the_LED()
{
  digitalWrite(BLUE_LED, LOW);
  delay(100);
  digitalWrite(BLUE_LED, HIGH);
  delay(100);
  digitalWrite(BLUE_LED, LOW);
}

void get_configuration(void)
{
  WiFiClient client;
  http.begin(client, CONFIG_HOST + CONFIG_FILE); // HTTP
  int httpCode = http.GET();
  // httpCode will be negative on error
  if (httpCode > 0)
  {
    // file found at server
    if (httpCode == HTTP_CODE_OK)
    {
      String payload = http.getString();
      // Serial.println(payload);
      StaticJsonDocument<1000> buffer;

      DeserializationError error = deserializeJson(buffer, payload);
      if (error)
      {
        Serial.println(F("Failed to read file, using default configuration"));
        Serial.println(error.c_str());
        return;
      }
      // else
      //   Serial.println("decoded ok");

      JsonArray sensors_list = buffer["sensors"].as<JsonArray>();
     
      char the_text_buffer[500];

      // const char * name = buffer["sensors"][0]["name"];
      // Serial.println("First name is");
      // Serial.println(name);
      // Serial.print("list size ");
      bool found = false;
      for (JsonObject sensor : sensors_list)
      {
        const char *n;
        n = sensor["name"];
        // Serial.println(n);
        if (strcmp(n, NAME) == 0)
        {
          found = true;
          deep_sleep = strcmp(sensor["sleep"], "true") == 0;
          sleep_for = ((int)sensor["interval"]) * US;
          sprintf(the_text_buffer, "Match! Found my config for %s Deep sleep = %s for %d seconds", NAME, deep_sleep ? "true" : "false", sleep_for / US);
          // Serial.print(the_text_buffer);
          loggit->send(the_text_buffer);
        }
      }

      if (found == false)
      {
        deep_sleep = true;
        sleep_for = 300 * US; // 5mins
        loggit->send("No config found - defaults used");
      }
      
      influx_host = buffer["influx_config"]["host"];
      influx_token = buffer["influx_config"]["token"];
      influx_orgid = buffer["influx_config"]["organisationID"];
      influx_org = buffer["influx_config"]["organisation"];
      influx_bucket = buffer["influx_config"]["bucket"];

      Serial.println("Influx Config:");
      Serial.print(" host is         : ");
      Serial.println(influx_host);
      Serial.print(" token is        : ");
      Serial.println(influx_token);
      Serial.print(" orgID is        : ");
      Serial.println(influx_orgid);
      Serial.print(" organisation is : ");
      Serial.println(influx_org);
      Serial.print(" bucket is       : ");
      Serial.println(influx_bucket);
    }
    else
    {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    }
  }
  else
  {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}
void setup()
{

  Serial.begin(9600);
  delay(10);
  pinMode(ONE_WIRE_BUS, INPUT_PULLUP);
  pinMode(BLUE_LED, OUTPUT);

  String influx_url;


  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  connect_to_wifi();

  get_configuration();

  influx = new InfluxDBClient(influx_host, influx_orgid, influx_bucket, influx_token);

  Serial.println("Built on " + String(compile_date));
  Serial.print("InfluxDB URL ");
  Serial.println(influx_host);

  if (influx->validateConnection())
  {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(influx->getServerUrl());
  }
  else
  {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(influx->getLastErrorMessage());
  }

  DS18B20.begin();

  Serial.println("init the dallas temp device");

  float temperatureC = getTemperature();
  send_measurement(temperatureC);

#ifdef BATTERY
  float reading = analogRead(analog) * 1.0;
  loggit->send("raw read " + String(reading) + "\n");
  input_voltage = reading / 1023.0 * 3.3;
  battery_voltage = input_voltage * resistor_ratio;
  send_measurement_voltage(battery_voltage);
#endif

  flash_the_LED();
  // end of code
}

unsigned long old_time = 0;
void loop()
{
  if (deep_sleep)
  {
#ifdef ESP32
    esp_sleep_enable_timer_wakeup(sleep_for);
    Serial.flush();
    esp_deep_sleep_start();
#else
    ESP.deepSleep(sleep_for);
#endif
  }
  else
  {
#ifndef ESP32
    MDNS.update();
#endif
    unsigned long now = millis();
    if ((now - old_time) > 10000)
    {
#ifndef ESP32
      MDNS.announce();
#endif
      old_time = now;
      flash_the_LED();
      float temperatureC = getTemperature();
      send_measurement(temperatureC);
    }
  }
}
