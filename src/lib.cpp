#include "lib.h"
#include <stdio.h>
#include <string.h>

int CommandHandler::add(char command[],handler_function f,char description[]){
    this->kept_function = f;
    this->kept_description = description;
    return 0;
}

int CommandHandler::exec(char command[],char arguments[]){
    return kept_function(arguments);
}

char * CommandHandler::description(char commandp[]){
    return this->kept_description;
}

Command::Command(handler_function f, char description[], char command[]){
    this->kept_function = f;
    this->kept_description = description;
    this->command = command;
}

int Command::isCommand(char command[]){
    return (!strcmp( this->command,command));
}

int Command::exec(char arguments[]){
    return this->kept_function(arguments);
}

char * Command::description(){
    return this->kept_description;
}