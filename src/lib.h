typedef int (*handler_function)(char command[]);

class Command{
private:
    handler_function kept_function;
    char * kept_description;
    char * command;
public:
    Command(handler_function f, char description[], char command[]);
    int isCommand(char command[]);
    int exec(char arguments[]);
    char * description();
};

class CommandHandler
{
 private:
    Command * kept_command;
 public:
    int add(char command[],handler_function f,char description[]);
    int exec(char command[],char arguments[]);
    char *  description(char command[]);
};