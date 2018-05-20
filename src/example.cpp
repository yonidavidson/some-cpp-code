//
#define LOG_ERROR 0
#define LOG_WARNING 1
#define LOG_INFO 2
#define LOG_DEBUG 3
#define LOG_VERBOSE 4
//----------------------------------------------------//
// project definitions
#define PROJECT_NAME "YardStick"
#define REPORT_INTERVAL 60 // in sec (theoretical)
int verbose_lvl = LOG_INFO;
//#define DEBUG_PROG // master debug flag
// Infrastructure definitions
//------------------------------------------------------------------------------------//
// Start LOG functions


char log_buffer[200];

#define LOG(lvl, format, ...) \
  do { \
    if(lvl <= verbose_lvl) {                                    \
      snprintf(log_buffer, sizeof(log_buffer), \
      format, ## __VA_ARGS__ ); \
      Serial.write(log_buffer);                                \
    }                                                           \
  }while(0)
  
#define ERR(format, ...) LOG(LOG_ERROR, format, ## __VA_ARGS__)
#define WARN(format, ...) LOG(LOG_WARNING, format, ## __VA_ARGS__)
#define INFO(format, ...) LOG(LOG_INFO, format, ## __VA_ARGS__)
#define DBG(format, ...) LOG(LOG_DEBUG, format, ## __VA_ARGS__)
#define VERBOSE(format, ...) LOG(LOG_VERBOSE, format, ## __VA_ARGS__)


#ifdef DEBUG_PROG
  #define DEBUG_PRINTLN(x)  Serial.println(x)
  #define DEBUG_PRINT(x)    Serial.print(x)
#else
  #define DEBUG_PRINTLN(x) 
  #define DEBUG_PRINT(x)
#endif
//----------------------------------------------------//
// Infrastructure libraries
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <Losant.h>
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <Ticker.h> //for LED status
#include <EasySSDP.h> // http://ryandowning.net/EasySSDP/
// project specific libraries
//#include <EEPROM.h> // ?
#include "DHT.h"

//#include "SerialCommandProcessor.h"

#define DHTTYPE DHT22   // DHT 22  (AM2302)
//----------------------------------------------------//
// Pin definitions
// Try not to use GPIO 0,2,15 as they are used for boot mode (D3, D4, D8 respectivly)
// Be carefull when using GPIO2/D4 (Used for flashing)
// GPIO16/D0 should be connected to RST when deepsleep mode is needed.
//-----------------
// Infrastructure pins
#define PROG_MODE_SEL_PIN     D5 // Analog soil humidity sensor definitions
#define SW_LED_PIN            D2 // LED pin, used for WiFi status
#define TRIGGER_PIN           D1 // Pull HIGH, Triggers the config portal when set to LOW


// Project specific pins
#define ANALOG_PIN            A0 // Analog soil humidity sensor definitions
#define SOIL_MEA_EN_PIN       D6 // Analog soil humidity sensor definitions

#define DHTPIN                D7 // data pin 2 GPIO2 or D4
#define DHT_PWR_PIN           D8 // not in use.


//----------------------------------------------------//
// function declarations //
//-----------------
// Infrastructure
void Losant_setup();
char* printDouble( double val, unsigned int digits);
//void WiFI_reconnect();
void saveConfigCallback();
void tick();
void configModeCallback (WiFiManager *myWiFiManager);
// project specific infrastructure
void LosantUpdateData(float Air_temperature, float Air_Humidity, float Soil_Humidity);
void SerialPrintData(float Air_temperature, float Air_Humidity, float Soil_Humidity);

// project specific
float GetAnalogReading();
void GetTempHumid( float* Current_AirTemp, float* Current_AirHumid);
void GetSoilHumidity(float* Current_SoilHumidity);
void SaveAnalogPinParameters();
void LoadAnalogPinParameters();

//
typedef void (* GenericFP)(String); //function pointer prototype to a function which takes an 'String' an returns 'void'
typedef struct _cmd
{
  String cmd;
  GenericFP handler;
  String HelpDescriptor;
} cmd_t;

//void SerialPortHandler();
int SerialStringBuilder(char* InputBuffer, int InputBufferSize, int NextPosition);
int CheckSerialBuffer(char* InputBuffer, int InputBufferSize, int NextPosition);
int ProcessInputBuffer(char* InputBuffer, int InputBufferSize, char* CommandBuffer, int CommandBufferSize, char* ArgumentBuffer, int ArgumentBufferSize);

// Handle serial port terminal requests
void yardstick_CommandHandler(String CommandString, String ArgumentsString);
void VERBOSE_handler (String InputArgs);
void parameter_erase_handler (String InputArgs);
void help_handler(String InputArgs);
void analog_read_handler(String InputArgs);
void DHT_ENmode_handler(String InputArgs);
void DHT_SetPowerOnDelay_handler(String InputArgs);

void handle_button_requests(int* GoDbg, int* WFMmode, char* InputBuffer, int InputBufferSize, int* InputBufferPointer);
//----------------------------------------------------//
// Losant default values.
//define your default values here, if there are different values in config.json, they are overwritten.
//-----------------
// Infrastructure
//char Losant_device_ID[30] = "";
//char Losant_access_key[40] = "";
//char Losant_access_secret[70] = "";

char Losant_device_ID[30] = "5ab272df90247b00073fb3ce";
char Losant_access_key[40] = "b18a0870-bc4a-45d9-bbc8-be78db90795d";
char Losant_access_secret[70] = "aa37eceab6d262494bab01e2868e788f8db699439a0ac55c53dac5a18bc8c114";

//char SoilSensor_MinValueStr[10] = "0.00";
//char SoilSensor_MaxValueStr[10] = "100.00";\
// Serial point handler variables


//----------------------------------------------------//
// Declare objects
//-----------------
// Infrastructure
WiFiClientSecure wifiClient; // Losant
LosantDevice device("");
Ticker ticker;      // a timer for LED status
// programm specific
DHT dht(DHTPIN, DHTTYPE);           // declate DHT object for air temp/humid measurements.

//----------------------------------------------------//
// Declare global variables & initial values
// Infrastructure
int timeSinceLastRead = 0;
bool shouldSaveConfig = false; //flag for saving data
// programm specific
int MAX_ANALOG_VAL = 397; // used as initial value, then automatically updates
int MIN_ANALOG_VAL = 258; // used as initial value, then automatically updates
int lastAnalogReading; // for analog soil humidity measurement

int DHT_DynamicPower = 0; // Selects if DHT powers up just before measurement (1) or always on (0)
int DHT_PowerOnDelay = 1000; // Sets the delay between DHT power up and first result request
// vars for serial port handler

//

//----------------------------------------------------//
void setup() 
{
  VERBOSE("VRB: setup:: Started.\n");
  
  // infrastructure variables
  char  InputBuffer[120];
  int   InputBufferPointer = 0;
  int   GoDbg = 0;
  int   WFMmode = 0;
  // application specific variables
  float Air_Humidity = 0;
  float Air_temperature = 0;
  float Soil_Humidity = 0;
  
  //----------------------------------------------------//
  Serial.begin(115200);
  DBG("DBG: setup:: %s debug terminal start...\n", PROJECT_NAME);
  //----------------------------------------------------//
  // Declare pins & pin mode
  // Infrastructure
  pinMode(TRIGGER_PIN, INPUT);        // trigger wifi connect mode
  pinMode(SW_LED_PIN, OUTPUT);        // set switch led pin as output
  pinMode(PROG_MODE_SEL_PIN, INPUT);  //
  digitalWrite(SW_LED_PIN, LOW);      // Turn off switch LED
  // programm specific      
  pinMode(DHT_PWR_PIN, OUTPUT);    //
  pinMode(SOIL_MEA_EN_PIN, OUTPUT);   //
  digitalWrite(DHT_PWR_PIN, HIGH);  // Turn off DHT
  digitalWrite(SOIL_MEA_EN_PIN, LOW);// Turn off soil measurement sensor

  //----------------------------------------------------//
  // Programm begin, infrastructure setup tasks
  if (DHT_DynamicPower == 0)
    {
      INFO("DBG: setup:: Powering on DHT sensor.\n");
      //digitalWrite(DHT_PWR_PIN, HIGH);
    }
  handle_button_requests(&GoDbg, &WFMmode, InputBuffer, sizeof(InputBuffer), &InputBufferPointer); // Handle programming button and reconfig button
  Losant_setup();
  //----------------------------------------------------//
  DBG("Setup:: Starting project specific Setup.\n"); //
  dht.begin();      // start the DHT object
  LoadAnalogPinParameters(); // Load saved soil humidity sensor parameters if they exist
  VERBOSE("VRB: Setup:: Setup done.\n");
  //----------------------------------------------------//
  VERBOSE("VRB: Setup:: Starting the active(loop) part.\n");
  
  GetTempHumid(&Air_temperature, &Air_Humidity);
  GetSoilHumidity(&Soil_Humidity);

  SerialPrintData(Air_temperature, Air_Humidity, Soil_Humidity);
  LosantUpdateData(Air_temperature, Air_Humidity, Soil_Humidity);

  DBG("DBG: setup:: Data sent!, waiting for next cycle\n");
  device.disconnect();
  VERBOSE("VRB: setup:: ended, going to sleep for %d seconds.\n", REPORT_INTERVAL);
  delay(200);
  ESP.deepSleep(REPORT_INTERVAL*1e6); // deepSleep is defined in micro seconds
}

void loop() 
{
  // loop must exist for Arduino environment
} // end of main loop


//----------------------------------------------------//----------------------------------------------------//
// Project infrastructure related functions
void LosantUpdateData(float Air_temperature, float Air_Humidity, float Soil_Humidity) 
{
  VERBOSE("VRB: LosantUpdateData:: Started.\n");
  bool toReconnect = false;

  if(WiFi.status() != WL_CONNECTED) {
    ERR("LosantUpdateData:: Disconnected from WiFi.\n");
    toReconnect = true;
  }

  if(!device.connected()) {
    ERR("LosantUpdateData:: Disconnected from MQTT. Err = %d.\n", device.mqttClient.state());
    toReconnect = true;
  }

  if(toReconnect) {
    //connect();
    //Losant_setup();
    
    int i = 0;
    while (i < 10)
    {
      INFO("connection attempt: %d\n", i);
      device.connectSecure(wifiClient, Losant_access_key, Losant_access_secret);
      int j=0;
      while((!device.connected()) && (j<=10)) 
      {
        delay(500);
        INFO(".");
      }
      INFO("\n");
      if(device.connected())
        break;
    }
    if(!device.connected())
        ESP.restart();
    INFO("LosantUpdateData:: Connected to Losant!\n");
  }
  device.loop();
  
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["AirTemp"] = Air_temperature;
  root["AirHumid"] = Air_Humidity;
  root["SoilHumid"] = Soil_Humidity;
  DBG("LosantUpdateData:: Finished data build.\n");
  device.sendState(root);
  VERBOSE("VRB: LosantUpdateData:: END.\n");
}

void SerialPrintData(float Air_temperature, float Air_Humidity, float Soil_Humidity) 
{
  VERBOSE("VRB: SerialPrintData:: Started.\n");
  
  DBG("Air temperature = %s [C].\n",printDouble(Air_temperature,2));
  DBG("Air humidity = %s [%].\n",printDouble(Air_Humidity,2));
  DBG("Soil humidity = %s [%].\n",printDouble(Soil_Humidity,2));

  VERBOSE("VRB: SerialPrintData:: Ended.\n");
}
//----------------------------------------------------//
// Infrastructure related functions
void handle_button_requests(int* GoDbg, int* WFMmode, char* InputBuffer, int InputBufferSize, int* InputBufferPointer)
{
  VERBOSE("VRB: handle_button_requests:: Started.\n");
  ticker.attach(0.2, tick);// start ticker with 0.2 - In terminal mode
  // Serial terminal variables
  char    CommandBuffer[20] = "";
  char    ArgumentBuffer[100] = "";
  
  while (digitalRead(PROG_MODE_SEL_PIN) == HIGH)
  {
    if (*GoDbg == 0)
    {
      INFO("loop:: Waiting in programming loop.\n");
      INFO("Waiting for command.\n");
      INFO("Command must look like [Command, Arg0 Arg1 ...][CR].\n");
      INFO(">");
      *GoDbg = 1;
    }
    *InputBufferPointer = SerialStringBuilder(InputBuffer, InputBufferSize, *InputBufferPointer);
    if (*InputBufferPointer == -1)
    {
      ERR("main:: SerialStringBuilder has detected a Buffer overrun.\n");
      *InputBufferPointer = 0;
    }
    else
    {
      // check if the input string was terminated by a ']'
      int SerialPacketReady = CheckSerialBuffer(InputBuffer, InputBufferSize, *InputBufferPointer); 
      //int SerialPacketReady = 1;
      if (SerialPacketReady > 0)
        {
          // Detected a well terminated string, go ahead and parse
        int InputStringReady = ProcessInputBuffer(InputBuffer, InputBufferSize, CommandBuffer, sizeof(CommandBuffer), ArgumentBuffer, sizeof(ArgumentBuffer));
        if (InputStringReady == 1) // Command is legal
        {
          DBG("DBG: TestCommandStructure:: Command buffer is \"%s\".\n", CommandBuffer);
          DBG("DBG: TestCommandStructure:: Argument buffer is \"%s\".\n", ArgumentBuffer);
          String CommandString(CommandBuffer);
          String ArgumentsString(ArgumentBuffer);
          yardstick_CommandHandler(CommandString, ArgumentsString);
        }
        // zero all buffers after execution
        InputBuffer[0] = 0;
        *InputBufferPointer = 0;
        CommandBuffer[0] = 0;
        ArgumentBuffer[0] = 0;
        
      }
    }
  delay(10);
  if (digitalRead(TRIGGER_PIN) == HIGH )  // is configuration portal requested?
    {
      INFO("WiFimanager trigger pressed, WFMmode = %d.\n>", *WFMmode);
      if (*WFMmode == 0)
      {
        INFO("Triggered into WiFimanager mode.\n>");
        *WFMmode = 1;
      }
      // WiFI_reconnect();
    }
  }
  ticker.detach(); // once WiFi is connected, stop blinking
  VERBOSE("VRB: handle_button_requests:: Ended.\n");
}

void Losant_setup()
{
  INFO("Losant_setup has started.\n");
  
  ticker.attach(0.6, tick);// start ticker with 0.5 because we start in AP mode and try to connect
  
  
  //
  //SPIFFS.format(); //clean FS, for testing
  //
  //read configuration from FS json
  DBG("DBG: Losant_setup:: mounting FS...\n");

  if (SPIFFS.begin()) 
  {
    DBG("mounted file system\n");
    if (SPIFFS.exists("/config.json")) // try to load the config file for Losant parameters
    {
      //file exists, reading and loading
      DBG("DBG: Losant_setup:: reading config file\n");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) 
      {
        DBG("DBG: Losant_setup:: opened config file for reading\n");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) 
        {
          DBG("DBG: Losant_setup:: parsed json");

          strcpy(Losant_device_ID, json["Losant_device_ID"]);
          strcpy(Losant_access_key, json["Losant_access_key"]);
          strcpy(Losant_access_secret, json["Losant_access_secret"]);

          DBG("DBG: Losant_setup:: Parameters are: Losant_device_ID = \"$s", Losant_device_ID);
          //DEBUG_PRINT(Losant_device_ID);
          DBG("\", Losant_access_key = \"%s", Losant_access_key);
          //DEBUG_PRINT(Losant_access_key);
          DBG("\", Losant_access_secret = \"%s\".\n", Losant_access_secret);
          //DEBUG_PRINT(Losant_access_secret);
          //DEBUG_PRINTLN("\".");

        } 
        else 
        {
          Serial.println("failed to load json config");
        }
      }
    }
  } 
  else
  {
    Serial.println("failed to mount FS");
  }
  // Done with file read procedures
  //----------------------------------------------------------------------------------//
  // Adding parameters to the wifi configuration portal
  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  //                   id/name placeholder/prompt default length
  WiFiManagerParameter custom_Losant_device_ID("ldid", "Losant device ID", Losant_device_ID, 30); 
  WiFiManagerParameter custom_Losant_access_key("lak", "Losant access key", Losant_access_key, 40);
  WiFiManagerParameter custom_Losant_access_secret("las", "Losant access secret", Losant_access_secret, 70);

  WiFiManager wifiManager; //WiFiManager Local intialization. Once its business is done, there is no need to keep it around
  //reset settings - for testing
  //wifiManager.resetSettings();
  //
  wifiManager.setConfigPortalTimeout(300); // (in seconds) Give 5 minutes as timeout before webserver loop ends and exits even if there has been no setup.
                               // usefull for devices that failed to connect at some point and got stuck in a webserver loop
   
  wifiManager.setConnectTimeout(60);  //(in seconds) Give 1 minute timeout for which to attempt connecting, usefull if you get a lot of failed connects                          

  
  
  wifiManager.setAPCallback(configModeCallback);//set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setSaveConfigCallback(saveConfigCallback); //set config save notify callback

  //set static ip ??
  //wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  
  //add all your parameters here
  wifiManager.addParameter(&custom_Losant_device_ID);
  wifiManager.addParameter(&custom_Losant_access_key);
  wifiManager.addParameter(&custom_Losant_access_secret);

  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(PROJECT_NAME)) // fetches ssid and pass and tries to connect, if it does not connect it starts an access point with the specified name.
  {
    Serial.println("failed to connect and hit timeout");
    ESP.reset(); //reset and try again, or maybe put it to deep sleep
    delay(1000);
  }

   //if you get here you have connected to the WiFi
  Serial.print("WiFi connected...yeey :)");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  ticker.detach(); // once WiFi is connected, stop blinking
  digitalWrite(BUILTIN_LED, LOW);//keep LED on

  //copy updated parameters from configuration page into global variables
  strcpy(Losant_device_ID, custom_Losant_device_ID.getValue());
  strcpy(Losant_access_key, custom_Losant_access_key.getValue());
  strcpy(Losant_access_secret, custom_Losant_access_secret.getValue());
  INFO("Losant_setup has finished, preparing to write to file.\n");
  
  if (shouldSaveConfig) //save the custom parameters to FS
  {
    DEBUG_PRINTLN("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["Losant_device_ID"] = Losant_device_ID;
    json["Losant_access_key"] = Losant_access_key;
    json["Losant_access_secret"] = Losant_access_secret;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) 
    {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
  } //end save

    //LosantDevice device(LOSANT_DEVICE_ID);
    device.setId(Losant_device_ID);
     Serial.print("Connecting to Losant...");
    DBG("Losant_device_ID: \"%s\", Losant_access_key: \"%s\", Losant_access_secret: \"%s\".\n", Losant_device_ID, Losant_access_key, Losant_access_secret);
    device.connectSecure(wifiClient, Losant_access_key, Losant_access_secret);
    while(!device.connected()) 
    {
      delay(500);
      Serial.print(".");
    }
    Serial.println("Connected to Losant!");
    //EasySSDP::begin(server, PROJECT_NAME);
    
    INFO("The Losant device is now ready for use!\n");

}
char* printDouble( double val, unsigned int digits)
{
  // prints val with number of decimal places determine by precision
  // NOTE: precision is 1 followed by the number of zeros for the desired number of decimial places
  // example: printDouble( 3.1415, 100); // prints 3.14 (two decimal places)
  long precision = pow(10,digits);
  int   index = 0;
  static  char s[16];
  char  format[16];
  unsigned int frac;
  if(val >= 0)
    frac = (val - int(val)) * precision;
  else
    frac = (int(val)- val ) * precision;
  sprintf(format, "%%d.%%0%dd", digits);
  sprintf(s, format, int(val), frac);

  return s;
}
void WiFI_reconnect()
{
   VERBOSE("VRB: WiFI_reconnect:: Started.\n");
   bool toReconnect = false;
   //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    //reset settings - for testing
    //wifiManager.resetSettings();

    //sets timeout until configuration portal gets turned off
    //useful to make it all retry or go to sleep
    //in seconds
    //wifiManager.setTimeout(120);

    //it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration

    //WITHOUT THIS THE AP DOES NOT SEEM TO WORK PROPERLY WITH SDK 1.5 , update to at least 1.5.1
    //WiFi.mode(WIFI_STA);
    
    if (!wifiManager.startConfigPortal(PROJECT_NAME)) 
    {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
    }

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
    
    if(!device.connected()) 
    {
    Serial.println("Disconnected from MQTT");
    Serial.println(device.mqttClient.state());
    toReconnect = true;
    }
    if(toReconnect) 
    {
    Serial.print("Connecting to Losant...");
    //device.connectSecure(wifiClient, LOSANT_ACCESS_KEY, LOSANT_ACCESS_SECRET);
    device.connectSecure(wifiClient, Losant_access_key, Losant_access_secret);

    
  
    while(!device.connected())
      {
        delay(500);
        Serial.print(".");
      }
    }
  VERBOSE("VRB: WiFI_reconnect:: Ended.\n");
}


