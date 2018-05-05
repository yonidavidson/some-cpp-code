#include "HeavenlyWaters_Includes.h"
#include "SerialCommandProcessor.h"
#include "HeavenlyWaters_ESP8266_SerialCommands.h"
#include "Bounce2.h"


int verbose_lvl = LOG_ERROR; // LOG_ERROR, LOG_WARNING ,LOG_INFO, LOG_DEBUG, LOG_VERBOSE
IPAddress remote_ip(192, 168, 0, 1); // for PING command

char log_buffer[200]; // for LogFunctions
char str_buffer[200]; // for LogFunctions

//typedef enum {Nothing, Error, Warning, Info, Extreme}  LogLevel_t ;
//LogLevel_t LOG_LEVEL = Extreme; //

// WiFi network parameters
const char* ssid = "Linksys14142";
const char* password = "bqcy547y9s";
//const char* ssid = "Termirate";
//const char* password = "kL426144!";

int LastNtpReply = -1;


String webPage = "";


//bool ConnectionEstablished = false;
//String inString;
unsigned long timeLastCheck = 0;
unsigned long intervalCheck = 500;


extern ConnectionState_t Serial_ConnectionState;
extern ConnectionState_t WiFi_ConnectionState;
extern ConnectionState_t mdns_ConnectionState;
extern ConnectionState_t HTTP_ConnectionState;

int8_t timeZone = 2; // for NTP, GMT+2 for Israel
int8_t minutesTimeZone = 0;// for NTP
bool wifiFirstConnected = false;// for NTP
boolean syncEventTriggered = false; // for NTP// True if a time even has been triggered
NTPSyncEvent_t ntpEvent; // for NTP// Last triggered event
String NTP_Reply = "";// for NTP
String NTP_Reply_human = "";// for NTP

Bounce debouncer = Bounce(); // Instantiate a Bounce object

void setup(void)
{
	//ESP.wdtDisable();
	pinMode(led_pin, OUTPUT);
	digitalWrite(led_pin, LOW);
	
	pinMode(pb_led_pin, OUTPUT);
	digitalWrite(pb_led_pin, LOW);
	
	pinMode(pb_sw_pin, INPUT);
	pinMode(pb_sw_pin,INPUT_PULLUP);
	debouncer.attach(pb_sw_pin);
	debouncer.interval(50); // interval in ms
	
	Serial.begin(115200);
	while (!Serial) 
	{
		; // wait for serial port to connect. Needed for native USB port only
	}
	BuildWebPage();
	
	static WiFiEventHandler e1, e2, e3; // for NTP
	NTP.onNTPSyncEvent ([](NTPSyncEvent_t event) 
	{
        ntpEvent = event;
        syncEventTriggered = true;
    });
	//NTP.setDayLight (TRUE);

    e1 = WiFi.onStationModeGotIP (onSTAGotIP);// As soon WiFi is connected, start NTP Client
    e2 = WiFi.onStationModeDisconnected (onSTADisconnected);
    e3 = WiFi.onStationModeConnected (onSTAConnected);
}


void loop(void)
{
	unsigned long timeNow = millis(); // Used if pseudo-counters are used for time hops
	static int i = 0;
	static int last = 0;
	
	char InputBuffer[120];
	int  InputBufferPointer = 0;
	int InputStringReady = 0;
	String CommandString;
	String ArgumentsString;

	char CommandBuffer[20] = "";
	char ArgumentBuffer[100] = "";

	
	debouncer.update();// Update the Bounce instance :
	int PbDebouncerValue = debouncer.read();	// Get the updated value :
	if ( PbDebouncerValue == LOW ) 
	{
		//digitalWrite(LED_PIN, HIGH );
	}
	else 
	{
		//digitalWrite(LED_PIN, LOW );
	}
	
	ManageSerialConnectionState();
	ManageWiFiConnectionState();
	ManageMdnsState();
	ManageHttpServerState();
	
	InputBufferPointer = SerialStringBuilder(InputBuffer, sizeof(InputBuffer), InputBufferPointer);
	if (InputBufferPointer == -1) // Buffer overrun
		InputBufferPointer = 0; // zero out the buffer
	
	if(InputBufferPointer > 0)
	{
		InputStringReady = ProcessInputBuffer(InputBuffer, sizeof(InputBuffer), CommandBuffer, sizeof(CommandBuffer), ArgumentBuffer, sizeof(ArgumentBuffer));
		if (InputStringReady == 1) // Command is legal
		{
			//DBG("TestCommandStructure:: Command buffer is \"%s\".\n", CommandBuffer);
			//DBG("TestCommandStructure:: Argument buffer is \"%s\".\n", ArgumentBuffer);
			String CommandString(CommandBuffer);
			String ArgumentsString(ArgumentBuffer);
			//DBG("TestCommandStructure:: Command String is \"%s\".\n", CommandString.c_str());
			//DBG("TestCommandStructure:: Argument String is \"%s\".\n", ArgumentsString.c_str());
			HeavenlyWaters_ESP8266_CommandHandler(CommandString, ArgumentsString);
			
		}
		// Command buffer is not ready yet.
	}
	
	/*
	if(Serial.available() > 0)
	{
		InputBufferPointer = serialEventHandler(InputBuffer, sizeof(InputBuffer), InputBufferPointer);
		HeavenlyWaters_ESP8266_CommandHandler(CommandBuffer, ArgumentBuffer);
	}
	*/
	
	if(HTTP_ConnectionState == Connected)
	{
		WebServer_HandleClient();
	}
	

	if (wifiFirstConnected) 
	{
		wifiFirstConnected = false;
		NTP.begin ("pool.ntp.org", timeZone, true, minutesTimeZone);
		//NTP.begin ("139.162.219.252", timeZone, true, minutesTimeZone);
		NTP.setInterval (63);
	}
	
	if (syncEventTriggered) 
	{
		processSyncEvent (ntpEvent);
		syncEventTriggered = false;
	}

	if ((millis() - last) > 5100) 
	{
		UpdateNtpMessage(i);
		last = millis();
		i++;
	}

}


