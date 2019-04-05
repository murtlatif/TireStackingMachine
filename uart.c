/**
 * @pre The Co-processor is not driving lines on the UART bus (e.g. the JP_TX
 *      and JP_RX jumpers are removed)
 * @pre The character LCD is in an Arduino Nano socket
 * @pre PIC-Arduino link switches are enabled (ON) for D1 of the Arduino (the RX
 *      pin). However, make sure that neither of D0 and D1 are enabled (ON) 
 *      while programming the Arduino Nano
 */

#include "uart.h"

unsigned char UART_Init(const long int baudrate) {
    unsigned int x;
    x = (_XTAL_FREQ - baudrate*64)/(baudrate*64);   //SPBRG for Low Baud Rate
    if (x>255) {                                    //If High Baud Rage Require 
        x = (_XTAL_FREQ - baudrate*16)/(baudrate*16); //SPBRG for High Baud Rate
        BRGH = 1;                                     //Setting High Baud Rate
    }
    if(x<256) {
        SPBRG = x;                                    //Writing SPBRG Register
        SYNC = 0;                                     //Setting Asynchronous Mode, ie UART
        SPEN = 1;                                     //Enables Serial Port
        TRISC7 = 1;                                   //As Prescribed in Datasheet
        TRISC6 = 1;                                   //As Prescribed in Datasheet
        CREN = 1;                                     //Enables Continuous Reception
        TXEN = 1;                                     //Enables Transmission
        __delay_ms(5);                                    // Takes time for TXEN to settle
        return FAIL;                                     //Returns 1 to indicate Successful Completion
  }
  
  return SUCCESS;                                    //Return to indicate UART initialization failed
}

unsigned char UART_Write(char data) {
    unsigned short timeoutCounter = 0;
    while(!TXIF ) {
        if (timeoutCounter == 10000) {
            return FAIL;
        }
    };
    TXREG = data;
    return SUCCESS;
}

char UART_TX_Empty(void) {
    return TRMT;
}

void UART_Write_Text(char *text) {
    int i;
    for (i=0;text[i]!='\0';i++) {
    UART_Write(text[i]);
    }
}

unsigned char UART_Transmit_Yield(unsigned char byteToTransmit) {
    unsigned short timeoutCounter = 0;
    
    while (!TXIF | !TRMT) {
        if (timeoutCounter == 10000) {
            return FAIL;
        }
        
        timeoutCounter++;
        __delay_ms(1);
    }
    
    TXREG = byteToTransmit;
    return SUCCESS;
}

unsigned char UART_Receive(unsigned char *data) {
    if (PIR1bits.RCIF) {        // Check if UART receive interrupt flag is set
        if (PIR1bits.TXIF) {    // Check if TXREG is empty
            *data = RCREG;
            return SUCCESS;
        }
    }
    
    return FAIL;
}

char UART_Data_Ready(void) {
    return RCIF;
}

char UART_Read(void) {
    while(!RCIF);
    return RCREG;
}

void UART_Read_Text(char *Output, unsigned int length) {
    unsigned int i;
    for (int i=0;i<length;i++) {
        Output[i] = UART_Read();
    }
}

char UART_Request_Data(unsigned char requestCode) {
    UART_Write(requestCode);
    return UART_Read();
}