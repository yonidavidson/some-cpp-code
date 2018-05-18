	/*
 * HeavenlyWaters_ESP8266_SerialCommands.cpp
 *
 * Created: 25-Feb-18 14:11:08
 *  Author: ubear
 */ 
#include <Arduino.h>
#include <ESP8266Ping.h>
#include <ESP8266WiFi.h>

#include "HeavenlyWaters_ESP8266_SerialCommands.h"
#include "LogFunctionDefinitions.h"
#include "Pins.h"
#include "SessionManagers.h"

extern ConnectionState_t Serial_ConnectionState;
extern IPAddress remote_ip;

cmd_t cmds[] = {
	{"PING", PING_handler},
	{"LED" , LED_handler}, // There is no LED on this board/ESP pin !!!
	{"SER_TEST", SER_TEST_handler}, // Tested OK.
	{"ESP_INFO", ESP_INFO_handler},
	{"GET_IP_ADDR", GET_IP_ADDR_handler},
//	{"VERBOSE", VERBOSE_handler},
//	{"GET_IP_ADDR", GET_IP_ADDR_handler},
//	{"GET_IP_ADDR", GET_IP_ADDR_handler},
//	{"GET_IP_ADDR", GET_IP_ADDR_handler},
//	{"GET_IP_ADDR", GET_IP_ADDR_handler},
						
	{"VERBOSE", VERBOSE_handler}
};

void PING_handler (String InputArgs)
{
	INFO("PING_handler:: has started, Arg = %s.\n", InputArgs.c_str());
	DBG("PING_handler:: Pinging address \"%s\",\n", InputArgs.c_str());
	remote_ip.fromString(InputArgs.c_str());
		
	if(Ping.ping(remote_ip))
	{
		INFO("HandleSerialRequest:: PING: Host has been reached!!\n");
		Serial.print("[PING: ");
		Serial.print(InputArgs);
		Serial.print(",OK]");
	}
	else
	{
		INFO("HandleSerialRequest:: PING: Host unreachable\n");
		Serial.print("[PING: ");
		Serial.print(InputArgs);
		Serial.print(",NOK]");
	}
}

void ESP_INFO_handler (String InputArgs)
{
	INFO("ESP_INFO_handler:: has started, Arg = %s.\n", InputArgs.c_str());
	SerialPrintStat();
}

void SER_TEST_handler (String InputArgs)
{
	INFO("SER_TEST_handler:: has started, Arg = %s.\n", InputArgs.c_str());
	Serial.println("[SER_TEST, ACK]");
	Serial_ConnectionState = Connected;
}

void LED_handler (String InputArgs)
{
	INFO("LED_handler:: has started, Arg = %s.\n", InputArgs.c_str());
	if (strcmp(InputArgs.c_str(), "ON") == 0)
	{
		INFO("LED_handler:: Turning LED On\n");
		digitalWrite(led_pin, HIGH);
	}
	else if (strcmp(InputArgs.c_str(), "OFF") == 0)
	{
		INFO("LED_handler:: Turning LED Off\n");
		digitalWrite(led_pin, LOW);
	}
	else
	{
		ERR("LED_handler:: Bad argument\n");
	}
}

void GET_IP_ADDR_handler (String InputArgs)
{
	INFO("GET_IP_ADDR_handler:: has started, Arg = %s.\n", InputArgs.c_str());
	Serial.print("[IPADDR, ");
	Serial.print(WiFi.localIP());
	Serial.println("]");
}

void VERBOSE_handler (String InputArgs)
{
	verbose_lvl = atoi(InputArgs.c_str());
	INFO("VERBOSE_handler:: has started, Arg = %s.\n", InputArgs.c_str());
}

void HeavenlyWaters_ESP8266_CommandHandler(String command, String Arguments)
{
	DBG("HeavenlyWaters_ESP8266_CommandHandler:: command = \"%s\",Arguments = \"%s\"\n", command.c_str(), Arguments.c_str());
	for(int i=0;i<sizeof(cmds)/sizeof(cmds[0]);i++)
	{
		const char* String1 = command.c_str();
		const char* String2 =cmds[i].cmd.c_str();
		DBG("HeavenlyWaters_ESP8266_CommandHandler:: Comparing command = \"%s\",to constant = \"%s\"\n", String1, String2);
		if (strcmp(String1, String2) == 0)
		{
			DBG("HeavenlyWaters_ESP8266_CommandHandler::  Match found\n");
			cmds[i].handler(Arguments);
			
		}
	}
}


void SerialPrintStat()
{
	Serial.println("");

	Serial.println("ESP8266 board info:");
	Serial.print("\tChip ID: ");
	Serial.println(ESP.getFlashChipId());
	Serial.print("\tCore Version: ");
	Serial.println(ESP.getCoreVersion());
	Serial.print("\tChip Real Size: ");
	Serial.println(ESP.getFlashChipRealSize());
	Serial.print("\tChip Flash Size: ");
	Serial.println(ESP.getFlashChipSize());
	Serial.print("\tChip Flash Speed: ");
	Serial.println(ESP.getFlashChipSpeed());
	Serial.print("\tChip Speed: ");
	Serial.println(ESP.getCpuFreqMHz());
	Serial.print("\tChip Mode: ");
	Serial.println(ESP.getFlashChipMode());
	Serial.print("\tSketch Size: ");
	Serial.println(ESP.getSketchSize());
	Serial.print("\tSketch Free Space: ");
	Serial.println(ESP.getFreeSketchSpace());

};