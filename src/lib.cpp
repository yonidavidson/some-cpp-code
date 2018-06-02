#include "lib.h"

unsigned int Factorial( unsigned int number ) {
    return number <= 1 ? number : Factorial(number-1)*number;
}

int CommandHandler::Add(char command[],handler_function f,char description[]){
    kept_function = f;
    return 0;
}

int CommandHandler::exec(char command[],char arguments[]){
    return kept_function(arguments);
}