/*
 * HeavenlyWaters_ESP8266_SerialCommands.h
 *
 * Created: 25-Feb-18 14:09:51
 *  Author: ubear
 */ 


#ifndef HEAVENLYWATERS_ESP8266_SERIALCOMMANDS_H_
#define HEAVENLYWATERS_ESP8266_SERIALCOMMANDS_H_

typedef void (* GenericFP)(String); //function pointer prototype to a function which takes an 'String' an returns 'void'

typedef struct _cmd
{
	String cmd;
	GenericFP handler;
} cmd_t;


void LED_handler			(String InputArgs);
void GET_IP_ADDR_handler	(String InputArgs1);
void VERBOSE_handler		(String InputArgs1);
void SER_TEST_handler		(String InputArgs);
void ESP_INFO_handler		(String InputArgs);
void PING_handler			(String InputArgs);


void HeavenlyWaters_ESP8266_CommandHandler(String command, String Arguments);
void SerialPrintStat();






#endif /* HEAVENLYWATERS_ESP8266_SERIALCOMMANDS_H_ */