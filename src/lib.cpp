#include "lib.h"
#include <stdio.h>
#include <string.h>

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

//**********

int CommandHandler::add(char command[],handler_function f,char description[]){
    this->kept_command = new Command(f,description,command);
    return 0;
}

int CommandHandler::exec(char command[],char arguments[]){
    return this->kept_command->exec(arguments);
}

char * CommandHandler::description(char commandp[]){
    return this->kept_command->description();
}