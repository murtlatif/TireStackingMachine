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
#define UART_READ_TIMEOUT 2
#define UART_WRITE_TIMEOUT 3

#define UART_Write_With_Error_Handle(MSG, ErrorCode) {\
    if (UART_Write(MSG) == UART_WRITE_TIMEOUT) {\
        setStatus(ST_ERROR);\
        setScreen(SC_SEND_ARDUINO_MESSAGE_ERROR);\
        \
        lcd_set_ddram_addr(LCD_LINE3_ADDR);\
        printf(ErrorCode);\
    }\
}

/************************ Public Function Prototypes *************************/
unsigned char UART_Init(long int baudrate);
unsigned char UART_Write(unsigned char data);
unsigned char UART_Write_Text(unsigned char *text);

char UART_Data_Ready(void);
unsigned char UART_Read(unsigned char *requestedData);
unsigned char UART_Read_Text(unsigned char *output, unsigned int length);

unsigned char UART_Request_Byte(unsigned char requestCode, unsigned char *requestedByte);
unsigned char UART_Request_Short(unsigned char requestCode, unsigned short *requestedShort);
#endif /* UART_H */