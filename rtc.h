/**
 * @file
 * @author Murtaza Latif
 *
 * Created on February 27th, 2019, 2:40 PM
 */

#ifndef RTC_H
#define RTC_H

/********************************* Includes **********************************/
#include <xc.h>
#include "configureBits.h"
#include "I2C.h"

/********************************** Macros ***********************************/
/******************************** Constants **********************************/
/********************************** Types ************************************/
/************************ Public Function Prototypes *************************/
/**
 * @brief Stores the current time into an array
 * @param pTime The size 7 array to store the time.
 */
void readTime(unsigned char pTime[7]);

/**
 * @brief Stores the current time into an array excluding the weekday and year
 * @param pTime The size 5 array to store the time.
 */
void condensedReadTime(unsigned char pTime[5])
#endif	/* RTC_H */