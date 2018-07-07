#include "lib.h"
#include <stdio.h>
#include <string.h>

Command::Command(handler_function f, char description[], char command[]) {
    this->kept_function = f;
    this->kept_description = description;
    this->command = command;
}

int Command::isCommand(char command[]) {
    return (!strcmp(this->command, command));
}

int Command::exec(char arguments[]) {
    return this->kept_function(arguments);
}

char *Command::description() {
    return this->kept_description;
}

//**********

int CommandHandler::add(char command[], handler_function f, char description[]) {
    this->commands[this->num_of_functions] = new Command(f, description, command);
    this->num_of_functions = this->num_of_functions + 1;
    return 0;
}

int CommandHandler::exec(char command[], char arguments[]) {
    int index = this->getCommandIndex(command);
    return this->commands[index]->exec(arguments);
}

char *CommandHandler::description(char command[]) {
    int index = this->getCommandIndex(command);
    return this->commands[index]->description();
}

int CommandHandler::getCommandIndex(char command[]) {
    for( int i = 0; i < this->num_of_functions; i = i + 1 ) {
        int is_command = this->commands[i]->isCommand(command);
        if(is_command){
            return i;
        }
    }
    return -1;
}
