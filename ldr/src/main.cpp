#include <Arduino.h>
#include <wifi_password.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP_EEPROM.h>
// these two lines need to be before include StringHandler
enum Command  { Get,Set,Help,Status,Reset};
enum Attribute { OnThreshold,OffThreshold,Debug };

#include <StringHandler.h>
#include <UDPLogger.h>
//#include <esp_log.h>

#define MDNS_NAME "ufh"

IPAddress logging_server;
UDPLogger *loggit;

const char compile_date[] = __DATE__ " " __TIME__;
WiFiUDP  control;

ESP8266WebServer server(80);
void handleRoot();              // function prototypes for HTTP handlers
void handleNotFound();

#include <InfluxDb.h>

#define INFLUXDB_HOST "piaware.local"
#define INFLUXDB_PORT "1337"
#define INFLUXDB_DATABASE "boiler_measurements"

CommandSet ldr_commands[] = {{"GET",Get,2},{"SET",Set,4} ,{"HELP",Help,1},{"STATUS",Status,1},{"RESET",Reset,1}};
AttributeSet ldr_attributes[] = { {"ON_THRESHOLD",OnThreshold},{"OFF_THRESHOLD",OffThreshold},{"DEBUG",Debug}};

enum BoilerState { boiler_is_off,boiler_is_on};
enum BoilerAction { boiler_switched_on, boiler_switched_off, boiler_no_change};

#define ON_THRESHOLD 400
#define OFF_THRESHOLD 300

// 10 seconds
#define TICK_INTERVAL_MS (1000*30)
unsigned long last_tick_ts = millis();

bool DEBUG_ON=false;
bool reset = false;
unsigned int  reset_count = 10;
unsigned long epoch;
unsigned long time_now;
unsigned long since_epoch;

BoilerAction boiler;
BoilerState boiler_status= boiler_is_off;
unsigned int boiler_switched_on_time;

unsigned int min_level = 1023;
unsigned int max_level = 0;
unsigned int current_level = 0;

struct Persistent {
  char     initialised[10];
  unsigned int on_thresh;
  unsigned int off_thresh;
} thresholds;

Influxdb influx(INFLUXDB_HOST);
Influxdb influx_tick(INFLUXDB_HOST);

unsigned int smooth_on = 0;
unsigned int smooth_off = 0;

 

void tell_influx(BoilerState status, unsigned int time_interval)
{
  //InfluxData measurement("radiators_ldr");
  InfluxData measurement("ufh_ldr");
  
  measurement.addValue("interval",time_interval);
  measurement.addValue("interval_mins",time_interval/60);
  measurement.addValue("interval_secs",time_interval%60);

  if ( status == boiler_is_off )
    measurement.addValue("value",0);
  else
    measurement.addValue("value",1);

    
    influx.write(measurement);
  
}

void tick_influx(String tag1,String tag2,float value)
{
  InfluxData measurement("tick_ufhldr");
  measurement.addTag("text",tag1);
  measurement.addTag("info",tag2);
  measurement.addValue("value",value);
  influx_tick.write(measurement);
}
void report_influx(String tag1,String tag2,float value)
{
  InfluxData measurement("report_ufhldr");
  measurement.addTag("text",tag1);
  measurement.addTag("info",tag2);
  measurement.addValue("value",value);
  influx.write(measurement);
}
void setup(void){
  char text[100];
  Serial.begin(9600);
  Serial.println("\nI'm alive");
  Serial.flush();

  
   
  WiFi.begin("cottage", WIFIPASSWORD);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  //logging_server = MDNS.queryHost("piaware");
  //Serial.println(logging_server.toString());

  #ifdef ESP32
  String logging_host = String("piaware.local");
  #else
  String logging_host = String("192.168.0.3");
  #endif
  loggit = new UDPLogger(logging_host.c_str(),(unsigned short int)8788);
  loggit->init();
  
  String address = WiFi.localIP().toString();
  Serial.println(""); 
  Serial.println("Built on " + String(compile_date)+ "\n");
  Serial.print("Connected to cottage : IP Address ");
  Serial.println(address);

if (MDNS.begin(MDNS_NAME)) {              // Start the mDNS responder for esp8266.local
    Serial.println("mDNS responder started for host ufh.local");
  } else {
    Serial.println("Error setting up MDNS responder!");
  }

  influx.setDb(INFLUXDB_DATABASE);
  influx_tick.setDb("ticks");
  epoch = millis()/1000;
  server.on("/", handleRoot);               // Call the 'handleRoot' function when a client requests URI "/"
  server.onNotFound(handleNotFound);        // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"
  server.begin(); 

  EEPROM.begin(sizeof(thresholds));

  EEPROM.get(0,thresholds);

  thresholds.initialised[3] = 0;

  if ( strcmp("YES",thresholds.initialised) )
  {
    Serial.println("Flash not initialised - set default parameters ");
    thresholds.on_thresh = ON_THRESHOLD;
    thresholds.off_thresh = OFF_THRESHOLD;
    strcpy(thresholds.initialised,"YES");
    EEPROM.put(0,thresholds);
    EEPROM.commit();
  }
  EEPROM.get(0,thresholds);
  sprintf(text,"on threshold=%d , off_threshold=%d\n",thresholds.on_thresh,thresholds.off_thresh);
  Serial.print(text);

  // Tell the database we are starting up
  tick_influx(String("started"), address,1.0);

  int udp = control.begin(8788);
  Serial.println("start UDP server on port 8788 " + String(udp)); 

}
String command(String command)
{
  command.toUpperCase();
  StringHandler sh(command.c_str(),sizeof(ldr_commands)/sizeof(CommandSet),ldr_commands,sizeof(ldr_attributes)/sizeof(AttributeSet),ldr_attributes);
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
          case OnThreshold:
            answer = "On Threshold : " + String(thresholds.on_thresh);
          break;
          case OffThreshold:
            answer = "Off Threshold : " + String(thresholds.off_thresh);
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
              thresholds.on_thresh = value;
              EEPROM.put(0,thresholds);
              EEPROM.commit();
            break;
            case OffThreshold:
              thresholds.off_thresh = value;
              EEPROM.put(0,thresholds);
              EEPROM.commit();
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
        answer = "Resetting in 5 seconds ";
        reset = true;
        reset_count = 10;
    break;
    case Status:
  
      if ( boiler_status == boiler_is_on)
      {
        int boiler_on_for;
        boiler_on_for = (millis()/1000) - boiler_switched_on_time;
        answer = "Boiler has been ON for " + String(boiler_on_for) + " seconds\nCurrent level " + String(current_level) + "\n";
      }
      else
      {
        answer = "Boiler is OFF ,  Current level " + String(current_level) + " , ";
        
      }
      
      answer = answer + "On Threshold : " + String(thresholds.on_thresh) + "\n";
      answer = answer + "Off Threshold : " + String(thresholds.off_thresh) + "\n";
      answer = answer + "Running for " + String(millis()/1000) + " seconds ( " + String(millis()/1000/3600) + " hours ) ";
    break;
    case Help:
    answer = "Commands ( ";
      for ( unsigned int i = 0 ; i < sizeof(ldr_commands)/sizeof(CommandSet); i++ )
      {
        if ( i )
          answer = answer + " | " + ldr_commands[i].cmd ;
        else
          answer = answer + ldr_commands[i].cmd ;
      }
      answer = answer + " ) , Attributes ( ";
      for ( unsigned int i = 0 ; i < sizeof(ldr_attributes)/sizeof(AttributeSet); i++ )
      {
        if ( i )
          answer = answer + " | " + ldr_attributes[i].attr ;
        else
          answer = answer + ldr_attributes[i].attr  ;
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
    String answer = "ufh ldr > " + text + "\n";
    control.write(answer.c_str() );
    control.endPacket();

  }
}

