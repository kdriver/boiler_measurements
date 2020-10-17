#include <Arduino.h>
#include <wifi_password.h>
#include <WiFi.h>
#include <Wire.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include "History.h"
#include "UDPLogger.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <driver/adc.h>
#include <Webtext.h>
#include <ESPmDNS.h>
#include <ArduinoNvs.h>

#define MDNS_NAME "boiler"

#define DEVICE "ESP32"
#include <InfluxDbClient.h>
InfluxDBClient *tick_client,*data_client;

Point tick("sound_tick");
Point diag("boiler_sound");
Point boiler_s("boiler_status");

enum Command  { Get,Set,Help,Status,Reset};
enum Attribute { LoopDelay,SamplePeriod,SamplesForAverage,OnThreshold,Debug };

#include <StringHandler.h>
CommandSet sound_commands[] = {{"GET",Get,2},{"SET",Set,4} ,{"HELP",Help,1},{"STATUS",Status,1},{"RESET",Reset,1}};
AttributeSet sound_attributes[] = {
     {"ON_THRESHOLD",OnThreshold},
     {"LOOP_DELAY",LoopDelay},
     {"SAMPLE_PERIOD",SamplePeriod},
     {"SAMPLES_FOR_AVERAGE",SamplesForAverage},
     {"DEBUG",Debug}
     };

const char compile_date[] = __DATE__ " " __TIME__;
WiFiUDP  control;


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


#define BOILER_OFF 0
#define BOILER_ON 1
#define BOILER_NO_CHANGE 2
#define BOILER_STARTING 3
#define ON_THRESHOLD 900
#define OFF_THRESHOLD 300
#define WATCHDOG_INTERVAL 300

unsigned long epoch;
unsigned long time_now;
unsigned long previous_time;
unsigned long since_epoch;

unsigned int boiler;
unsigned int boiler_status= BOILER_OFF;
unsigned int boiler_switched_on_time=0;
unsigned int abs_average;
char web_page[16384];
//unsigned char historic_readings[1000];

unsigned long last_time=millis();
unsigned long on_for=0;
unsigned int threshold = 1300;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

String address = "0.0.0.0";

bool DEBUG_ON=false;
bool quiet = false;
bool reset = false;
unsigned int reset_count = 3;

unsigned int loop_delay = 50;
unsigned int sample_period = 50;
unsigned int sample_average = 30;
unsigned int boiler_on_threshold = 120;
unsigned int boiler_on_threshold_1 = 1800;

AsyncWebServer server(80);
void handleRoot();              // function prototypes for HTTP handlers
void handleTest();
void handleNotFound();

IPAddress logging_server;
UDPLogger *loggit;

#ifdef REDUNDENT
int post_it(String payload ,String db)
{
    HTTPClient http;
    int response;
    //Serial.println("post to influx\n");
    String target;
    target= "http://" + logging_server.toString() + ":8086/write?db="+db;
    http.begin(target);
    http.addHeader("Content-Type","text/plain");
    //Serial.print(target + " : ");
    //Serial.println(payload);
    response = http.POST(payload);

    loggit->send(db + " " + payload + "\n");
    if ( response > 250 )
      loggit->send("InfluxDb POST error response " + String(response) + "\n");

    http.end();
    return response;
}
#endif
void report_event_to_influx(Point &p,unsigned int status, unsigned int time_interval)
{
  String payload; 
  //int response;
  p.clearFields();
  p.addField("interval",(float)time_interval);
  p.addField("interval_mins",(float)(time_interval/60));
  p.addField("interval",(float)(time_interval%60));
  p.addField("bolier_on",(float)status);

  data_client->writePoint(p);

  //payload = "boiler_status interval=" + String(time_interval);
  //payload = payload + ",interval_mins=" + String(time_interval/60);
  //payload = payload + ",interval_secs=" + String(time_interval%60);
  //payload = payload + ",boiler_on=" + String(status);
  //post_it(payload,"boiler_measurements");
  //Serial.print(response);
  
}
void diag_influx(Point &p, unsigned int sound, unsigned int boiler_status)
{
  p.clearFields();
  p.addField("value",(float)sound);
  p.addField("boiler_on",(float)(boiler_status==BOILER_ON?1:0));
  data_client->writePoint(p);
  
  //String payload;
  //payload = "boiler_sound value=" + String(sound);
  //payload = payload + ",boiler_on=" + String(boiler_status==BOILER_ON?1:0);
  //post_it(payload,"boiler_measurements");
}


hw_timer_t *timer=NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
//History *history = new History(300);
History *pp_history = new History(500);

