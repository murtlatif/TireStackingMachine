/**
 * uart.h
 * Author: Murtaza Latif
 */

#ifndef UART_H
#define UART_H

/********************************* Includes **********************************/
#include <xc.h>
#include "configureBits.h"
#include "lcd.h"
/********************************** Macros ***********************************/
#define UART_READ_TIMEOUT 2
#define UART_WRITE_TIMEOUT 3

#define UART_ErrorHandleWrite(UART_Result, ErrorMsg) {\
    if (UART_Result == UART_WRITE_TIMEOUT) {\
        setStatus(ST_ERROR);\
        setScreen(SC_SEND_ARDUINO_MESSAGE_ERROR);\
        lcd_set_ddram_addr(LCD_LINE3_ADDR);\
        printf(ErrorMsg);\
    }\
}

#define UART_ErrorHandleRead(UART_Result) {\
    if (UART_Result == UART_READ_TIMEOUT) {\
        setStatus(ST_ERROR);\
        setScreen(SC_UART_READ_TIMEOUT_ERROR);\
        break;\
    }\
}

#define UART_Write_With_Error_Handle(MSG, ErrorCode) {\
    if (UART_Write(MSG) == UART_WRITE_TIMEOUT) {\
        setStatus(ST_ERROR);\
        setScreen(SC_SEND_ARDUINO_MESSAGE_ERROR);\
        \
        lcd_set_ddram_addr(LCD_LINE3_ADDR);\
        printf(ErrorCode);\
    }\
}

#define UART_Request_Error_Handling(UART_Result, errorMsg) {\
    if (UART_Result == UART_READ_TIMEOUT) {\
        setStatus(ST_ERROR);\
        setScreen(SC_UART_READ_TIMEOUT_ERROR);\
        break;\
    } else if (UART_Result == UART_WRITE_TIMEOUT) {\
        setStatus(ST_ERROR);\
        setScreen(SC_SEND_ARDUINO_MESSAGE_ERROR);\
\
        lcd_set_ddram_addr(LCD_LINE3_ADDR);\
        printf(errorMsg);\
    }\
}

#define UART_Request_Error_Handling_Not_Recursive(UART_Result, errorMsg) {\
    if (UART_Result == UART_READ_TIMEOUT) {         \
        CURRENT_STATUS = ST_ERROR;                  \
        CURRENT_SCREEN = SC_UART_READ_TIMEOUT_ERROR;\
        displayPage("     ERROR      ",             \
                    "UART read timed ",             \
                    "      out       ",             \
                    "[D] OK          ");            \
        break;                                      \
    } else if (UART_Result == UART_WRITE_TIMEOUT) { \
        CURRENT_STATUS = ST_ERROR;                  \
        CURRENT_SCREEN = SC_SEND_ARDUINO_MESSAGE_ERROR;\
            displayPage("     ERROR      ",         \
                        "Send code failed",         \
                        "                ",         \
                        "[D] OK          ");        \
        lcd_set_ddram_addr(LCD_LINE3_ADDR);         \
        printf(errorMsg);                           \
        break;                                      \
    }                                               \
}

/************************ Public Function Prototypes *************************/
unsigned char UART_Init(long int baudrate);
unsigned char UART_Write(unsigned char data);
unsigned char UART_Write_Text(unsigned char *text);

char UART_Data_Ready(void);
unsigned char UART_Read(unsigned char *requestedData);
unsigned char UART_Read_Text(unsigned char *output, unsigned int length);
unsigned char UART_Read_Short(unsigned short *requestedData);

unsigned char UART_Request_Byte(unsigned char requestCode, unsigned char *requestedByte);
unsigned char UART_Request_Short(unsigned char requestCode, unsigned short *requestedShort);
#endif /* UART_H */