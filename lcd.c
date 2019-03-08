/**
 * @file
 * @author Murtaza Latif
 *
 * Created on January 15th, 2019, 5:21 PM
 * @ingroup CharacterLCD
 */

/********************************* Includes **********************************/
#include "lcd.h"

/******************************** Constants **********************************/
const unsigned char LCD_SIZE_HORZ = 16;
const unsigned char LCD_SIZE_VERT = 4;

const unsigned char LCD_LINE1_ADDR = 0;
const unsigned char LCD_LINE2_ADDR = 64;
const unsigned char LCD_LINE3_ADDR = 16;
const unsigned char LCD_LINE4_ADDR = 80;

/***************************** Private Functions *****************************/
/**
 * @brief Pulses the LCD register enable signal, which causes the LCD to latch
 *        the data on LATD. Interrupts are disabled during this pulse to
 *        guarantee that the timing requirements of the LCD's protocol are met
 */
static inline void pulse_e(void){
    unsigned char interruptState = INTCONbits.GIE;
    di();
    E = 1;
    // This first delay only needs to be 1 microsecond in theory, but 25 was
    // selected experimentally to be safe
    __delay_us(25);
    E = 0;
    __delay_us(100);
    INTCONbits.GIE = interruptState;
}

/**
 * @brief Low-level function to send 4 bits to the display
 * @param data The byte whose 4 least-significant bits are to be sent to the LCD
 */
static void send_nibble(unsigned char data){
    // Send the 4 least-significant bits
    LATD = (unsigned char)(LATD & 0x0F); // Clear LATD[7:4]
    LATD = (unsigned char)((data << 4) | LATD); // Write data[3:0] to LATD[7:4]
    pulse_e();
}

/**
 * @brief Low-level function to send a byte to the display
 * @param data The byte to be sent
 */
static void send_byte(unsigned char data){
    // Send the 4 most-significant bits
    send_nibble(data >> 4);
    
    // Send the 4 least-significant bits
    send_nibble(data);
}

/***************************** Public Functions ******************************/
void lcdInst(char data){
    RS = 0;
    send_byte(data);
}

void initLCD(void){
    __delay_ms(15);
    
    RS = 0;
    // Set interface length to 4 bits wide (pg. 46 of data sheet)
    send_nibble(0b0011);
    __delay_ms(5);
    send_nibble(0b0011);
    __delay_us(150);
    send_byte(0b00110010);
    
    send_byte(0b00101000); // Set N = number of lines (1 or 2) and F = font
    send_byte(0b00001000); // Display off
    send_byte(0b00000001); // Display clear
    __delay_ms(5);
    send_byte(0b00000110); // Entry mode set
    
    // Enforce on: display, cursor, and cursor blinking
    lcd_display_control(true, false, false);
}

void lcd_shift_cursor(unsigned char numChars, lcd_direction_e direction){
    for(unsigned char n = numChars; n > 0; n--){
        lcdInst((unsigned char)(0x10 | (direction << 2)));
    }
}

void lcd_shift_display(unsigned char numChars, lcd_direction_e direction){
    for(unsigned char n = numChars; n > 0; n--){
        lcdInst((unsigned char)(0x18 | (direction << 2)));
    }
}

void putch(char data){
    RS = 1;
    send_byte((unsigned char)data);
}

// Display functions
void displayPage(char line1[], char line2[], char line3[], char line4[]) {
    // Displays text on the LCD screen
    lcd_clear();
    lcd_home();
    
    printf(line1);
    lcd_set_ddram_addr(LCD_LINE2_ADDR);
    printf(line2);
    lcd_set_ddram_addr(LCD_LINE3_ADDR);
    printf(line3);
    lcd_set_ddram_addr(LCD_LINE4_ADDR);
    printf(line4);
}

void displayMenuPage(char line1[], char line2[], char line3[], bool leftPage, bool rightPage) {
    // Displays text on the LCD screen and displays a menu at the bottom
    // depending on whether leftPage and rightPage are true/false.
    displayPage(line1, line2, line3, "");
    if (leftPage) {
        printf("%c[*] ", 0b01111111);
    } else {
        printf("     ");
    }
    
    printf("BCK[0]");
    
    if (rightPage) {
        printf(" [#]%c", 0b01111110);
    }
}

void displayTime(unsigned char time[]) {
    
    // Convert time to decimal from 1-12
    if (time[5] > 0x09) {
        time[5] = time[5] - 6;
    }
    
    lcd_set_ddram_addr(LCD_LINE2_ADDR);
    printf("%s ", months[time[5] - 1]);     // Month
    printf("%02X", time[4]);                // Date
    if ((time[4] & 0x0F) <= 3) {              // Date Suffix
        printf("%s", dateSuffix[(time[4] & 0x0F)]);
    } else {
        printf("th");
    }
    printf(", 20%02X", time[6]);            // Year
    
    lcd_set_ddram_addr(LCD_LINE3_ADDR);
    unsigned char hourInAmPm;
    if (time[2] > 0x12) {
        
    }
    printf("%02X %02X:", time[2], time[2] > 0x12 ? ((((time[2] >> 4) - 1) << 4) | (time[2] & 0xF)) : time[2]); // Hour
    printf("%02X:", time[1]);                       // Minute
    printf("%02X ", time[0]);                       // Second
    printf("%s  ", time[2] > 0x12 ? "PM" : "AM");    // AM/PM
}