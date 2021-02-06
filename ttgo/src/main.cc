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
     {"ON_THRESHOLD",OnThreshold},
     {"DEBUG",Debug}
     };


IPAddress logging_server;
UDPLogger *loggit;
const char compile_date[] = __DATE__ " " __TIME__;
WiFiUDP  control;

History temps,pressures,winds;


TFT_eSPI tft = TFT_eSPI();   
#define BGCOLOR    0xAD75
#define GRIDCOLOR  0xA815
#define BGSHADOW   0x5285
#define GRIDSHADOW 0x600C
#define RED        0xF800
#define WHITE      0xFFFF

#define TFT_WIDTH_R 320
#define TFT_HEIGHT_R 135

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

unsigned long the_time;
unsigned long weather_time;
bool clear_the_screen = false;
bool  first=false;
float full_time = 60 * 1000 * 15;
int how_long_left = 0;

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
    the_time=millis();
    weather_time = the_time;
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
    tft.println("Initialising");
    Serial.println("Setup done");
    clear_the_screen = true;
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
    button_loop();
#ifdef BALL
    button_loop();
    tft.fillCircle(xpos,ypos,RADIUS,TFT_BLACK);
    ypos = ypos + yinc;
    if ( ypos == (dy - RADIUS ))
        yinc = -1;
    if ( ypos == (RADIUS+15) )
        yinc = 1;
    xpos = xpos + xinc;
    if ( xpos == (dx - RADIUS ))
        xinc = -1;
    if ( xpos == (RADIUS ) )
        xinc = 1;
    tft.fillCircle(xpos,ypos,RADIUS,colour);
#endif

    if ( millis() > ( the_time + 10000 ) )
    {
        if ( clear_the_screen)
        {
            clear_the_screen = false;
            tft.fillScreen(TFT_BLACK);
            // force an initial read
            weather_time = 0;
            first=true;
            //tft.drawRect(dx-11,49,11,dy-49,TFT_BLUE);
            //tft.fillRect(dx-10,50,10,(dy-50),TFT_BLUE);
        }
        //Serial.println("get num planes");
        the_time = millis();
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

        how_long_left = round((( the_time - weather_time )/ full_time )*(dy-50));
        //tft.fillRect(dx-10,50,9,how_long_left,TFT_BLACK);
        unsigned long seconds_left,secs,mins;
        seconds_left = (full_time-(the_time-weather_time))/1000;
        mins = floor(seconds_left/60);
        secs =  seconds_left % 60;
        tft.setTextSize(2);
        tft.setCursor(0,dy-tft.fontHeight());
        tft.setTextColor(TFT_OLIVE,TFT_BLACK);
        tft.printf("time left %2lu:%02lu  ",mins,secs);
        loggit->send("hll -> " + String(how_long_left) + " "  +String(mins)+ ":" + String(secs) + " mins:secs\n");

    }
    if (first || (millis() > (weather_time + full_time)))
    {
        first = false;
        weather_time = millis();
        String url;
        //lat' : '50.836720', 'lon' : '-1.954630' , 'APPID' : 'ed410d3794e1bb9c7fd5cdad31ff703c','units' : 'metric' 
        url="http://api.openweathermap.org/data/2.5/weather";
        url=url+"?lat=50.836720&lon=-1.954630&APPID=ed410d3794e1bb9c7fd5cdad31ff703c&units=metric";
        http.begin(url);
        int rcode = http.GET();
        //json_text = http.getString();
        //Serial.print(" weather to ");
        //Serial.print(rcode);
        tft.setCursor(0,50);
        tft.setTextSize(2);
        tft.setTextFont(1);
        if ( rcode == 200 )
        {
            DynamicJsonDocument doc(2048);
            deserializeJson(doc,http.getString().c_str());
            float temperature = doc["main"]["temp"].as<float>();
            int pressure = doc["main"]["pressure"].as<int>();
            float wind_speed = doc["wind"]["speed"].as<float>();
            temps.add(temperature);
            pressures.add(pressure);   
            winds.add(wind_speed); 

            tft.setTextColor(TFT_GREEN,TFT_BLACK);
            tft.print("temp  " + String(temperature)+ " C  \n");
            tft.print("press " + String(pressure)+ " mB  \n");
            tft.print("wind  " + String(wind_speed)+ " km/h   ");

           write_direction(temperature,temps.average(),0);
           write_direction(pressure,pressures.average(),1);
           write_direction(wind_speed,winds.average(),2);
        }
        else
        {
            loggit->send("http error code "+String(rcode));
            tft.setTextColor(TFT_RED,TFT_BLACK);
            tft.print("error " + String(rcode)+ "  ");
        }
        http.end();
        //tft.fillRect(dx-10,50,10,(dy-50),TFT_BLUE);
    }
    delay(5);
}