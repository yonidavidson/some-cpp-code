unsigned int Factorial( unsigned int number );

class SerialPortalHandler
{
 private:
 public:
    int SerialStringBuilder(char* InputBuffer, int InputBufferSize, int NextPosition);
    int CheckSerialBuffer(char* InputBuffer, int InputBufferSize, int NextPosition);
    int ProcessInputBuffer(char* InputBuffer, int InputBufferSize, char* CommandBuffer, int CommandBufferSize, char* ArgumentBuffer, int ArgumentBufferSize);
}; 