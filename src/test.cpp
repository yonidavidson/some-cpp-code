#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include "lib.h"

TEST_CASE( "Factorials are computed", "[factorial]" ) {
    REQUIRE( Factorial(1) == 1 );
    REQUIRE( Factorial(2) == 2 );
    REQUIRE( Factorial(3) == 6 );
    REQUIRE( Factorial(10) == 3628800 );
}

int my_function(char arguments[]){
    return int(arguments[0]);
}

TEST_CASE( "Check default Command line parser" ) {
    char command[] = "my_command";
    int a = 10;
    char arguments[2];
    char description[] = "sample function description";
    sprintf(arguments,"%d",a);
    
    CommandHandler *uut = new CommandHandler();
	int add_response = uut->Add(command, my_function, description);
	int response = uut->exec(command, arguments);
    REQUIRE(add_response == 0);
    REQUIRE( response == 10 );
}