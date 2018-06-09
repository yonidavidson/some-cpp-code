#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include "lib.h"

int my_function(char arguments[]){
    return atoi(arguments);
}

TEST_CASE( "Check default Command line parser" ) {
    char key[] = "my_command";
    char arguments[]= "123";
    char description[] = "sample function description";
    handler_function f = my_function;

    CommandHandler *uut = new CommandHandler();
	int add_response = uut->add(key, f, description);
	int response = uut->exec(key, arguments);
    REQUIRE(add_response == 0);
    REQUIRE( response == 123 );
    REQUIRE(!strcmp( description, uut->description(key)));
}

TEST_CASE( "Check Command" ) {
    char key[] = "my_command";
    char arguments[]= "123";
    char description[] = "sample function description";
    handler_function f = my_function;

    Command *uut = new Command(f,description,key);
	int response = uut->exec(arguments);
    REQUIRE( response == 123 );
    REQUIRE(!strcmp( description, uut->description()));
}