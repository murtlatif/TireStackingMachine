/**
 * uart.c
 * Author: Murtaza Latif
 */

#include "uart.h"

unsigned char UART_Init(long int baudrate) {
    unsigned int x;

    x = (_XTAL_FREQ - baudrate*64) / (baudrate*64);     // SPBRG for Low Baud Rate
    if (x > 255) {                                      // If High Baud Rage Require 
        x = (_XTAL_FREQ - baudrate*16) / (baudrate*16); // SPBRG for High Baud Rate
        BRGH = 1;                                       // Setting High Baud Rate
    }

    if(x < 256) {
        SPBRG = x;      // Writing SPBRG Register
        SYNC = 0;       // Setting Asynchronous Mode, i.e. UART
        SPEN = 1;       // Enables Serial Port
        TRISC7 = 1;     // As Prescribed in Datasheet
        TRISC6 = 1;     // As Prescribed in Datasheet
        CREN = 1;       // Enables Continuous Reception
        TXEN = 1;       // Enables Transmission
        __delay_ms(5);  // Takes time for TXEN to settle

        return SUCCESSFUL;  // return to indicate a successful completion
  }
  
  return UNSUCCESSFUL;  //Return to indicate UART initialization failed
}

// Writes a byte to the serial comm.
unsigned char UART_Write(unsigned char data) {
    unsigned short timeoutCounter = 0;

    // If the byte cannot be written for more than 1.5 seconds, return UNSUCCESSFUL
    while(!TXIF) {
        if (timeoutCounter == 15000) {
            return UNSUCCESSFUL;
        }
    };

    __delay_us(100);
    timeoutCounter++;

    TXREG = data;
    return SUCCESSFUL;
}

// Writes multiple bytes to the serial comm.
void UART_Write_Text(unsigned char *text) {
    for (unsigned char i = 0; text[i] != '\0'; i++) {
        UART_Write(text[i]);
    }
}

// Checks whether data is ready to receive from the serial comm.
char UART_Data_Ready(void) {
    return RCIF;
}

// Read and return the data in the register
unsigned char UART_Read(void) {
    while(!RCIF);
    return RCREG;
}

// Read and store data of variable length into an output variable
void UART_Read_Text(char *output, unsigned int length) {
    unsigned char i;
    for (i = 0; i < length; i++) {
        output[i] = UART_Read();
    }
}

// Write a code into the serial comm. and return the next byte received
char UART_Request_Byte(unsigned char requestCode) {
    UART_Write(requestCode);
    return UART_Read();
}

// Write a code into the serial comm. and return the next two bytes received
short UART_Request_Short(unsigned char requestCode) {
    short requestedData;
    UART_Write(requestCode);
    requestedData = UART_Read() << 8;

    return requestedData | UART_Read();
}