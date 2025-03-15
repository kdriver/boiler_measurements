#include <Arduino.h>
#include <wifi_password.h>


#include <WiFi.h>
#include <ESPmDNS.h>



// these two lines need to be before include StringHandler
enum Command  { Get,Set,Help,Status,Reset};
enum Attribute { OnThreshold,OffThreshold,Debug };

#include <StringHandler.h>
#include <UDPLogger.h>
#include "influx_stuff.h"


String ldr_name = "top";

#define MDNS_NAME ldr_name

IPAddress logging_server;
UDPLogger *loggit;

const char compile_date[] = __DATE__ " " __TIME__;




#define ON_THRESHOLD 400
#define OFF_THRESHOLD 300

int current_level_a0 = 0;
int current_level_a3 = 0;
void setup(void){

  Serial.begin(9600);
  Serial.println("A0,A3");
  Serial.flush();


}

int lasta0 = 0;
int lasta3 = 0;
int peak_a0[5];
int a0_index =0;
int a3_index =0;
int peak_a3[5];
void loop() {

  unsigned long int current_ts;

  delay(5);

  //MDNS.update();

  unsigned int a0pin,a3pin ;
    
  a0pin = analogRead(A0);
  a3pin = analogRead(A3);

  peak_a0[0] = peak_a0[1];
  peak_a0[1] = peak_a0[2];
  peak_a0[2] = a0pin;

  peak_a3[0] = peak_a3[1];
  peak_a3[1] = peak_a3[2];
  peak_a3[2] = a3pin;

  if ((peak_a0[1] - peak_a0[0]) > 5)
    if ((peak_a0[1] - peak_a0[2]) > 5)
    {
      Serial.print("peak A0 : ");
      Serial.print(peak_a0[0]);
      Serial.print(",");
      Serial.print(peak_a0[1]);
      Serial.print(",");
      Serial.println(peak_a0[2]);
    }

  /*if ( abs(current_level_a0 - lasta0 ) > 50 )
  {
      current_ts = millis();
      lasta0 = current_level_a0;
      Serial.println(String(current_level_a0) + " A0 " + String(current_ts));
      //delay(10);
  } 

 if ( abs(current_level_a3 - lasta3 ) > 50 )
  {
      current_ts = millis();
      lasta3 = current_level_a3;
      Serial.println(" A3 " + String(current_level_a3) + " A3 " + String(current_ts));
      //delay(10);
  } */

  // Serial.print(a0pin);  
  // Serial.print(" , ");  
  // Serial.println(a3pin);


  //Serial.println(current_level);



}

