/**
 * @pre The Co-processor is not driving lines on the UART bus (e.g. the JP_TX
 *      and JP_RX jumpers are removed)
 * @pre The character LCD is in an Arduino Nano socket
 * @pre PIC-Arduino link switches are enabled (ON) for D1 of the Arduino (the RX
 *      pin). However, make sure that neither of D0 and D1 are enabled (ON) 
 *      while programming the Arduino Nano
 */

#include "uart.h"

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