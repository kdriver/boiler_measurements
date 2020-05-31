
/*
  Circus-Control_fan_speed-NodeMCU.ino

  This example code shows how to control a fan depending on a signal at circusofthings.com API through its CircusNodeMCULib-1.0.0 library for Arduino IDE.

  Created by Jaume Miralles Isern, June 19, 2019.
*/ 

#include <CircusNodeMCULib.h>

// ------------------------------------------------
// These are the CircusNodeMCULib related declarations
// ------------------------------------------------

char ssid[] = "your_SSID_here"; // Place your wifi SSID here
char password[] =  "your_wifi_password_here"; // Place your wifi password here
char token[] = "your_circus_token_here"; // Place your token, find it in 'account' at Circus. It will identify you.
char server[] = "www.circusofthings.com";
char fanSpeed_key[] = "6276"; // Type the Key of the Circus Signal you want the NodeMCU listen to. 
CircusNodeMCULib circusNodeMCU(server,ssid,password); // The object representing an NodeMCU to whom you can order to Write or Read

// ------------------------------------------------
// These are the Fan Speed Example related declarations
// ------------------------------------------------

// Fan control pins connected to driver
int FanIN3 = 12; // pin 12 (D6) will be the NodeMCU output connected to the IN3 driver input
int FanIN4 = 13; // pin 13 (D7) will be the NodeMCU output connected to the IN4 driver input 
int FanENB = 15; // pin 15 (D8) will be the NodeMCU output, providing PWM, connected to the ENB driver input
// PWM properties in NodeMCU:
// default frequency is 1000Hz
// resolution is 10 bit, then the range for duty cicle is from 0 (0%) to 1024 (100%)
int dutyCycle = 0; // let's start with the fan stopped

void setup() {
  // Enable console for debug
  Serial.begin(115200);
  // Let the Circus object set up itself for an SSL/Secure connection
  circusNodeMCU.begin(); 
  // set pins as outputs
  pinMode(FanIN3, OUTPUT);
  pinMode(FanIN4, OUTPUT);
  pinMode(FanENB, OUTPUT);
  digitalWrite(FanIN4, LOW);
  digitalWrite(FanIN3, HIGH);
}

void loop() {
  // This will read the value of my signal at Circus. I'm supposed to set it between 0 and 1024.
  double fanSpeed_setPoint = circusNodeMCU.read(fanSpeed_key,token); 
  // As I defined at Circus the range (0-1024) as the duty cycle of the NodeMCU (0-1024), there's no need to make aditional conversions.
  // If you want to set different ranges, use the line code below to make the proper linear conversion
  dutyCycle = fanSpeed_setPoint;
  // Set a new PWM duty cycle in output, thus a new fan speed
  analogWrite(FanENB,dutyCycle);
  // Some debug in console just to check
  Serial.print("Fan duty cycle (0-1024) set to: ");
  Serial.println(dutyCycle);
  delay(3000);
}