void IRAM_ATTR sound();
void IRAM_ATTR measureit();
unsigned long measure_interval = sample_period;
unsigned int measure() {
  // called on interrupt
  unsigned int max_val = 0;
  unsigned int min_val = 1024;
  unsigned long start = millis();
  unsigned int reading;
  unsigned int pp;

  while ( millis() < ( start + measure_interval ))
  {
      reading = analogRead(A0);
      if ( reading < 5000 )
      {
         if ( reading > max_val )
             max_val = reading;
         if ( reading < min_val )
            min_val = reading;
      }
  }
  // work out the peak to peak value measure over the interval
  pp = max_val - min_val;

  // record it in the history
  portENTER_CRITICAL_ISR(&timerMux);
  pp_history->add(pp);
  portEXIT_CRITICAL_ISR(&timerMux);
  return pp;
}

void p_lcd(String s,unsigned int x,unsigned int y)
{
  display.setTextSize(1); 
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(x, y);
  display.println(s);
  display.display();
}
/*
SETUP=================================================================================================
*/
void setup() {
char prog[] = {'|','/','-','\\'};
int rot=0;
String c;

Serial.begin(9600);


if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

Serial.println("I'm alive");
Serial.println("Built on : " + String(compile_date));
display.clearDisplay();
p_lcd("Searching for WiFi",0,0);
 
  WiFi.begin("cottage", WIFIPASSWORD);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
    c=prog[rot];
    rot = rot + 1 ;
    if ( rot == sizeof(prog))
      rot = 0;
    display.writeFillRect(0,8,5,8,SSD1306_BLACK);
    p_lcd(c,0,8);
  }
  
  
  p_lcd("i'm alive",0,0);   

  address = WiFi.localIP().toString();
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println("cottage");
  Serial.print("IP address: ");
  Serial.println(address);

    // Set up mDNS responder:
    // - first argument is the domain name, in this example
    //   the fully-qualified domain name is "esp8266.local"
    // - second argument is the IP address to advertise
    //   we send our IP address on the WiFi network
    if (!MDNS.begin(MDNS_NAME)) {
        Serial.println("Error setting up MDNS responder!");
    }
    else
      Serial.println("mDNS responder started");

    // Start TCP (HTTP) server
    Serial.println("TCP server started");

    // Add service to MDNS-SD
    MDNS.addService("http", "tcp", 80);

    
    logging_server = MDNS.queryHost("piaware");
    Serial.println(logging_server.toString());
    loggit = new UDPLogger(logging_server.toString().c_str(),(unsigned short int)8788);
    loggit->init();

    bool ans;
    ans = NVS.begin("boiler");
    loggit->send("Initialised NVS access result " + String(ans?" OK \n": " Failed\n"));
    if (NVS.getInt("b_on_thresh1") == 0  )
    {
      
      NVS.setInt("loop_delay",(uint32_t)50);
      NVS.setInt("sample_period",(uint32_t)50);
      NVS.setInt("sample_average",(uint32_t)30);
      NVS.setInt("b_on_thresh",(uint32_t)120);
      ans = NVS.setInt("b_on_thresh1",(uint32_t)1800,true);
      loggit->send("initialise NVRAM values " + String(ans?"True":"False") + "\n");
    }
    else
      loggit->send("read parameters from NVRAM\n");

      boiler_on_threshold = NVS.getInt("b_on_thresh");
      loop_delay = NVS.getInt("loop_delay");
      sample_period = NVS.getInt("sample_period");
      sample_average = NVS.getInt("sample_average");
      pp_history->update_ma_period(sample_average);
      boiler_on_threshold_1 = NVS.getInt("b_on_thresh1");

  loggit->send(" boiler on threshold1 set to " + String(boiler_on_threshold_1) );
  loggit->send("\n boiler on (unused)   set to " + String(boiler_on_threshold) );
  loggit->send("\n sample Average       set to " + String(sample_average) );
  loggit->send("\n loop_delay           set to " + String(loop_delay) );
  loggit->send("\n sample_period        set to " + String(sample_period) + "\n" );

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    sprintf(web_page,index_html,pp_history->moving_average(sample_average),boiler_status==0?"OFF":"ON",boiler_on_threshold,loop_delay,sample_period,sample_average,boiler_on_threshold_1,(sample_average*loop_delay/1000.0));
    request->send(200, "text/html", web_page);});               // Call the 'handleRoot' function when a client requests URI "/"

  server.on("/graph", HTTP_GET, [](AsyncWebServerRequest *request){
    String the_data;
    String the_mov_ave;
    String labels;
   
    int length = 0;
    the_data = pp_history->list();
    the_mov_ave = pp_history->list_of_ma();
    length = pp_history->num_entries()-1;
    labels = String("[");
    for ( int i =0; i < length; i++ )
    {
        if ( (i+1) == length )
         labels= labels + String(i);
        else
         labels= labels + String(i) + String(",");
    }
    labels = labels + String("]");
    labels = pp_history->list_of_times();
    sprintf(web_page,graph,labels.c_str(),the_data.c_str(),the_mov_ave.c_str());
    request->send(200, "text/html", web_page);});     

  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    bool ans;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    inputMessage = "No message sent";
    inputParam = "none";
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      boiler_on_threshold = inputMessage.toInt();
      NVS.setInt("b_on_thresh",boiler_on_threshold,true);
    }
    if (request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_2)->value();
      inputParam = PARAM_INPUT_2;
      loop_delay = inputMessage.toInt();
      NVS.setInt("loop_delay",loop_delay,true);
    }
    if (request->hasParam(PARAM_INPUT_3)) {
      inputMessage = request->getParam(PARAM_INPUT_3)->value();
      inputParam = PARAM_INPUT_3;
      sample_period = inputMessage.toInt();
      NVS.setInt("sample_period",sample_period,true);
    }
    if (request->hasParam(PARAM_INPUT_4)) {
      inputMessage = request->getParam(PARAM_INPUT_4)->value();
      inputParam = PARAM_INPUT_4;
      sample_average = inputMessage.toInt();
      NVS.setInt("sample_average",sample_average,true);
      pp_history->update_ma_period(sample_average);
    }
     if (request->hasParam(PARAM_INPUT_5)) {
      inputMessage = request->getParam(PARAM_INPUT_5)->value();
      inputParam = PARAM_INPUT_5;
      boiler_on_threshold_1 = inputMessage.toInt();
      ans = NVS.setInt("b_on_thresh1",boiler_on_threshold_1,true);
      loggit->send("set boiler threshold 1 to " + String(boiler_on_threshold_1) + " result " + String(ans?"OK\n":"Failed\n"));
    }
    Serial.println(inputMessage);
    request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                     + inputParam + ") with value: " + inputMessage +
                                     "<br><a href=\"/\">Return to Home Page</a>");
  });
  server.onNotFound(notFound);
  server.begin();  
  int udp = control.begin(8788);
  Serial.println("start UDP server on port 8788 " + String(udp));
  
  loggit->send("up and running\n");
 
  String influx_url;
  influx_url = "http://" + logging_server.toString() + ":8086";
  tick_client = new InfluxDBClient(influx_url.c_str(),"ticks");
  data_client = new InfluxDBClient(influx_url.c_str(),"boiler_measurements");

  
  tick.addTag("device", DEVICE);
  tick.addTag("SSID", WiFi.SSID());
  tick.addTag("IP", WiFi.localIP().toString());
  p_lcd("start influx",0,0);
  Serial.print(influx_url);
   
  tick.clearFields();
  // Report RSSI of currently connected network
  tick.addField("rssi", WiFi.RSSI());
   if (!tick_client->writePoint(tick)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(tick_client->getLastErrorMessage());
  }
  else
  {
    Serial.println("wrote influx datapoint ok");
  }
  //tick_influx(String("initialised"), address,1.0);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  epoch = millis();
  previous_time = epoch;
