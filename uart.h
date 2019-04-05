#ifndef UART_H
#define UART_H

/********************************* Includes **********************************/
#include <xc.h>
#include "configureBits.h"

/********************************** Macros ***********************************/


/************************ Public Function Prototypes *************************/
unsigned char UART_Init(const long int baudrate);
unsigned char UART_Write(char data);
unsigned char UART_Transmit_Yield(unsigned char byteToTransmit);
char UART_Read(void);
char UART_Data_Ready(void);
void UART_Read_Text(char *Output, unsigned int length);
unsigned char UART_Receive(unsigned char *data);
char UART_Request_Data(unsigned char requestCode);
#endif /* UART_H */