void loop() {

  unsigned long int current_ts;

  delay(500);

  if ( reset == true )
  {
    reset_count = reset_count - 1;
    Serial.print("count down to reset ");
    Serial.println(reset_count);
    if ( reset_count == 0 )
    {
      ESP.reset();
    }
  }
  MDNS.update();

  handleUDPPackets();

  unsigned int a0pin ;
    
  a0pin = analogRead(A0);

  current_level = a0pin;

  if ( a0pin > max_level )
    max_level = a0pin;
  if ( a0pin < min_level )
    min_level = a0pin;

   current_ts = millis();

if ( (current_ts - last_tick_ts)>TICK_INTERVAL_MS)
 {
  last_tick_ts = current_ts;
  report_influx("ufh","level",(float)current_level);
  String my_text = "ufh current " + String(current_level) + " max " + String(max_level) + " min " + String(min_level );
  loggit->send(my_text + "\n");
  Serial.println(my_text);
 }
  boiler = boiler_no_change;

  if ( (boiler_status == boiler_is_off ) && (a0pin > thresholds.on_thresh) )
  {
      smooth_on = smooth_on + 1 ;
      if ( smooth_on == 3 )
      {
         //Serial.println(" Boiler On");
         boiler = boiler_switched_on;
         smooth_off = 0;
      }
  }
  else
  {
    smooth_on = 0;
  }

if ( (boiler_status == boiler_is_on ) && (a0pin <  thresholds.off_thresh ) )
  {
      smooth_off = smooth_off + 1 ;
      if ( smooth_off == 3 )
      {
         //Serial.println(" Boiler Off");
         boiler = boiler_switched_off;
         smooth_on = 0;
      }
  }
  else
  {
    smooth_off = 0;
  }

  switch(boiler)
  {
    case boiler_no_change:
    break;
    case boiler_switched_on:
      Serial.println("Boiler switched on");
      boiler_switched_on_time = millis()/1000;
      boiler_status = boiler_is_on;
      tell_influx(boiler_is_on,0);
    break;
    case boiler_switched_off:
      if ( boiler_status == boiler_is_on )
      {
        int boiler_on_for;
        boiler_on_for = (millis()/1000) - boiler_switched_on_time;
        Serial.print("Boiler on for ");
        Serial.print(boiler_on_for);
        Serial.print(" seconds , boiler switched OFF\n");
        boiler_status=boiler_is_off;
        tell_influx(boiler_is_off,boiler_on_for);
      }
    break;
    default:
      Serial.println("ERROR - Default case in Boiler switch\n");
  }

  server.handleClient(); 

}
void handleRoot() {
  String document;
  document = "<html><head> <meta http-equiv=\"refresh\" content=\"20\"></head><body><p>Current level ";
  document = document +  String(current_level,DEC);
  document = document +  "</p><p> Boiler status ";
  document = document + String((boiler_status == boiler_is_on) ? "ON"  : "OFF");
  document = document + "</p>Up and running for " + String(millis()/1000/3600) + " hours </p>";
  document = document + "</body></</html>";

  
   server.send(200, "text/html",  document );   // Send HTTP status 200 (Ok) and send some text to the browser/client
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}