//  Just set the threshold to the first reading. We'll average it out later.
  threshold = analogRead(A0);

  p_lcd("end of setup",0,0);
  Serial.println("setup is finnished\n");

  display.clearDisplay();
}

unsigned int events = 0;
//int last_second_events =0;
float moving_average;

unsigned int choose_scale(unsigned int max)
{
  unsigned int scale=500;

  scale = max * 1.5;

  if  ( scale < 100 )
    scale = 100;
       return scale;
}


bool read_analogue()
{
    int ma;
    bool detected_on = false;

      measure();
      ma = pp_history->moving_average(sample_average);
      if  ( ma > boiler_on_threshold_1 )
      {
        detected_on = true;
      }
      else
      {
          detected_on = false;
      }
      
    return  detected_on ;
}
String command(String command)
{
  command.toUpperCase();
  StringHandler sh(command.c_str(),sizeof(sound_commands)/sizeof(CommandSet),sound_commands,sizeof(sound_attributes)/sizeof(AttributeSet),sound_attributes);
  int toks;
  toks = sh.tokenise();

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
          case OnThreshold:
            answer = "On Threshold : " + String(boiler_on_threshold_1);
          break;
          case LoopDelay:
            answer = "Loop Delay : " + String(loop_delay);
          break;
          case SamplesForAverage:
            answer = "Samples to average: " + String(sample_average);
          break;          
          case SamplePeriod:
            answer = "Sample period (ms) : " + String(sample_period);
          break;
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
            case OnThreshold:
              boiler_on_threshold_1 = value;
              NVS.setInt("b_on_thresh1",(uint32_t)boiler_on_threshold_1,true);
            break;
            case LoopDelay:
              loop_delay = value;
              NVS.setInt("loop_delay",(uint32_t)loop_delay,true);
            break;
            case SamplesForAverage:
              sample_average = value;
              NVS.setInt("sample_average",(uint32_t)sample_average,true);
            break;
            case SamplePeriod:
              sample_period = value;
              NVS.setInt("sample_period",(uint32_t)sample_period,true);
            break;      
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
      reset = true;
      Serial.println("Reset requested");
      loggit->send("Reset requested");
    break;
    case Status:
    {
      int ma = pp_history->moving_average(sample_average);
      if ( boiler_status == BOILER_ON)
      {
        int boiler_on_for;
        boiler_on_for = (millis()/1000) - boiler_switched_on_time;
        answer = "Boiler has been ON for " + String(boiler_on_for) + " seconds, Current ave " + String(ma) + " , ";
      }
      else
      {
        answer = "Boiler is OFF ,  Current ave " + String(ma) ;
        
      }
      
      answer = answer + "\nOn Threshold    : " + String(boiler_on_threshold_1);
      answer = answer + "\nSample Period   : " + String(sample_period);
      answer = answer + "\nSamples for Ave : " + String(sample_average);
      answer = answer + "\nLoop Delay      : " + String(loop_delay);
      answer = answer + "\nDebug           : " + String(DEBUG_ON)+ "\n";
    }
    break;
    case Help:
      answer = "Commands ( ";
      for ( unsigned int i = 0 ; i < sizeof(sound_commands)/sizeof(CommandSet); i++ )
      {
        if ( i )
          answer = answer + " | " + sound_commands[i].cmd ;
        else
          answer = answer + sound_commands[i].cmd ;
      }
      answer = answer + " ) , Attributes ( ";
      for ( unsigned int i = 0 ; i < sizeof(sound_attributes)/sizeof(AttributeSet); i++ )
      {
        if ( i )
          answer = answer + " | " + sound_attributes[i].attr ;
        else
          answer = answer + sound_attributes[i].attr  ;
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
    IPAddress target = control.remoteIP();
    control.beginPacket(target,control.remotePort());
    String answer = "boiler ldr > " + text + "\n";
    control.print(answer);
    control.endPacket();

  }
}
void loop() {
bool boiler_on = false;
        handleUDPPackets();
        delay(loop_delay);
        display.clearDisplay();
        String s;
        int ma;
        time_now = millis();
        boiler_on = read_analogue();
        
        if ( (time_now - last_time)  >  2000 )
        {
          if (WiFi.status() == WL_CONNECTED)
            p_lcd("WiFi OK",72,0);
          else
          {
            p_lcd("No WiFi",72,0);
            reset = true;
          }
          p_lcd(address + " " + String(time_now/1000),0,8);
          if ( boiler_status == true )
          {
            p_lcd("BOILER ON ",0,0);
            p_lcd("ON for " + String((time_now - boiler_switched_on_time)/1000),0,16);
          }
          else
          {
            p_lcd("BOILER OFF",0,0);
            p_lcd("was ON for " + String(on_for),0,16);
          }
           p_lcd("threshold " + String(boiler_on_threshold_1),0,24);
           ma = pp_history->moving_average(sample_average);
          // history->add(abs_average);
           diag_influx(diag,ma,boiler_status);
          
          last_time = time_now;
        
          if ( reset == true )
          {
              reset_count = reset_count - 1;
              Serial.println("Counting down to reset " + String(reset_count));
              if ( reset_count == 0 )
                ESP.restart();
          }
        }


        if ( (boiler_status == BOILER_OFF) && (boiler_on == true) )
        {
            String text;
            text = String("Boiler switched ON \n ") ;
            loggit->send(text);
            report_event_to_influx(boiler_s,BOILER_ON,0);
            boiler_status = BOILER_ON;
            boiler_switched_on_time = time_now;
          
        }
        if ( (boiler_status == BOILER_ON)  && ( boiler_on == false ) )
        {
            unsigned int  interval;
            char output[70];
            interval = (time_now - boiler_switched_on_time)/1000;
            on_for = interval;
            if ( interval > 5 )
            {
              report_event_to_influx(boiler_s,BOILER_OFF,interval);
            }
            boiler_status = BOILER_OFF;
            sprintf(output,"boiler was on for %d seconds \n",interval);
            loggit->send(output);
        }

}      
