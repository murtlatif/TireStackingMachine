/**
 * logs.h
 * Author: Murtaza Latif
 */

#ifndef LOGS_H
#define LOGS_H

/********************************* Includes **********************************/
#include <xc.h>
#include <stdbool.h>
#include "configureBits.h"
/********************************** Macros ***********************************/
#define ADDR_FIRST_LOG 0x00   // the address of the first log

#define LOG_SIZE 49      // the size (bytes) that one operation takes
#define MAX_LOGS 6       // maximum number of logs that can be stored

#define SLOT_AVAILABLE 4 // value to represent an available log slot
#define SLOT_USED 5      // value to represent a used log slot

#define stopIfUnsuccessful(RESULT) {    \
     if (RESULT == UNSUCCESSFUL) {      \
          return UNSUCCESSFUL;          \
     }                                  \
}
/******************************** Constants **********************************/

/********************************** Types ************************************/
typedef struct Operation {
     // Stored information
     bool savedIntoLogs;
     unsigned char startTime[5];
     unsigned char duration;
     unsigned char totalSuppliedTires;
     unsigned char totalNumberOfPoles;
     unsigned char tiresDeployedOnPole[10];
     unsigned char tiresOnPoleAfterOperation[10];
     unsigned short distanceOfPole[10];

     // Unstored (temporary) information
     unsigned char tiresRemaining;
     unsigned char saveSlot;
     float position;

} Operation;

/************************ Public Function Prototypes *************************/
unsigned char getLogSlot(unsigned char slotNumber);

unsigned char getSlotsAvailable(void);

unsigned char storeOperationIntoLogs(Operation op, unsigned char slotNumber);

unsigned char getOperationFromLogs(Operation *op, unsigned char slotNumber);

#endif	/* LOGS_H */