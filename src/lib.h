unsigned int Factorial( unsigned int number );

class CommandHandler
{
 private:
 public:
    int Add(char command[],int (*pt2Function)(char command[]),char description[]);
    int exec(char command[],char arguments[]);
}; 