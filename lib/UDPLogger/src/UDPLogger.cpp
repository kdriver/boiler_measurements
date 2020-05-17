#include <Arduino.h>
#ifndef ESP32
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <WiFiUDP.h>
#include "UDPLogger.h"


WiFiUDP Udp;
byte tx_buffer[1000];

unsigned int len;
byte tx[] = {'H','E','L','L','O','\n'};
String local_ip;
UDPLogger::UDPLogger(const char *ip,unsigned short int port)
{
    strcpy(remote,ip);
    dest_port = port;
}
void UDPLogger::init(void)
{
    Udp.begin(8787);
    Udp.beginPacket(remote,dest_port);
    Udp.write(tx,sizeof(tx));
    Udp.endPacket();
    local_ip = WiFi.localIP().toString();
}

void UDPLogger::send(String s )
{
    String the_line;

    the_line = local_ip + " : " + s;
    Udp.beginPacket(remote,dest_port);
    the_line.getBytes(tx_buffer,sizeof(tx_buffer));
    Udp.write(tx_buffer,the_line.length());
    Udp.endPacket();
}