//callback notifying us of the need to save config
void saveConfigCallback () 
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}


void tick()
{
  //toggle state
  int state = digitalRead(SW_LED_PIN);  // get the current state of GPIO1 pin
  digitalWrite(SW_LED_PIN, !state);     // set pin to the opposite state
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) 
{
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}
//----------------------------------------------------//
// Project specific functions
void SaveAnalogPinParameters()
{
  // Use this function to save the analog pin min/max parameters
  VERBOSE("VRB: SaveAnalogPinParameters:: Started.\n");
  char MinValueStr[10];
  char MaxValueStr[10];
  itoa(MIN_ANALOG_VAL, MinValueStr, 10);
  itoa(MAX_ANALOG_VAL, MaxValueStr, 10);
  DEBUG_PRINTLN("saving soil sensor config");
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["SoilSensor_MinValue"] = MinValueStr;
  json["SoilSensor_MaxValue"] = MaxValueStr;

  File configFile = SPIFFS.open("/AnalogPin_config.json", "w");
  if (!configFile) 
  {
    Serial.println("failed to open config file for writing");
  }

  json.printTo(Serial);
  Serial.println("");
  json.printTo(configFile);
  configFile.close();
  VERBOSE("VRB: SaveAnalogPinParameters:: Ended.\n");
}


