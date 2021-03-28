#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Button2.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <wifi_password.h>
#include <Stringhandler.h>
#include <UDPLogger.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <History.h>



#define MDNS_NAME "ttgo"

// these two lines need to be before include StringHandler
enum Command  { Get,Set,Help,Status,Reset};
enum Attribute { OnThreshold,Debug };
CommandSet ttgo_commands[] = {{"GET",Get,2},{"SET",Set,4} ,{"HELP",Help,1},{"STATUS",Status,1},{"RESET",Reset,1}};
AttributeSet ttgo_attributes[] = {
     {"DEBUG",Debug}
     };

bool reset = false;
bool DEBUG_ON  = false;
IPAddress logging_server;
UDPLogger *loggit;
const char compile_date[] = __DATE__ " " __TIME__;
WiFiUDP  control;

History temps,pressures,winds;
float current_temp,current_wind;
int current_pressure;
unsigned long reset_time;

TFT_eSPI tft = TFT_eSPI();   
#define RED        0xF800
#define WHITE      0xFFFF

void connect_to_wifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(MYSSID, WIFIPASSWORD);
  int counter=0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    counter=counter+1;
    if ( counter > 20 )
    {
      counter = 0 ;
      WiFi.disconnect();
      WiFi.begin(MYSSID, WIFIPASSWORD);
      Serial.println("\nretry Wifi\n");
    }
  }
  if (!MDNS.begin(MDNS_NAME)) {
        Serial.println("Error setting up MDNS responder!");
    }
    else
      Serial.println("mDNS responder started");
}
String command(String command)
{
  command.toUpperCase();
  StringHandler sh(command.c_str(),sizeof(ttgo_commands)/sizeof(CommandSet),ttgo_commands,sizeof(ttgo_attributes)/sizeof(AttributeSet),ttgo_attributes);
  int toks;
  toks = sh.tokenise();

  if ( toks == 0 )
    return String("no command");

  if ( sh.validate() == false )
  {
    Serial.println("Invalid command with " + String(toks) + "tokens  : " + command);
    return String("Invalid command ") + command;
  }
 
  String answer;
  switch ( sh.get_command() )
  {
    case Get:
      {
        switch(sh.get_attribute())
        {
           case Debug:
            answer = "DEBUG_ON : " + String(DEBUG_ON);
          break;
          default:
            answer = "Unknown attribute " + String(sh.get_token(2));
          break;
        }
      }
    break;
    case Set:
    unsigned int value ;
      value = sh.get_value();
      switch(sh.get_attribute())
          {
               case Debug:
              if ( value == 0 )
                DEBUG_ON = false;
              else
                DEBUG_ON = true;
            break;
            default:
              answer = "Unknown attribute " + String(sh.get_token(2));
            break;
          }
    break;
    case Reset:
        answer = "Resetting in 5 seconds ";
        reset = true;
        reset_time = millis() + (5*1000);
    break;
    case Status:

      answer = "\nweather stats\n";
      answer = answer + "temp ave    : " + String(temps.average())     +  " now " + String(current_temp) + "\n";
      answer = answer + "pessure ave : " + String(pressures.average()) +  " now " + String(current_pressure) + "\n";
      answer = answer + "wind ave    : " + String(winds.average())     +  " now " + String(current_wind)  + "\n";
      answer = answer + "samples     : " + String(winds.num_entries()) + "\n";
  
    break;
    case Help:
    answer = "Commands ( ";
      for ( unsigned int i = 0 ; i < sizeof(ttgo_commands)/sizeof(CommandSet); i++ )
      {
        if ( i )
          answer = answer + " | " + ttgo_commands[i].cmd ;
        else
          answer = answer + ttgo_commands[i].cmd ;
      }
      answer = answer + " ) , Attributes ( ";
      for ( unsigned int i = 0 ; i < sizeof(ttgo_attributes)/sizeof(AttributeSet); i++ )
      {
        if ( i )
          answer = answer + " | " + ttgo_attributes[i].attr ;
        else
          answer = answer + ttgo_attributes[i].attr  ;
      }
      answer = answer + " ) \n";
        
    break;
  }
  return answer;
  
   
}
void handleUDPPackets(void) {
  char packet[500];
  int cb = control.parsePacket();
  String text;
  if (cb) {
    int length;
    length  = control.read(packet, sizeof(packet));
    String myData = "";
    for(int i = 0; i < length; i++) {
      myData += (char)packet[i];
    }
    Serial.println(myData);
    text = command(myData);
   
    control.beginPacket(control.remoteIP(),control.remotePort());
    String answer = String(MDNS_NAME) + " > " + text + "\n";
    control.print(answer );
    control.endPacket();

  }
}

unsigned int colour = WHITE;
int xpos=65,ypos=65;
int yinc = 1;
int xinc = 1;
#define RADIUS 10

#define BUTTON1 0
#define BUTTON2 35
Button2 *reverse_direction = new Button2(BUTTON1);
Button2 *change_colour = new Button2(BUTTON2);
HTTPClient http;

unsigned long plane_time;
unsigned long weather_time;
unsigned int last_secs = 0;

bool clear_the_screen = false;
bool  first=false;
#define MINUTES(M) (60*1000*M)
unsigned long full_time = MINUTES(5);
int how_long_left = 0;

enum orientation { LANDSCAPE, PORTRAIT };

