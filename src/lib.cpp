#include "lib.h"

int CommandHandler::Add(char command[],handler_function f,char description[]){
    kept_function = f;
    return 0;
}

int CommandHandler::exec(char command[],char arguments[]){
    return kept_function(arguments);
}