void LoadAnalogPinParameters() 
{
  // Use this function to load the saved parameters - if they exist
  // Otherwise use defaults

  VERBOSE("VRB: LoadAnalogPinParameters:: Started.\n");
  
  char AnalogPin_MinValueStr[10] = "0.00";
  char AnalogPin_MaxValueStr[10] = "100.00";
  
  if (SPIFFS.begin()) 
{
  DBG("DBG: LoadAnalogPinParameters:: mounted file system.\n");
  if (SPIFFS.exists("/AnalogPin_config.json")) 
  {
    //file exists, reading and loading
    DBG("DBG: LoadAnalogPinParameters:: reading Soil sensor config file.\n");
    File configFile = SPIFFS.open("/AnalogPin_config.json", "r");
    if (configFile) 
      {
        DBG("DBG: LoadAnalogPinParameters:: opened SoilSensor config file\n");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) 
        {
          Serial.println("\nparsed json");

          strcpy(AnalogPin_MinValueStr, json["SoilSensor_MinValue"]);
          strcpy(AnalogPin_MaxValueStr, json["SoilSensor_MaxValue"]);
          MAX_ANALOG_VAL = atoi(AnalogPin_MaxValueStr);
          MIN_ANALOG_VAL = atoi(AnalogPin_MinValueStr);

          DBG("DBG: LoadAnalogPinParameters:: Loaded MAX_ANALOG_VAL from file (%d).\n", MAX_ANALOG_VAL);
          DBG("DBG: LoadAnalogPinParameters:: Loaded MIN_ANALOG_VAL from file (%d).\n", MIN_ANALOG_VAL);

          //Serial.print("SoilSensor parameters are: SoilSensor_MinValue = \"");
          //Serial.print(MIN_ANALOG_VAL);
          //Serial.print("\", SoilSensor_MaxValue = \"");
          //Serial.print(MAX_ANALOG_VAL);
          //Serial.println("\".");

        } 
        else 
        {
          ERR("ERR: LoadAnalogPinParameters:: failed to load SoilSensor json config file.\n");
        }
      }
    } 
  }
  VERBOSE("VRB: LoadAnalogPinParameters:: Ended.\n");
}          


