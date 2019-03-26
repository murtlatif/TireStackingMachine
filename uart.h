#ifndef UART_H
#define UART_H

/********************************* Includes **********************************/
#include <xc.h>
#include "configureBits.h"

/********************************** Macros ***********************************/


/************************ Public Function Prototypes *************************/
unsigned char UART_Transmit_Yield(unsigned char byteToTransmit);

unsigned char UART_Receive(unsigned char *data);

#endif /* UART_H */