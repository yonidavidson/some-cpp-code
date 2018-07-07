#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"
#include "lib.h"

int function1(char *arguments) {
    return atoi(arguments);
}

int function2(char *arguments) {
    return atoi(arguments)*2;
}

TEST_CASE("Check one function working") {
    char key[] = "my_command";
    char arguments[] = "123";
    char description[] = "sample function description";
    handler_function f = function1;
    CommandHandler *uut = new CommandHandler();
    int add_response = uut->add(key, f, description);

    int exec_response = uut->exec(key, arguments);

    REQUIRE(add_response == 0);
    REQUIRE(exec_response == 123);
    REQUIRE(!strcmp(description, uut->description(key)));
}

TEST_CASE("Check multiple functions execution") {
    char key1[] = "function1";
    char description1[] = "function1";
    handler_function f1 = function1;
    char key2[] = "function2";
    char description2[] = "function2";
    handler_function f2 = function2;

    char arguments[] = "123";

    CommandHandler *uut = new CommandHandler();
    int add_response1 = uut->add(key1, f1, description1);
    int add_response2 = uut->add(key2, f2, description2);

    int exec_response1 = uut->exec(key1, arguments);
    int exec_response2 = uut->exec(key2, arguments);

    REQUIRE(add_response1 == 0);
    REQUIRE(exec_response1 == 123);
    REQUIRE(!strcmp(description1, uut->description(key1)));

    REQUIRE(add_response2 == 0);
    REQUIRE(exec_response2 == 246);
    REQUIRE(!strcmp(description2, uut->description(key2)));
}