float GetAnalogReading()
{
  // Read analog pin, filter and percentalize
  // lastAnalogReading is a global persistant value for filtering
  
  VERBOSE("GetAnalogReading:: started.\n");
  int   CurrentAnalogReading = analogRead(ANALOG_PIN);
  DBG("Analog value: %d.\n", CurrentAnalogReading);
  // filter 
  //lastAnalogReading += (CurrentAnalogReading - lastAnalogReading) / 10;
  lastAnalogReading = CurrentAnalogReading;
  DBG("Filtered analog value: %d.\n", lastAnalogReading);
  if (CurrentAnalogReading < MIN_ANALOG_VAL)
  {
    MIN_ANALOG_VAL = CurrentAnalogReading;
    //lastAnalogReading = CurrentAnalogReading;
    DBG("Soil humidity MIN_ANALOG_VAL adjusted to: %s.\n",printDouble(CurrentAnalogReading,2));
    SaveAnalogPinParameters();
  }
   if (CurrentAnalogReading > MAX_ANALOG_VAL)
  {
    MAX_ANALOG_VAL = CurrentAnalogReading;
    //lastAnalogReading = CurrentAnalogReading;
    DBG("Soil humidity MAX_ANALOG_VAL adjusted to: %s,\n",printDouble(CurrentAnalogReading,2));
    SaveAnalogPinParameters();
  }
  // calculate soil humidity in % 
  float CurrentAnalogValuePercent = map(lastAnalogReading, MIN_ANALOG_VAL, MAX_ANALOG_VAL, 0, 100); // map(value, fromLow, fromHigh, toLow, toHigh)
  DBG("Percentilized analog value is: %s.\n",printDouble(CurrentAnalogValuePercent,2));
  VERBOSE("GetAnalogReading:: ended.\n");
  return CurrentAnalogValuePercent;
}
void GetSoilHumidity(float* Current_SoilHumidity)
{
  digitalWrite(SOIL_MEA_EN_PIN, HIGH);// Turn on soil measurement sensor
  delay(200); // Power up to data valid delay
  *Current_SoilHumidity    = GetAnalogReading();
  digitalWrite(SOIL_MEA_EN_PIN, LOW);// Turn off soil measurement sensor
}
void GetTempHumid( float* Current_AirTemp, float* Current_AirHumid)
{
  // Get DHT sensor data
  VERBOSE("GetTempHumid:: Started.\n");

  if (DHT_DynamicPower == 1)
    {
      DBG("DBG: GetTempHumid:: Powering off DHT sensor.\n");
      //digitalWrite(DHT_PWR_PIN, HIGH);
      delay(DHT_PowerOnDelay); // Power up to data valid delay
    }
  
  for(int i=0;i<5;i++)
  {
    *Current_AirTemp             = dht.readTemperature(); // dht.getTemperature();
    *Current_AirHumid            = dht.readHumidity(); // dht.getHumidity();
    if (isnan(*Current_AirTemp) || isnan(*Current_AirHumid)) 
    {
      ERR("GetTempHumid:: Failed to read from DHT sensor. i=%d!\n", i);
      delay(100);
    }
    else
    {
      DBG("GetTempHumid:: DHT22 Temp=%s",printDouble(*Current_AirTemp,2));
      DBG(", DHT22 Humidity=%s.\n", printDouble(*Current_AirHumid,2));
      break;
    }
  }
  if (DHT_DynamicPower == 1)
    {
      DBG("DBG: GetTempHumid:: Powering off DHT sensor.\n");
      //digitalWrite(DHT_PWR_PIN, LOW);
    }
  VERBOSE("GetTempHumid:: Ended.\n");   
}

