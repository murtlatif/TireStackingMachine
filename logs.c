/**
 * @file
 * @author Murtaza Latif
 *
 * Created on February 27th, 2019 at 2:31 PM
 */

/********************************* Includes **********************************/
#include "logs.h"

/******************************** Constants **********************************/

/***************************** Private Functions *****************************/
static inline unsigned char EEPROM_ReadByte(unsigned char eepromAdr) {
    // Reads the data from EEPROM at address eepromAdr
    while (EECON1bits.RD || EECON1bits.WR) { continue; }    // Wait until ready
    EEADR = eepromAdr;      // Set desired address
    EECON1bits.EEPGD = 0;   // Configure to EEPROM memory
    EECON1bits.CFGS = 0;    // Configure to EEPROM memory
    EECON1bits.RD = 1;      // Read the data
    return EEDATA;          // Return data stored in EEDATA.
}

static inline unsigned char EEPROM_WriteByte(unsigned char eepromAdr, unsigned char eepromData) {
    while (EECON1bits.RD || EECON1bits.WR) { continue; }    // Wait until ready
    EEADR = eepromAdr;      // Set desired address
    EEDATA = eepromData;    // Set desired data
    EECON1bits.EEPGD = 0;   // Configure to EEPROM memory
    EECON1bits.CFGS = 0;    // Configure to EEPROM memory
    unsigned char prevGIE = INTCONbits.GIE; // Store interrupt configuration
    INTCONbits.GIE = 0;     // Disable interrupts
    EECON1bits.WREN = 1;    // Allow writes to EEPROM
    EECON2 = 0x55;          // Instruction for writing data
    EECON2 = 0xAA;          // Instruction for writing data
    EECON1bits.WR = 1;      // Instruction for writing data
    INTCONbits.GIE = prevGIE;   // Reset interrupt setting
    while (EECON1bits.WR) { continue; } // Wait for write to complete
    
    EECON1bits.WREN = 0;    // Disable writes to EEPROM
    
    // Verify that byte was written successfully
    if (EEPROM_ReadByte(eepromAdr) == eepromData) {
        return SUCCESS;
    } else {
        return FAIL;
    }
}
/***************************** Public Functions ******************************/
unsigned char setLogCount(unsigned char newLogCount) {
    if (newLogCount > MAX_LOGS) {
        return FAIL;
    }
    
    unsigned char successful = EEPROM_WriteByte(ADDR_LOGCOUNT, newLogCount);
    return successful;
}

unsigned char getLogCount() {
    return EEPROM_ReadByte(ADDR_LOGCOUNT);
}

void storeOperation(unsigned char operation[32],
                    unsigned char startTimeOfOperation[5],
                    unsigned char duration,
                    unsigned char totalSuppliedTires,
                    unsigned char totalNumberOfPoles,
                    unsigned char tiresDeployedOnPole[10],
                    unsigned char tiresOnPoleAfterOperation[10],
                    unsigned short distanceOfPole[10]) {
    
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
    char i = 0;     // Temporary variable for running loops
    unsigned char currentAddr = 0;  // Current address being stored in operation
    unsigned char leftHalfByte = 0x00;  // Stores tire info about Pole 1 + (2n)
    unsigned char rightHalfByte = 0x00; // Stores tire info about Pole 2 + (2n)
    
    // Store BYTE 00 - 04
    for (i = 0; i < 5; i++) {
        operation[currentAddr] = startTimeOfOperation[i];
        currentAddr++;
    }
    
    // Store BYTE 05
    operation[currentAddr] = duration;
    currentAddr++;
    
    // Store BYTE 06
    operation[currentAddr] = ((totalSuppliedTires & 0x0F) << 4) | (totalNumberOfPoles & 0x0F);
    currentAddr++;
    
    // Store BYTE 07 - 11
    for (i = 0; i < totalNumberOfPoles; i++) {
        if (i % 2 == 0) {
            leftHalfByte = ((tiresDeployedOnPole[i] & 0x03) << 2) | (tiresOnPoleAfterOperation[i] & 0x03);
            if (i == totalNumberOfPoles - 1) {
                operation[currentAddr] = (leftHalfByte << 4);
                currentAddr++;
            }
        } else {
            rightHalfByte = ((tiresDeployedOnPole[i] & 0x03) << 2) | (tiresOnPoleAfterOperation[i] & 0x03);
            operation[currentAddr] = (leftHalfByte << 4) | (rightHalfByte);
            currentAddr++;
        }
    }
    
    currentAddr += (totalNumberOfPoles - 10);   // Skip addresses reserved for poles
    
    // Store BYTE 12 - 31
    for (i = 0; i < totalNumberOfPoles; i++) {
        operation[currentAddr] = (distanceOfPole[i] & 0xFF00) >> 8;
        currentAddr++;
        operation[currentAddr] = (distanceOfPole[i] & 0x00FF);
        currentAddr++;
    }
    
}

unsigned char saveOperationIntoLogs(unsigned char operationNumber, 
                                    unsigned char operation[32]) {
    if (operationNumber > MAX_LOGS) {
        return FAIL;
    }
    
    unsigned char result;
    for (char i = 0; i < 32; i++) {
        result = EEPROM_WriteByte((operationNumber * LOG_SIZE) + ADDR_FIRST_LOG + i, operation[i]);
        if (result == FAIL) {
            return FAIL;
        }
    }
    
    return SUCCESS;
    
}

unsigned char getOperationFromLogs(unsigned char operationNumber,
                                   unsigned char operation[32]) {
    if (operationNumber > MAX_LOGS) {
        return FAIL;
    }
    
    for (char i = 0; i < 32; i++) {
        operation[i] = EEPROM_ReadByte((operationNumber * LOG_SIZE) + ADDR_FIRST_LOG + i);
    }
    
    return SUCCESS;
    
}