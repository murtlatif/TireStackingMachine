/**
 * @file
 * @author Murtaza Latif
 *
 * Created on February 27th, 2019 at 2:31 PM
 */

/********************************* Includes **********************************/
#include "rtc.h"

/******************************** Constants **********************************/

/***************************** Private Functions *****************************/

/***************************** Public Functions ******************************/
void readTime(unsigned char pTime[7]) {
    // Reset RTC memory pointer
    I2C_Master_Start(); // Start condition
    I2C_Master_Write(0b11010000); // 7 bit RTC address + Write
    I2C_Master_Write(0x00); // Set memory pointer to seconds    
    I2C_Master_Stop(); // Stop condition

    // Read current time
    I2C_Master_Start(); // Start condition
    I2C_Master_Write(0b11010001); // 7 bit RTC address + Read
    for(unsigned char i = 0; i < 6; i++){
        pTime[i] = I2C_Master_Read(ACK); // Read with ACK to continue reading
    }
    pTime[6] = I2C_Master_Read(NACK); // Final Read with NACK
    I2C_Master_Stop(); // Stop condition
}

void condensedReadTime(unsigned char pTime[5]) {
    // Excludes year and weekday
    unsigned char time[7];
    readTime(time);
    
    /* pTime[i]:
     * seconds = 0
     * minutes
     * hours
     * day of month
     * month
     */
    // Store seconds - day of month
    for (char i = 0; i < 4; i++) {
        pTime[i] = time[i];
    }
    
    // Store month
    pTime[4] = time[5];
    
}