//---------------------------------------------------------------//
int SerialStringBuilder(char* InputBuffer, int InputBufferSize, int NextPosition)
{
  /// This function is called when data is available in the serial buffer.
  /// it builds the serial string until the string end character is received.
  bool PrintDbgMessage = false;

  VERBOSE("SerialStringBuilder:: Started.\n");
  
  while ((Serial.available()) &&  (NextPosition < InputBufferSize))// Data is available at Serial
  {
    char inChar = Serial.read(); // Read data from serial
    InputBuffer[NextPosition] = inChar;
    Serial.write(inChar);
    NextPosition++;
    PrintDbgMessage = true;
  }
  // check end conditions
  if(NextPosition == InputBufferSize)
  {
    ERR("SerialStringBuilder:: Buffer overrun\n");
    return -1;
  }
  else
  {
    VERBOSE("SerialStringBuilder:: Ended.\n");
    if (PrintDbgMessage)
      DBG("SerialStringBuilder:: Next position is %d.\n",NextPosition);
    return NextPosition;
  }
}

// Commands related to handlins serial terminal commands
int CheckSerialBuffer(char* InputBuffer, int InputBufferSize, int NextPosition)
{
  // This function checks if the data available in the serial port input buffer is ready.
  // it checks for two conditions:
  // Current character (last character in buffer) is a [CR] (\n) - then it terminates the string buffer with a \0 and returns the buffer length
  // if buffer is not in ready, it returns -1;.
  VERBOSE("CheckSerialBuffer:: Started.\n");
  if (InputBuffer[NextPosition - 1] == ']')
    {
      InputBuffer[NextPosition] = 0;
      VERBOSE("VRB: CheckSerialBuffer:: Ended, a well terminater string was found.\n");
      INFO("\n");
      return NextPosition;
    }
  else
    {
      VERBOSE("CheckSerialBuffer:: Ended, string not terminated yet.NextPosition = %d.\n", NextPosition);
      return -1;
    }
}

