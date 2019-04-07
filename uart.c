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
            return UART_WRITE_TIMEOUT;
        }
    };
    
    __delay_us(100);
    timeoutCounter++;

    TXREG = data;
    return SUCCESSFUL;
}

// Writes multiple bytes to the serial comm.
unsigned char UART_Write_Text(unsigned char *text) {
    for (unsigned char i = 0; text[i] != '\0'; i++) {
        if (UART_Write(text[i]) == UART_WRITE_TIMEOUT) {
            return UART_WRITE_TIMEOUT;
        }
    }

    return SUCCESSFUL;
}

// Checks whether data is ready to receive from the serial comm.
char UART_Data_Ready(void) {
    return RCIF;
}

// Read and return the data in the register
unsigned char UART_Read(unsigned char *requestedData) {
    unsigned short timeoutCounter = 0;

    // If there is no data available after 1.5s, timeout 
    while(!UART_Data_Ready()) {
        if (timeoutCounter == 15000) {
            return UART_READ_TIMEOUT;
        }
    }

    *requestedData = RCREG;
    return SUCCESSFUL;
}

// Read and store data of variable length into an output variable
unsigned char UART_Read_Text(unsigned char *output, unsigned int length) {
    unsigned char i;
    for (i = 0; i < length; i++) {
        if (UART_Read(&(output[i])) == UART_READ_TIMEOUT) {
            return UART_READ_TIMEOUT;
        }
    }

    return SUCCESSFUL;
}

// Write a code into the serial comm. and return the next byte received
unsigned char UART_Request_Byte(unsigned char requestCode, unsigned char *requestedByte) {
    // Write the code to the serial comm.
    if (UART_Write(requestCode) == UART_WRITE_TIMEOUT) {
        return UART_WRITE_TIMEOUT;
    }
    
    // Read the value into requestedByte
    if (UART_Read(requestedByte) == UART_READ_TIMEOUT) {
        return UART_READ_TIMEOUT;
    }

    return SUCCESSFUL;
}

// Write a code into the serial comm. and return the next two bytes received
unsigned char UART_Request_Short(unsigned char requestCode, unsigned short *requestedShort) {
    unsigned char requestedShort_Upper;
    unsigned char requestedShort_Lower;
    
    // Write the code to the serial comm.
    if (UART_Write(requestCode) == UART_WRITE_TIMEOUT) {
        return UART_WRITE_TIMEOUT;
    }

    // Read the upper byte into a temporary variable
    if (UART_Read(&requestedShort_Upper) == UART_READ_TIMEOUT) {
        return UART_READ_TIMEOUT;
    }

    // Read the lower byte into requestedShort
    if (UART_Read(&requestedShort_Lower) == UART_READ_TIMEOUT) {
        return UART_READ_TIMEOUT;
    }

    // Set requestedShort to be the combination of the upper and lower byte
    *requestedShort = (requestedShort_Upper << 8) | requestedShort_Lower;

    return SUCCESSFUL;
}