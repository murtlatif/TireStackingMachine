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

static inline unsigned char EEPROM_WriteByte(unsigned short eepromAdr, unsigned char eepromData) {
    while (EECON1bits.RD || EECON1bits.WR) { continue; }    // Wait until ready
    EEADR = eepromAdr & 0x00FF; // Set low address value (00h - FFh)
    EEADRH = (eepromAdr >> 8);  // Set high address value (0h - 3h) upper 6 bits ignored
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
unsigned char writeAllLogSlots(unsigned short newLogSlots) {
    unsigned char successful = EEPROM_WriteByte(ADDR_LOGSLOTS_LOW, (newLogSlots >> 8) & 0xFF);
    if (successful == FAIL) {
        return FAIL;
    }

    successful = EEPROM_WriteByte(ADDR_LOGSLOTS_HIGH, (newLogSlots & 0xFF));
    return successful;
}

unsigned char setLogSlot(unsigned char slotNumber, logslot_setting setting) {
    // Must be a valid slot
    if (slotNumber > MAX_LOGS) {
        return FAIL;
    }

    // Select logslot address depending on requested slot number
    unsigned char logslot_addr = ADDR_LOGSLOTS_LOW;
    if (slotNumber > 7) {
        logslot_addr = ADDR_LOGSLOTS_HIGH;
    }

    // Retrieve log slot information
    unsigned char logslotList = EEPROM_ReadByte(logslot_addr);

    // Set the slot number depending on the setting
    if (setting == AVAILABLE) {
        logslotList &= ~(1 << (7 - (slotNumber % 8)));
    } else if (setting == USED) {
        logslotList |= 1 << (7 - (slotNumber % 8));
    } else {
        return FAIL;
    }
    
    // Write new logslot list to EEPROM
    unsigned char successful = EEPROM_WriteByte(logslot_addr, logslotList);
    return successful;
}

logslot_setting getLogSlot(unsigned char slotNumber) {
    // Retrieve the correct log slot address based on the slot number
    unsigned char logslot_addr = ADDR_LOGSLOTS_LOW;
    if (slotNumber > 7) {
        logslot_addr = ADDR_LOGSLOTS_HIGH;
    }

    // Retrieve the log slot list from EEPROM
    unsigned char logslotList = EEPROM_ReadByte(logslot_addr);

    // Return the bit corresponding to the slot number
    return (logslotList >> (7 - (slotNumber % 8))) & 1;
}

unsigned char getSlotsUsed(void) {
    unsigned short logslotValues = (EEPROM_ReadByte(ADDR_LOGSLOTS_LOW) << 8) | EEPROM_ReadByte(ADDR_LOGSLOTS_HIGH);
    unsigned char count = 0;

    // Count the number of 1's in the list of log slots
    while (logslotValues > 0) {
        if ((logslotValues & 1) == USED) {
            count++;
        }
        logslotValues >>= 1;
    }

    return count;
}

void storeCondensedOperation(Operation op) {
    char i = 0;     // Temporary variable for running loops
    unsigned char currentAddr = 0;  // Current address being stored in operation
    unsigned char leftHalfByte = 0x00;  // Stores tire info about Pole 1 + (2n)
    unsigned char rightHalfByte = 0x00; // Stores tire info about Pole 2 + (2n)
    
    // Store BYTE 00 - 04
    for (i = 0; i < 5; i++) {
        op.condensedOperation[currentAddr] = op.startTime[i];
        currentAddr++;
    }
    
    // Store BYTE 05
    op.condensedOperation[currentAddr] = op.duration;
    currentAddr++;
    
    // Store BYTE 06
    op.condensedOperation[currentAddr] = ((op.totalSuppliedTires & 0x0F) << 4) | (op.totalNumberOfPoles & 0x0F);
    currentAddr++;
    
    // Store BYTE 07 - 11
    for (i = 0; i < op.totalNumberOfPoles; i++) {
        if (i % 2 == 0) {
            leftHalfByte = ((op.tiresDeployedOnPole[i] & 0x03) << 2) | (op.tiresOnPoleAfterOperation[i] & 0x03);
            if (i == op.totalNumberOfPoles - 1) {
                op.condensedOperation[currentAddr] = (leftHalfByte << 4);
                currentAddr++;
            }
        } else {
            rightHalfByte = ((op.tiresDeployedOnPole[i] & 0x03) << 2) | (op.tiresOnPoleAfterOperation[i] & 0x03);
            op.condensedOperation[currentAddr] = (leftHalfByte << 4) | (rightHalfByte);
            currentAddr++;
        }
    }
    
    currentAddr += (op.totalNumberOfPoles - 10);   // Skip addresses reserved for poles
    
    // Store BYTE 12 - 31
    for (i = 0; i < op.totalNumberOfPoles; i++) {
        op.condensedOperation[currentAddr] = (op.distanceOfPole[i] & 0xFF00) >> 8;
        currentAddr++;
        op.condensedOperation[currentAddr] = (op.distanceOfPole[i] & 0x00FF);
        currentAddr++;
    }
    
}

void unpackCondensedOperation(Operation op) {
    unsigned char currentAddr = 0;
    for (char i = 0; i < 5; i++) {
        op.startTime[i] = op.condensedOperation[currentAddr];
        currentAddr++;
    }
    
    op.duration = op.condensedOperation[currentAddr];
    currentAddr++;

    op.totalSuppliedTires = op.condensedOperation[currentAddr] >> 4;
    op.totalNumberOfPoles = op.condensedOperation[currentAddr] & 0x0F;
    currentAddr++;

    for (char i = 0; i < op.totalNumberOfPoles; i++) {
        if (i % 2 == 0) {
            op.tiresDeployedOnPole[i] = op.condensedOperation[currentAddr] >> 6;
            op.tiresOnPoleAfterOperation[i] = (op.condensedOperation[currentAddr] >> 4) & 0x3;
            if (i == op.totalNumberOfPoles - 1) {
                currentAddr++;
            }
        } else {
            op.tiresDeployedOnPole[i] = (op.condensedOperation[currentAddr] >> 2) & 0x3;
            op.tiresOnPoleAfterOperation[i] = op.condensedOperation[currentAddr] & 0x3;
            currentAddr++;
        }
    }

    currentAddr += ((10 - op.totalNumberOfPoles)) / 2;

    for (char i = 0; i < op.totalNumberOfPoles; i++) {
        op.distanceOfPole[i] = (op.condensedOperation[currentAddr] << 8) | (op.condensedOperation[currentAddr + 1]);
        currentAddr += 2;
    }

}

unsigned char saveCondensedOperationIntoLogs(unsigned char slotNumber, Operation op) {

    if (slotNumber >= MAX_LOGS) {
        return FAIL;
    }
    
    unsigned char result;
    storeCondensedOperation(op);

    for (char i = 0; i < 32; i++) {
        result = EEPROM_WriteByte((slotNumber * LOG_SIZE) + ADDR_FIRST_LOG + i, op.condensedOperation[i]);
        if (result == FAIL) {
            return FAIL;
        }
    }
    
    // Write log slot as used
    result = setLogSlot(slotNumber, USED);
    return result;   
}

unsigned char getCondensedOperationFromLogs(Operation op, unsigned char slotNumber) {
    if (slotNumber > MAX_LOGS) {
        return FAIL;
    }
    
    for (char i = 0; i < 32; i++) {
        op.condensedOperation[i] = EEPROM_ReadByte((slotNumber * LOG_SIZE) + ADDR_FIRST_LOG + i);
    }
    
    return SUCCESS;
    
}