#include "lib.h"

unsigned int Factorial( unsigned int number ) {
    return number <= 1 ? number : Factorial(number-1)*number;
}

int CommandHandler::Add(char command[],int (*pt2Function)(char command[]),char description[]){
    return 0;
}

int CommandHandler::exec(char command[],char arguments[]){
    return 0;
}