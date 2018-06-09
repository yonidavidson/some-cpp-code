typedef int (*handler_function)(char command[]);

class CommandHandler
{
 private:
    handler_function kept_function;
    char * kept_description;
 public:
    int add(char command[],handler_function f,char description[]);
    int exec(char command[],char arguments[]);
    char *  description(char command[]);
}; 