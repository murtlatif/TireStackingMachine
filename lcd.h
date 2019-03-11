/**
 * @file
 * @author Murtaza Latif
 *
 * Created on January 15th, 2019, 5:31 PM
 *
 * @defgroup CharacterLCD
 * @brief Driver for Hitachi HD44780-based character LCD
 * @{
 */

#ifndef RTC_H
#define RTC_H

/********************************* Includes **********************************/
#include <xc.h>
#include "configureBits.h"
#include <stdbool.h>
#include <stdio.h>

/********************************** Macros ***********************************/
#define RS LATDbits.LATD2          
#define E  LATDbits.LATD3

/** @brief Clears both LCD lines */
#define lcd_clear(){\
	lcdInst(0x01);\
	__delay_ms(7);\
}

 /* Sets cursor position to start of second line. */
 #define lcd_newline() {\
    lcdInst(0xC0);\
    __delay_ms(2);\
 }

/** @brief Sets cursor position to start of first line */
#define lcd_home(){\
	lcdInst(0x80);\
	__delay_ms(2);\
}

/**
 * @brief Sets the cursor's position to a specific display data RAM (DDRAM)
 *        address
 * @param addr The DDRAM address to move the cursor to (min: 0, max: 127)
 * @note The cursor will not be visible at all addresses
 */
#define lcd_set_ddram_addr(addr){\
    lcdInst(0x80 | addr);\
}

/**
 * @brief Backlight and cursor control
 * @param display_on Turns on the backlight if true, otherwise turns it off
 * @param cursor_on Turns on cursor if true, otherwise turns it off
 * @param blink_cursor Blinks the cursor if true, otherwise cursor is static
 */
#define lcd_display_control(\
	display_on,\
	cursor_on,\
	blink_cursor\
)\
{\
	lcdInst(\
        (unsigned char)(8 | (display_on << 2) | (cursor_on << 1) | blink_cursor)\
    );\
}

/******************************** Constants **********************************/
// Display dimensions as seen in the real world (before you use these in your
// code, double-check that they match the size of your LCD)
extern const unsigned char LCD_SIZE_HORZ; /**< Number of visible columns */
extern const unsigned char LCD_SIZE_VERT; /**< Number of visible rows    */

extern const unsigned char LCD_LINE1_ADDR; /**< Address of first line */
extern const unsigned char LCD_LINE2_ADDR; /**< Address of 2nd line   */
extern const unsigned char LCD_LINE3_ADDR; /**< Address of 3rd line   */
extern const unsigned char LCD_LINE4_ADDR; /**< Address of 4th line   */

extern const char * months[];
/********************************** Types ************************************/
/** @brief The directions the display contents and cursor can be shifted */
typedef enum{
	LCD_SHIFT_LEFT = 0, /**< Left shift  */
	LCD_SHIFT_RIGHT = 1 /**< Right shift */
}lcd_direction_e;

/************************ Public Function Prototypes *************************/
/**
 * @brief Sends a command to a display control register
 * @param data The command byte for the Hitachi controller
 */
void lcdInst(char data);

/** @brief Performs the initial setup of the LCD */
void initLCD(void);

/**
 * @brief Moves the cursor in a given direction by numChars characters
 * @param numChars The number of character positions by which the cursor is to
 *        be moved (min: 0, max: 127)
 * @param direction The direction for which the shift is to occur
 */
void lcd_shift_cursor(unsigned char numChars, lcd_direction_e direction);

/**
 * @brief Shifts the display in a given direction by numChars characters
 * @param numChars The number of character positions by which the display
 *        contents are to be shifted (min: 0, max: 127)
 * @param direction The direction for which the shift is to occur
 */
void lcd_shift_display(unsigned char numChars, lcd_direction_e direction);

/**
 * @brief Sends a character to the display for printing
 * @details The familiar C function printf internally calls a function named
 *          "putch" (put character) whenever a character is to be written to
 *          the screen. Here we have chosen to implement putch so that it
 *          sends the character to the LCD, but you can choose to implement it
 *          however you'd like (e.g. send the character over UART, etc.)
 * @param data The character (byte) to be displayed
 */
void putch(char data);

void displayPage(char line1[], char line2[], char line3[], char line4[]);

void displayMenuPage(char line1[], char line2[], char line3[], bool leftPage, bool rightPage);

void displayTime(unsigned char time[]);
/**
 * @}
 */




#endif	/* LCD_H */