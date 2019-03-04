/**
 * @file
 * @author Murtaza Latif
 *
 * Created on January 15th, 2019, 5:31 PM
 */

#ifndef LOGS_H
#define LOGS_H

/********************************* Includes **********************************/

/********************************** Macros ***********************************/
#define SUCCESS 1
#define FAIL 0

#define ADDR_LOGCOUNT 0x00
#define ADDR_FIRST_LOG 0x01
#define MAX_LOGS 12
/******************************** Constants **********************************/


/********************************** Types ************************************/

/************************ Public Function Prototypes *************************/
unsigned char setLogCount(unsigned char newLogCount);

unsigned char getLogCount();

void storeOperation(unsigned char operation[32],
                    unsigned char startTimeOfOperation[5],
                    unsigned char duration,
                    unsigned char totalSuppliedTires,
                    unsigned char totalNumberOfPoles,
                    unsigned char tiresDeployedOnPole[10],
                    unsigned char tiresOnPoleAfterOperation[10],
                    unsigned short distanceOfPole[10]);
    
    /* OPERATION STORAGE LEGEND:
     *  BYTE 00 - 04: Start time of operation
     * BYTE 00: startTimeOfOperation[0] (seconds)
     * BYTE 01: startTimeOfOperation[1] (minutes)
     * BYTE 02: startTimeOfOperation[2] (hours)
     * BYTE 03: startTimeOfOperation[3] (day of month)
     * BYTE 04: startTimeOfOperation[4] (month)
     * 
     * BYTE 05: duration (time elapsed during operation) 
     * 
     * BYTE 06: totalSuppliedTires | totalNumberOfPoles (0xAB)
     * 
     *  BYTE 07 - 11: tiresDeployedOnPole | tiresOnPoleAfterOperation for 2 poles (0bD1A1D2A2)
     * BYTE 07: tiresDeployedOnPole[0] | tiresOnPoleAfterOperation[0] | 
     *          tiresDeployedOnPole[1] | tiresOnPoleAfterOperation[1]
     * BYTE 08: tiresDeployedOnPole[2] | tiresOnPoleAfterOperation[2] | 
     *          tiresDeployedOnPole[3] | tiresOnPoleAfterOperation[3]
     * BYTE 09: tiresDeployedOnPole[4] | tiresOnPoleAfterOperation[4] | 
     *          tiresDeployedOnPole[5] | tiresOnPoleAfterOperation[5]
     * BYTE 10: tiresDeployedOnPole[6] | tiresOnPoleAfterOperation[6] | 
     *          tiresDeployedOnPole[7] | tiresOnPoleAfterOperation[7]
     * BYTE 11: tiresDeployedOnPole[8] | tiresOnPoleAfterOperation[8] | 
     *          tiresDeployedOnPole[9] | tiresOnPoleAfterOperation[9]
     * 
     * 
     * 
     *  BYTE 12 - 31: distanceOfPole (even bytes are upper two bytes)
     * BYTE 12: (distanceOfPole[0] & 0xFF00) >> 8
     * BYTE 13: distanceOfPole[0] & 0x00FF
     * BYTE 14: (distanceOfPole[1] & 0xFF00) >> 8
     * BYTE 15: distanceOfPole[1] & 0x00FF
     * BYTE 16: (distanceOfPole[2] & 0xFF00) >> 8
     * BYTE 17: distanceOfPole[2] & 0x00FF
     * BYTE 18: (distanceOfPole[3] & 0xFF00) >> 8
     * BYTE 19: distanceOfPole[3] & 0x00FF
     * BYTE 20: (distanceOfPole[4] & 0xFF00) >> 8
     * BYTE 21: distanceOfPole[4] & 0x00FF
     * BYTE 22: (distanceOfPole[5] & 0xFF00) >> 8
     * BYTE 23: distanceOfPole[5] & 0x00FF
     * BYTE 24: (distanceOfPole[6] & 0xFF00) >> 8
     * BYTE 25: distanceOfPole[6] & 0x00FF
     * BYTE 26: (distanceOfPole[7] & 0xFF00) >> 8
     * BYTE 27: distanceOfPole[7] & 0x00FF
     * BYTE 28: (distanceOfPole[8] & 0xFF00) >> 8
     * BYTE 29: distanceOfPole[8] & 0x00FF
     * BYTE 30: (distanceOfPole[9] & 0xFF00) >> 8
     * BYTE 31: distanceOfPole[9] & 0x00FF
     *
     */

unsigned char saveOperationIntoLogs(unsigned char operationNumber, 
                                    unsigned char operation[32]);

unsigned char getOperationFromLogs(unsigned char operationNumber,
                                   unsigned char operation[32]);

#endif	/* LOGS_H */