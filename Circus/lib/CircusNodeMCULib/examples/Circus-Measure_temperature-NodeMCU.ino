
/*
  Circus-Measure_temperature-NodeMCU.ino

  This example code shows how to control a fan depending on a signal at circusofthings.com API through its CircusNodeMCULib-1.0.0 library for Arduino IDE.

  Created by Jaume Miralles Isern, June 21, 2019.
*/ 

#include <CircusNodeMCULib.h>
#include <DHT.h>

// ------------------------------------------------
// These are the CircusNodeMCULib related declarations
// ------------------------------------------------

char ssid[] = "your_SSID_here"; // Place your wifi SSID here
char password[] =  "your_password_here"; // Place your wifi password here
char token[] = "your_token_here"; // Place your token, find it in 'account' at Circus. It will identify you.
char server[] = "www.circusofthings.com";
char temperature_key[] = "12286";  // Place the Key of the signal you created at Circus Of Things for the Temperature
char humidity_key[] = "14569";  // Place the Key of the signal you created at Circus Of Things for the Humidity
CircusNodeMCULib circusNodeMCU(server,ssid,password); // The object representing an NodeMCU to whom you can order to Write or Read


// ------------------------------------------------
// These are the Temperature Measure Example related declarations
// ------------------------------------------------

#define DHTPIN 2      // digital of your NodeMCU connected to DHT11 (D4, that is the GPIO2 in the pin out)
#define DHTTYPE DHT11 // exact model of temperature sensor DHT 11 for the general library
DHT dht(DHTPIN, DHTTYPE); // The object representing your DHT11 sensor



void setup() {
    Serial.begin(115200); // Remember to match this value with the baud rate in your console
    dht.begin(); // Let the DHT object be initialized
    circusNodeMCU.begin(); // Let the Circus object set up itself for an SSL/Secure connection
}

void loop() {
    delay(5000);
    // Read the Temperature that DHT11 sensor is measuring.
    float t = dht.readTemperature(); 
    if (isnan(t))
        t=-1; // if so, check the connection of your DHT11 sensor... something is disconnected ;-)
    // Read the Humidity that DHT11 sensor is measuring.
    float h = dht.readHumidity();
    if (isnan(h))
        h=-1; // if so, check the connection of your DHT11 sensor... something is disconnected ;-)
    // Show values, just for debuging
    Serial.println(""); Serial.print("Temperature: "); Serial.println(t); Serial.print("Humidity: "); Serial.println(h);

    // Report the values gathered by the sensor to the Circus
    circusNodeMCU.write(temperature_key,t,token); // Report the temperature measured to Circus.
    circusNodeMCU.write(humidity_key,h,token); // Report the humidity measured to Circus.
}