void reverse_clicked(Button2 & b )
{
    loggit->send("Button1 clicked\n");
    yinc = yinc * -1;
    xinc = xinc * -1;
    weather_time = 0;
    first=true;
}
void change_colour_clicked(Button2 & b )
{
    loggit->send("Button2 clicked\n");
    colour = random(0x00,0xFFFF);
    weather_time = 0;
    first=true;
}
void button_loop()
{
    reverse_direction->loop();
    change_colour->loop();
}

void setup()
{
    unsigned long the_time=millis();
    orientation orient = LANDSCAPE;
    weather_time = the_time;
    plane_time = the_time;
    Serial.begin(9600);
    Serial.println("Running!");

    reverse_direction->setClickHandler(reverse_clicked);
    change_colour->setClickHandler(change_colour_clicked);
    
    connect_to_wifi();

    logging_server = MDNS.queryHost("piaware");
    Serial.println(logging_server.toString());
    loggit = new UDPLogger(logging_server.toString().c_str(),(unsigned short int)LOGGIT_PORT);
    loggit->init();

    tft.init();
    tft.setRotation(1); // Landscape orientation, USB at bottom right
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED,TFT_BLACK);
    tft.setCursor(0,50);
    tft.setTextSize(3);
    tft.setTextFont(1);
    tft.println("Initialising\n");
    tft.setTextSize(2);
    tft.setTextColor(TFT_OLIVE,TFT_BLACK);
    tft.println("Build date:");
    tft.println(compile_date);
    Serial.println("Setup done");
    clear_the_screen = true;

    int udp = control.begin(8788);
    Serial.println("start UDP server on port 8788 " + String(udp)); 
}



unsigned int dx=240;
unsigned int dy=135;

void write_direction(float now, float ave, int where ){
    tft.setCursor(dx - 25,50+(tft.fontHeight(1)*where));
    if ( now > ave)
        tft.print('U');
    else if ( now == ave)
            tft.print('=');
        else
            tft.print('D');
}

void loop(){
    String json_text;
    unsigned long int time_now = millis();
    button_loop();
    handleUDPPackets();

    if ( reset == true )
    {
        if ( time_now > reset_time )
        {
            ESP.restart();
        }
    }


    if ( time_now > ( plane_time + 5000 ) )
    {
        if ( clear_the_screen)
        {
            clear_the_screen = false;
            tft.fillScreen(TFT_BLACK);
           
            weather_time = 0;
            first=true;
           
        }
 
        
        http.useHTTP10(true);
        http.begin("http://192.168.0.3:4443/numplanes");
        http.GET();
        json_text = http.getString();
        //loggit->send(json_text + "\n");
        DynamicJsonDocument doc(1024);
        deserializeJson(doc,json_text.c_str());
        String numplanes = doc["planes"].as<String>() ;
        //loggit->send(numplanes + "\n" );
        http.end();
        tft.setTextColor(TFT_YELLOW,TFT_BLACK);
        tft.setCursor(0,0);
        tft.setTextSize(3);
        tft.setTextFont(1);
        tft.print("Planes : " + numplanes + "   ");

        how_long_left = round((( time_now - weather_time )/ full_time )*(dy-50));
        
        plane_time = time_now;

    }
    unsigned long seconds_left,secs,mins;
    seconds_left = (full_time-(time_now-weather_time))/1000;
    mins = floor(seconds_left/60);
    secs =  seconds_left % 60;
    if ( (secs != last_secs) && (time_now > 5000 ))
    {   
        last_secs = secs;
        tft.setTextSize(2);
        tft.setCursor(0,dy-tft.fontHeight());
        tft.setTextColor(TFT_OLIVE,TFT_BLACK);
        tft.printf("time left %2lu:%02lu     ",mins,secs);
        //loggit->send("hll -> " + String(how_long_left) + " "  +String(mins)+ ":" + String(secs) + " mins:secs\n");
    }
    if (first || (time_now > (weather_time + full_time)))
    {
        first = false;
        weather_time = time_now;
        String url;
        //lat' : '50.836720', 'lon' : '-1.954630' , 'APPID' : 'ed410d3794e1bb9c7fd5cdad31ff703c','units' : 'metric' 
        url="http://api.openweathermap.org/data/2.5/weather";
        url=url+"?lat=50.836720&lon=-1.954630&APPID=ed410d3794e1bb9c7fd5cdad31ff703c&units=metric";
        http.begin(url);
        int rcode = http.GET();

        tft.setCursor(0,50);
        tft.setTextSize(2);
        tft.setTextFont(1);
        if ( rcode == 200 )
        {
            DynamicJsonDocument doc(2048);
            deserializeJson(doc,http.getString().c_str());
            current_temp = doc["main"]["temp"].as<float>();
            current_pressure = doc["main"]["pressure"].as<int>();
            current_wind = doc["wind"]["speed"].as<float>();
            temps.add(current_temp);
            pressures.add(current_pressure);   
            winds.add(current_wind); 

            tft.setTextColor(TFT_GREEN,TFT_BLACK);
            tft.print("temp  " + String(current_temp)+ " C  \n");
            tft.print("press " + String(current_pressure)+ " mB  \n");
            tft.print("wind  " + String(current_wind)+ " km/h   ");

           write_direction(current_temp,temps.average(),0);
           write_direction(current_pressure,pressures.average(),1);
           write_direction(current_wind,winds.average(),2);
        }
        else
        {
            loggit->send("http error code "+String(rcode));
            tft.setTextColor(TFT_RED,TFT_BLACK);
            tft.print("error " + String(rcode)+ "  ");
        }
        http.end();
     
    }
    delay(5);
}