int ProcessInputBuffer(char* InputBuffer, int InputBufferSize, char* CommandBuffer, int CommandBufferSize, char* ArgumentBuffer, int ArgumentBufferSize)
{
  // This function is called when a well terminated string was received in the serial port.
  // The function now searches if a command string can be found in the input string
  // a command string is defined as as a string that starts with a "[" character and is terminated with a "]" character.
  // if a valid command string is found, it is stripped of the limiting characters, trimmed of null characters and passed to be decoded.
  // the rest of the string is trimmed and sent as the argument string

  VERBOSE("ProcessInputBuffer:: Started");
  
  int i = 0;
  int LegalBufferStart;
  int LegalBufferEnd;
  char TempString[120];
  String CommandString;
  String ArgumentsString;
  char TempCmdBuff[20];
  char TempArgBuff[100];
  
  
  
  while ((InputBuffer[i] != ']') && (i < InputBufferSize))// Data is available for analysis, look for end character
  {
    i++;
  }
  if (InputBuffer[i] == ']')
  {
    LegalBufferEnd = i;
    DBG("ProcessInputBuffer:: Found buffer end at position %d.\n", i);
    i = 0;
    while ((InputBuffer[i] != '[') && (i < LegalBufferEnd))// look for buffer start
    {
      i++;
    }
    if (InputBuffer[i] == '[')
    {
      //INFO("Legal string is \"%s\"\n", )
      LegalBufferStart = i+1;
      DBG("ProcessInputBuffer:: Found buffer start at position %d and end at position %d.\n", LegalBufferStart, LegalBufferEnd);
      i = 0;
      for(int j=LegalBufferStart;j<LegalBufferEnd;j++)
      {
        TempString[i] = InputBuffer[j];
        i++;
      }
      TempString[i] = 0;
      //String CommandString = inString.substring(StringStart+1, StringEnd);
      DBG("ProcessInputBuffer:: Command sub-string is \"%s\"\n", TempString);
      String CommandString(TempString);
      
      CommandString.trim();
      DBG("ProcessCommandLine:: Trimmed Command sub-string is \"%s\"\n", CommandString.c_str());
      
      if (CommandString.indexOf(',') > 0)
      {
        CommandString = strtok(TempString, ",");
        //DBG("ProcessInputBuffer:: Command buffer is \"%s\"\n", CommandString.c_str());
        //String CommandBufferString(CommandBuffer);
        //INFO("ProcessInputBuffer:: Command String is \"%s\"\n", CommandBufferString.c_str());
        CommandString.trim();
        //INFO("ProcessInputBuffer:: Trimmed Command String is \"%s\"\n", CommandString.c_str());
        
        CommandString.toCharArray(TempCmdBuff, sizeof(TempCmdBuff));
        int CmdStrLen = CommandString.length();
        strncpy(CommandBuffer, TempCmdBuff, CmdStrLen);
        CommandBuffer[CmdStrLen] = 0;
        //INFO("ProcessInputBuffer:: Trimmed Command buffer is \"%s\"\n", CommandBuffer);
        
        ArgumentsString = strtok(NULL, ",");
        //INFO("ProcessInputBuffer::Argument buffer is \"%s\"\n", ArgumentsString.c_str());
        //String ArgumentBufferString(ArgumentBuffer);
        //INFO("ProcessInputBuffer:: Argument String is \"%s\"\n", ArgumentBufferString.c_str());
        ArgumentsString.trim();
        //INFO("ProcessInputBuffer:: Trimmed Argument String is \"%s\"\n", ArgumentsString.c_str());
        ArgumentsString.toCharArray(TempArgBuff, sizeof(TempArgBuff));
        int ArgStrLen = ArgumentsString.length();
        strncpy(ArgumentBuffer, TempArgBuff, ArgStrLen);
        ArgumentBuffer[ArgStrLen] = 0;
      }
      else
      {
        DBG("ProcessInputBuffer:: Command only state\n");
        CommandString.toCharArray(TempCmdBuff, sizeof(TempCmdBuff));
        int CmdStrLen = CommandString.length();
        strncpy(CommandBuffer, TempCmdBuff, CmdStrLen);
        CommandBuffer[CmdStrLen] = 0;
        ArgumentBuffer[0] = 0;

      }
      DBG("ProcessInputBuffer:: Command is \"%s\", Arguments are \"%s\"\n", CommandBuffer, ArgumentBuffer);
      VERBOSE("ProcessInputBuffer:: Ended");
      return 1;
    }
    else
    {
      ERR("ProcessInputBuffer:: Buffer Start character is missing\n");
    }
  }
  else if (i == InputBufferSize)//InputBufferSize
  {
    ERR("ProcessInputBuffer:: Buffer overrun. i=%d, InputBufferSize = %d, size of input buffer = %d\n",i , InputBufferSize, sizeof(InputBuffer));
    VERBOSE("ProcessInputBuffer:: Ended");
    return -1; // Buffer overrun
  }
  else
  {
    VERBOSE("ProcessInputBuffer:: Ended");
    return 0;
  }
}




