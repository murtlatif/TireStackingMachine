/**
 * uart.h
 * Author: Murtaza Latif
 */

#ifndef UART_H
#define UART_H

/********************************* Includes **********************************/
#include <xc.h>
#include "configureBits.h"

/********************************** Macros ***********************************/


/************************ Public Function Prototypes *************************/
unsigned char UART_Init(long int baudrate);
unsigned char UART_Write(unsigned char data);
void UART_Write_Text(unsigned char *text);

char UART_Data_Ready(void);
char UART_Read(void);
void UART_Read_Text(char *output, unsigned int length);

char UART_Request_Byte(unsigned char requestCode);
short UART_Request_Short(unsigned char requestCode);
#endif /* UART_H */