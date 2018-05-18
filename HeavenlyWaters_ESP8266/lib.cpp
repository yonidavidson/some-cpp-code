#include "lib.h"

unsigned int Factorial( unsigned int number ) {
    return number <= 1 ? number : Factorial(number-1)*number;
}

int SerialPortalHandler::SerialStringBuilder(char* InputBuffer, int InputBufferSize, int NextPosition){
    return 0;
}
int SerialPortalHandler::CheckSerialBuffer(char* InputBuffer, int InputBufferSize, int NextPosition){
    return 0;
}
int SerialPortalHandler::ProcessInputBuffer(char* InputBuffer, int InputBufferSize, char* CommandBuffer,
                                             int CommandBufferSize, char* ArgumentBuffer, 
                                             int ArgumentBufferSize)
{
    return 0;

}