//void DHT_ENmode_handler(String InputArgs), void DHT_SetPowerOnDelay_handler(String InputArgs)
  cmd_t cmds[] = {
  {"z", parameter_erase_handler, "[z]: Erase analog pin parameters"},
  {"h" , help_handler, "[h]: Print help for all available commands"},
  {"a", analog_read_handler,"[a]: Measure & display raw analog pin values"},
  {"DDP", DHT_ENmode_handler, "[DDP, B]: Enable DHT sensor dynamic power control (B = 1, default) or always on (B = 0)"},
  {"DDPD", DHT_SetPowerOnDelay_handler, "[DDPD, X]: Set DHT sensor dynamic power delay (PWR_En to first measurement) to X mSec's"},
//  {"VERBOSE", VERBOSE_handler},
//  {"GET_IP_ADDR", GET_IP_ADDR_handler},
//  {"GET_IP_ADDR", GET_IP_ADDR_handler},
//  {"GET_IP_ADDR", GET_IP_ADDR_handler},
//  {"GET_IP_ADDR", GET_IP_ADDR_handler},
  {"v", VERBOSE_handler, "[v, X]: Set verbose level to X. 0 = LOG_ERROR, 1 = LOG_WARNING, 2 = LOG_INFO, 3 = LOG_DEBUG, 4 = LOG_VERBOSE."}          
//  {"VERBOSE", VERBOSE_handler, "[VERBOSE, X]: Set verbose level to X. 0 = LOG_ERROR, 1 = LOG_WARNING, 2 = LOG_INFO, 3 = LOG_DEBUG, 4 = LOG_VERBOSE."}
  };


