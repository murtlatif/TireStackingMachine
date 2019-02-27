/**
 * @file
 * @author Murtaza Latif
 *
 * Created on January 15th, 2019, 5:21 PM
 * @ingroup CharacterLCD
 */

/********************************* Includes **********************************/
#include "rtc.h"

/******************************** Constants **********************************/
enum Months {JAN., FEB.,


/***************************** Private Functions *****************************/

/**
 * @brief Low-level function to send a byte to the display
 * @param data The byte to be sent
 */
static void example(unsigned char test) {
    // Do something here)
}

/***************************** Public Functions ******************************/

void putch(char data){
    RS = 1;
    send_byte((unsigned char)data);
}