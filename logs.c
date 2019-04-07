/**
 * logs.c
 * Author: Murtaza Latif
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
        return SUCCESSFUL;
    } else {
        return UNSUCCESSFUL;
    }
}
/***************************** Public Functions ******************************/
unsigned char getLogSlot(unsigned char slotNumber) {
    // Checks the slot number to see if the slot is available to store an operation
    if (slotNumber >= MAX_LOGS) {
        return UNSUCCESSFUL;
    }

    // Check the first byte of the operation to see if there is an operation saved
    if (EEPROM_ReadByte == SLOT_USED) {
        return SLOT_USED;
    }

    return SLOT_AVAILABLE;
}

unsigned char getSlotsAvailable(void) {
    // Return the number of save log slots available

    unsigned char count = 0;    // How many slots are available
    unsigned char i;            // iterative variable
    for (i = 0; i < MAX_LOGS; i++) {
        if (getLogSlot(i) == SLOT_AVAILABLE) {
            count++;
        }
    }

    return count;
}

unsigned char storeOperationIntoLogs(Operation op, unsigned char slotNumber) {
    // Do not allow storage of logs past the maximum limit
    if (slotNumber >= MAX_LOGS) {
        return UNSUCCESSFUL;
    }

    unsigned char result;   // Used to return SUCCESSFUL/UNSUCCESSFUL
    unsigned char i;        // Iterative variable
    unsigned short currentMemoryAddr = ADDR_FIRST_LOG + (slotNumber * LOG_SIZE);    // Keeps track of current memory address

    // Flag that the operation is saved into logs
    op.savedIntoLogs = true;

    // Store byte [0] as a true (used to check whether an operation is saved in a specifed slot)
    result = EEPROM_WriteByte(currentMemoryAddr, op.savedIntoLogs);
    stopIfFailed(result);
    currentMemoryAddr++;

    // Store bytes [1, 5] as startTime
    for (i = 0; i < 5; i++) {
        result = EEPROM_WriteByte(currentMemoryAddr, op.startTime[i])
        stopIfFailed(result);
        currentMemoryAddr++;
    }

    // Store byte [6] as duration
    result = EEPROM_WriteByte(currentMemoryAddr, op.duration);
    stopIfFailed(result);
    currentMemoryAddr++;

    // Store byte [7] as total supplied tires
    result = EEPROM_WriteByte(currentMemoryAddr, op.totalSuppliedTires);
    stopIfFailed(result);
    currentMemoryAddr++;

    // Store byte [8] as total number of poles detected
    result = EEPROM_WriteByte(currentMemoryAddr, op.totalNumberOfPoles);
    stopIfFailed(result);
    currentMemoryAddr++;

    // Store bytes [9, 18] as tires deployed on each pole
    for (i = 0; i < 10; i++) {
        result = EEPROM_WriteByte(currentMemoryAddr, op.tiresDeployedOnPole[i]);
        stopIfFailed(result);
        currentMemoryAddr++;
    }

    // Stores bytes [19, 28] as total tires on each pole after operation
    for (i = 0; i < 10; i++) {
        result = EEPROM_WriteByte(currentMemoryAddr, op.tiresOnPoleAfterOperation[i]);
        stopIfFailed(result);
        currentMemoryAddr++;
    }

    // Stores bytes [29, 48] as the distances of each pole after operation
    for (i = 0; i < 10; i++) {
        // Each distance is a short value (2 bytes), store most significant byte first
        result = EEPROM_WriteByte(currentMemoryAddr, op.tiresOnPoleAfterOperation[i] >> 8);
        stopIfFailed(result);
        currentMemoryAddr++;

        result = EEPROM_WriteByte(currentMemoryAddr, op.tiresOnPoleAfterOperation[i] & 0xFF);
        stopIfFailed(result);
        currentMemoryAddr++;
    }

    return SUCCESSFUL;

}

unsigned char getOperationFromLogs(Operation *op, unsigned char slotNumber) {
    // If the log slot is out of range or there are no operations saved, return UNSUCCESSFUL
    unsigned char logSlotStatus = getLogSlot(slotNumber);
    if (logSlotStatus == UNSUCCESSFUL || logSlotStatus == SLOT_AVAILABLE) {
        return UNSUCCESSFUL;
    }

    // Initialize the current memory address (add 1 to ignore the "saved in logs" byte)
    unsigned short currentMemoryAddr = (LOG_SIZE * slotNumber) + ADDR_FIRST_LOG + 1;
    unsigned char i;    // Reused iterative variable

    // Read 5 bytes for the start time
    for (i = 0; i < 5; i++) {
        op->startTime[i] = EEPROM_ReadByte(currentMemoryAddr);
        currentMemoryAddr++;
    }

    // Read 1 byte for the duration
    op->duration = EEPROM_ReadByte(currentMemoryAddr);
    currentMemoryAddr++;

    // Read 1 byte for the total number of tires supplied
    op->totalSuppliedTires = EEPROM_ReadByte(currentMemoryAddr);
    currentMemoryAddr++;

    // Read 1 byte for the total number of poles detected
    op->totalNumberOfPoles = EEPROM_ReadByte(currentMemoryAddr);
    currentMemoryAddr++;

    // Read 10 bytes for the number of tires deployed on each pole
    for (i = 0; i < 10; i++) {
        op->tiresDeployedOnPole[i] = EEPROM_ReadByte(currentMemoryAddr);
        currentMemoryAddr++;
    }

    // Read 10 bytes for the number of tires on each pole after the operation
    for (i = 0; i < 10; i++) {
        op->tiresOnPoleAfterOperation[i] = EEPROM_ReadByte(currentMemoryAddr);
        currentMemoryAddr++;
    }

    // Read 20 bytes (10 short values, 2 bytes each) for the positions of each pole
    for (i = 0; i < 10; i++) {

        // Store the MSB into a short
        unsigned short position = EEPROM_ReadByte(currentMemoryAddr) << 8;
        currentMemoryAddr++;

        op->distanceOfPole[i] = position | EEPROM_ReadByte(currentMemoryAddr);
        currentMemoryAddr++;
    }

    return SUCCESSFUL;
}