void yardstick_CommandHandler(String CommandString, String ArgumentsString)
{
  VERBOSE("yardstick_CommandHandler:: Started.\n");
  DBG("DBG: yardstick_CommandHandler:: command = \"%s\",Arguments = \"%s\"\n", CommandString.c_str(), ArgumentsString.c_str());
  const char* String1 = CommandString.c_str();
  
  for(int i=0;i<sizeof(cmds)/sizeof(cmds[0]);i++)
  {
    
    const char* String2 =cmds[i].cmd.c_str();
    VERBOSE("VRB: yardstick_CommandHandler:: Comparing command = \"%s\",to constant = \"%s\"\n", String1, String2);
    if (strcmp(String1, String2) == 0)
    {
      DBG("DBG: yardstick_CommandHandler::  Match found\n");
      cmds[i].handler(ArgumentsString);
      DBG("DBG: yardstick_CommandHandler::  handler executed\n");
    }
  }
  VERBOSE("yardstick_CommandHandler:: Ended.\n");
}

void VERBOSE_handler (String InputArgs)
{
  
  VERBOSE("VRB: VERBOSE_handler:: Started");
  DBG("DBG: VERBOSE_handler:: InputArgs = \"%s\".\n", InputArgs.c_str());
  verbose_lvl = atoi(InputArgs.c_str());
  INFO("levels:: 0 = LOG_ERROR, 1 = LOG_WARNING, 2 = LOG_INFO, 3 = LOG_DEBUG, 4 = LOG_VERBOSE.\n");
  INFO("Verbose level set to %d.\n", verbose_lvl);
  INFO("\n>");
  VERBOSE("VRB: VERBOSE_handler:: Ended");
}

void parameter_erase_handler (String InputArgs)
{
  VERBOSE("VRB: parameter_erase_handler:: Started.\n");
  INFO("parameter_erase_handler:: Erasing analog parameters.\n");
  MIN_ANALOG_VAL = 1024;
  MAX_ANALOG_VAL = 0;
  SaveAnalogPinParameters();
  INFO("\n>");
  VERBOSE("VRB: parameter_erase_handler:: Ended.\n");
}

void help_handler(String InputArgs)
{
  VERBOSE("VRB: help_handler:: Started.\n");
  int NumOfCmds = sizeof(cmds)/sizeof(cmd_t);
  DBG("DBG: help_handler:: NumOfCmds = %d, sizeof(cmds) = %d, sizeof(cmd_t) = %d.\n", NumOfCmds, sizeof(cmds), sizeof(cmd_t));
  for(int i=0;i<NumOfCmds;i++)
  {
    INFO("%s\n",cmds[i].HelpDescriptor.c_str());
  }
  INFO(">");
  VERBOSE("VRB: help_handler:: Ended.\n");
}

void analog_read_handler(String InputArgs)
{
  VERBOSE("VRB: analog_read_handler:: Started.\n");
  int   CurrentAnalogReading = analogRead(ANALOG_PIN);
  INFO("loop:: Soil humidity is %d [Analog] [%d...%d].\n",CurrentAnalogReading, MIN_ANALOG_VAL, MAX_ANALOG_VAL);
  INFO("\n>");
  VERBOSE("VRB: analog_read_handler:: Ended.\n");
}
void DHT_ENmode_handler(String InputArgs)
{
  VERBOSE("VRB: DHT_ENmode_handler:: Started.\n");
  DBG("DBG: DHT_ENmode_handler:: InputArgs = \"%s\".\n", InputArgs.c_str());
  DHT_DynamicPower = atoi(InputArgs.c_str());
  INFO("DBG: DHT_ENmode_handler:: DHT_DynamicPower set to %d.\n", DHT_DynamicPower); //
  INFO("\n>");
  VERBOSE("VRB: DHT_ENmode_handler:: Ended.\n");
}

void DHT_SetPowerOnDelay_handler(String InputArgs)
{
  VERBOSE("VRB: DHT_SetPowerOnDelay_handler:: Started.\n");
  DBG("DBG: DHT_SetPowerOnDelay_handler:: InputArgs = \"%s\".\n", InputArgs.c_str());
  DHT_PowerOnDelay = atoi(InputArgs.c_str());
  INFO("DBG: DHT_SetPowerOnDelay_handler:: DHT_PowerOnDelay set to %d.\n", DHT_PowerOnDelay);
  INFO("\n>");
  VERBOSE("VRB: DHT_SetPowerOnDelay_handler:: Ended.\n");
}

    
