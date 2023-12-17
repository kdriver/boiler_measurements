#include <Arduino.h>
#ifndef ESP32
#include <ESP8266WiFi.h>

#else
#include <WiFi.h>
#endif
#include <WiFiUdp.h>
#include "UDPLogger.h"


static WiFiUDP Udp;
byte tx_buffer[1000];

unsigned int len;
byte tx[] = {' ','H','E','L','L','O',' '};
String local_ip;
UDPLogger::UDPLogger(IPAddress ip,unsigned short int port)
{
    remote = ip;
    
    Serial.print("Initialise logger to ");
    Serial.print(remote.toString());
    Serial.print(" ");
    Serial.println(port);
    dest_port = port;
}

void UDPLogger::init(String s)
{
    local_ip = WiFi.localIP().toString();
    while ( local_ip.length() < 15)
        local_ip = local_ip + ' ';
    Serial.print("local address is ");
    Serial.println(local_ip);
    String welcome;
    welcome = "HELLO " + s;
    Udp.begin(8787);
    send(welcome);
}

void UDPLogger::send(String s )
{
    String the_line;

    the_line = local_ip + ": " + s;
   /* Serial.print("send to : ");
    Serial.print(remote);
    Serial.print(" ");
    Serial.print(dest_port);
    Serial.print(" ");
    Serial.println(s);
    */
    // the_line.getBytes(tx_buffer,sizeof(tx_buffer));
    //Udp.write(tx_buffer,the_line.length());
    
    Udp.beginPacket(remote,dest_port);
    Udp.write((const u_int8_t *)the_line.c_str(),the_line.length());
    Udp.endPacket();
}