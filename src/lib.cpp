#include "lib.h"

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