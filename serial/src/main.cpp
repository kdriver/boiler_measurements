

#include <OneWire.h>
#include <DallasTemperature.h>

const byte DS18B20_PIN = 4; //define sensor data pin
OneWire ds(DS18B20_PIN); // DS18B20 on pin 2
DallasTemperature sensor(&ds);

byte data[12]; // buffer for data
byte address[8]; // 64 bit device address

void setup(void)
{
    pinMode(4,INPUT_PULLUP);
  Serial.begin(9600);
  if (ds.search(address)) 
  {
    Serial.println("Slave device found!");
  }
  else 
  {
    Serial.println("Slave device not found;!");
  }
  sensor.begin();
}
void loop()
{
    delay(2000);
    sensor.requestTemperatures();
    Serial.print("Temperature: ");
    Serial.print(sensor.getTempCByIndex(0));
    Serial.print((char)176);//shows degrees character
    Serial.println("C  |  ");
}