void UpdateNtpMessage(int i)
{
			//Serial.println(millis() - last);
		//int last = millis ();

		String CurrentTimeDate = NTP.getTimeDateString ();
		String SummerWinter = NTP.isSummerTime() ? "Summer Time. " : "Winter Time. ";
		String WiFiState = WiFi.isConnected () ? "connected" : "not connected";
		String Uptime = NTP.getUptimeString ();
		String NtpFirstSyncTime = NTP.getTimeDateString (NTP.getFirstSync());
		
		INFO("%d: %s, %s, ",i , CurrentTimeDate.c_str(), SummerWinter.c_str());
		INFO("WiFi is %s, Uptime: %s, ", WiFiState.c_str(), Uptime.c_str());
		INFO("since %s.\n", NtpFirstSyncTime.c_str());
		
		/*
		if(LOG_LEVEL >= Info)
		{

			Serial.print (i); 
			Serial.print (" ");
			Serial.print (NTP.getTimeDateString ());
			Serial.print (" ");
			Serial.print (NTP.isSummerTime () ? "Summer Time. " : "Winter Time. ");
			Serial.print ("WiFi is ");
			Serial.print (WiFi.isConnected () ? "connected" : "not connected"); Serial.print (". ");
			Serial.print ("Uptime: ");
			Serial.print (NTP.getUptimeString ()); Serial.print (" since ");
			Serial.println (NTP.getTimeDateString (NTP.getFirstSync ()).c_str ());

		}
		*/
		//return last;
}	







void onSTAConnected (WiFiEventStationModeConnected ipInfo) 
{
	INFO("Connected to %s\r\n", ipInfo.ssid.c_str ());
}


// Start NTP only after IP network is connected
void onSTAGotIP (WiFiEventStationModeGotIP ipInfo) 
{
	INFO ("Got IP: %s\r\n", ipInfo.ip.toString ().c_str ());
	INFO("Connected: %s\r\n", WiFi.status () == WL_CONNECTED ? "yes" : "no");
	wifiFirstConnected = true;
}

// Manage network disconnection
void onSTADisconnected (WiFiEventStationModeDisconnected event_info) 
{
	INFO("Disconnected from SSID: %s\n", event_info.ssid.c_str ());
	INFO("Reason: %d\n", event_info.reason);
	// digitalWrite (ONBOARDLED, HIGH); // Turn off LED
	//NTP.stop(); // NTP sync can be disabled to avoid sync errors
}

void processSyncEvent (NTPSyncEvent_t ntpEvent) 
{
	NTP_Reply_human = "";
	NTP_Reply = "";
	
	if (ntpEvent)
	{
		NTP_Reply_human +="NTPERR: ";
		if (ntpEvent == noResponse)
		{
			NTP_Reply_human += "NREACH"; // Serial.println ("NTP server not reachable");
			NTP_Reply = "ERR:01";
			INFO ("Time Sync error: NTP server not reachable\n");
		}
		else if (ntpEvent == invalidAddress)
		{
			NTP_Reply_human += "NSRVR"; // Serial.println ("Invalid NTP server address");
			NTP_Reply = "ERR:02";
			INFO ("Time Sync error: Invalid NTP server address");
		}
	}
	else
	{
		NTP_Reply_human += NTP.getTimeDateString (NTP.getLastNTPSync ());
		LastNtpReply = millis();
		NTP_Reply = String(NTP.getLastNTPSync());
		INFO ("Got NTP time: %s.\n", NTP.getTimeDateString (NTP.getLastNTPSync ()).c_str());
	}
}




