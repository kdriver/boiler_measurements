#include <Arduino.h>

#include <wifi_password.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <StringHandler.h>

WiFiUDP  control;

// sid SK59c605e1c770d91fce9a53e75e638ede
// DVmGmscLmcJjp1st6AAO3m2Y1TmaTRdP
ESP8266WebServer server(80);
void handleRoot();              // function prototypes for HTTP handlers
void handleNotFound();

#include <InfluxDb.h>

#define INFLUXDB_HOST "piaware.local"
#define INFLUXDB_PORT "1337"
#define INFLUXDB_DATABASE "boiler_measurements"

enum BoilerState { boiler_is_off,boiler_is_on};
enum BoilerAction { boiler_switched_on, boiler_switched_off, boiler_no_change};


#define ON_THRESHOLD 400
#define OFF_THRESHOLD 300

#define WATCHDOG_INTERVAL 300

bool DEBUG_ON=true;

bool quiet = false;
unsigned long epoch;
unsigned long time_now;
unsigned long since_epoch;

BoilerAction boiler;
BoilerState boiler_status= boiler_is_off;
unsigned int boiler_switched_on_time;

unsigned int min_level = 1023;
unsigned int max_level = 0;
unsigned int current_level = 0;

Influxdb influx(INFLUXDB_HOST);

unsigned int smooth_on = 0;
unsigned int smooth_off = 0;

#define HISTORY_SIZE 300
unsigned short int history[HISTORY_SIZE];
unsigned int history_entry=0; 

void tell_influx(BoilerState status, unsigned int time_interval)
{
  InfluxData measurement("radiators_ldr");
  
  measurement.addValue("interval",time_interval);
  measurement.addValue("interval_mins",time_interval/60);
  measurement.addValue("interval_secs",time_interval%60);

  if ( status == boiler_is_off )
    measurement.addValue("value",0);
  else
    measurement.addValue("value",1);

    
    influx.write(measurement);
  
}

void tick_influx(String text,unsigned int min, unsigned int max,unsigned int current)
{
  InfluxData measurement("tick_ldr");
  measurement.addTag("lifecycle",text);
  measurement.addValue("min_reading",min);
  measurement.addValue("max_reading",max);
  measurement.addValue("current",current);
  measurement.addValue("value",1);
  measurement.addValue("uptime_hours",millis()/1000/60/60);
  influx.write(measurement);
}

void setup(void){
  Serial.begin(9600);
  Serial.println("I'm alive");
  
  WiFi.begin("cottage", WIFIPASSWORD);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  
  String address = WiFi.localIP().toString();
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println("cottage");
  Serial.print("IP address: ");
  Serial.println(address);

if (MDNS.begin("boilerldr")) {              // Start the mDNS responder for esp8266.local
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error setting up MDNS responder!");
  }

  influx.setDb(INFLUXDB_DATABASE);
  epoch = millis()/1000;
  server.on("/", handleRoot);               // Call the 'handleRoot' function when a client requests URI "/"
  server.onNotFound(handleNotFound);        // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"
  server.begin(); 

  // Tell the database we are starting up
//  tick_influx(String("Initialised_1_0_"+ address),0,1000,0);

  for ( int i = 0 ; i < HISTORY_SIZE ; i++ )
    history[i] = 2000;    // 200 is an invalid value from the analogueread

  control.begin(8787);

}
void command(String command)
{
  command.toUpperCase();
  if ( command.startsWith("HELP"))
  {
    Serial.println("Usage: [SET | GET | HELP] ");
  }
  StringHandler sh(command.c_str());
  unsigned int tokens;
  tokens = sh.tokenise();
  Serial.print("Found ");
  Serial.print(tokens);
  Serial.println(" tokens");
  for ( unsigned int i =0 ; i < tokens ; i++ )
  {
     const char *tok;
     tok = sh.get_token(i);
     if ( tok != 0 )
     {
       Serial.println(tok);
     }
  }
  
   
}
void handleUDPPackets(void) {
  char packet[500];
  int cb = control.parsePacket();
  if (cb) {
    int length;
    length  = control.read(packet, sizeof(packet));
    String myData = "";
    for(int i = 0; i < length; i++) {
      myData += (char)packet[i];
    }
    Serial.println(myData);
    command(myData);
    IPAddress remoteIP = control.remoteIP();
   
    control.beginPacket(control.remoteIP(),control.remotePort());
    String answer = remoteIP.toString() + "\n";
    control.write(answer.c_str() );
    control.endPacket();

  }
}

void loop() {

  delay(500);

  handleUDPPackets();

  unsigned int a0pin ;
    
  a0pin = analogRead(A0);

  current_level = a0pin;

  history[history_entry] = current_level;

  history_entry = history_entry + 1;

  if ( history_entry >= HISTORY_SIZE )
  {
    history_entry = 0;
  }

  if ( a0pin > max_level )
    max_level = a0pin;
  if ( a0pin < min_level )
    min_level = a0pin;

  since_epoch = (millis()/1000) - epoch;

if ( DEBUG_ON  || !(since_epoch % WATCHDOG_INTERVAL) )
 {
  float voltage;
  voltage = a0pin * 3.3/1024.0;
  since_epoch = millis() - epoch;
  Serial.print(since_epoch);
  Serial.print("\t");
  Serial.print(a0pin);
  Serial.print("\t");
  Serial.print(voltage);
  Serial.print("\t");
  Serial.print("boiler_status \t");
  Serial.println(boiler_status);
  //tick_influx("alive",min_level,max_level,current_level);
 }
  boiler = boiler_no_change;

  if ( (boiler_status == boiler_is_off ) && (a0pin > ON_THRESHOLD) )
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

if ( (boiler_status == boiler_is_on ) && (a0pin <  OFF_THRESHOLD ) )
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
  document = document + "</body></</html>";

  
   server.send(200, "text/html",  document );   // Send HTTP status 200 (Ok) and send some text to the browser/client
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}