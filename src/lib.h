typedef int (*handler_function)(char command[]);

class CommandHandler
{
 private:
    handler_function kept_function;
 public:
    int Add(char command[],handler_function f,char description[]);
    int exec(char command[],char arguments[]);
}; 