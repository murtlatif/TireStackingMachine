/**
 * @file
 * @author Murtaza Latif
 * 
 * Created on January 12th, 2019, 5:15 PM
 * 
 * Recreated after accidental deletion on February 26th, 2019 at 7:25 PM.
 */

//** INCLUDES **//
#include <xc.h>
#include <stdio.h>
#include <stdbool.h>
#include <I2C.h>
#include "configureBits.h"
#include "lcd.h"
#include "logs.h"
#include "rtc.h"

//** MACROS **//

//** CONSTANTS **//
// Constant values
const char keys[] = "123A456B789C*0#D";

// Enumerations
typedef enum {
    ST_STANDBY = 0
} Status;

typedef enum {
            SC_STANDBY = 0,
            SC_MENU,
            SC_DEBUG,
            SC_ABOUT,
            SC_LOGS_MENU,
            SC_LOGS_VIEW,
} Screen;

// Number to String Arrays
static const char * months[] = {"Jan. ",
                                "Feb. ",
                                "March",
                                "April",
                                "May  ",
                                "June ",
                                "July ",
                                "Aug. ",
                                "Sept.",
                                "Oct. ",
                                "Nov. ",
                                "Dec. "};

static const char * dateSuffix[] = {"th", "st", "nd", "rd"};

//** VARIABLES **//
// State variables
Status CURRENT_STATUS;
Screen CURRENT_SCREEN;

// Interrupt Variables
volatile bool key_was_pressed = false;
volatile bool emergency_stop_pressed = false;
volatile unsigned char key;

// RTC Variables
const char valueToWriteToRTC[7] = {
    0x00, // Seconds 
    0x00, // Minutes
    0x22, // Hours (set to 24 hour mode)
    0x03, // Weekday
    0x26, // Day of month
    0x02, // Month
    0x19  // Year
};

//** PROTOTYPES **//
// State Functions
void initialize(void);
Status getStatus(void);
void setStatus(Status newStatus);
Screen getScreen(void);
void setScreen(Screen newScreen);
void refreshScreen(void);

// Display Functions
void displayPage(char line1[], char line2[], char line3[], char line4[]);
void displayMenuPage(char line1[], char line2[], char line3[], bool leftPage, bool rightPage);
void displayTime(void);

// RTC Functions
void readTime(unsigned char pTime[7]);

void main(void) {
    // Setup variables, pins and peripherals
    initialize();
    
    // Main Loop
    while (1) {
        
        // Screen based actions
        switch (getScreen()) {
            case SC_STANDBY:
                /* [Any Key] - Go into the main menu
                 */
                if (key_was_pressed) {
                    // After any key press, go into the menu
                    setScreen(SC_MENU);
                    
                    key_was_pressed = false;
                }
                
                displayTime();
                
                break;
                
            case SC_MENU:
                /* [A] Load Tires
                 * [B] Logs
                 * [C] About
                 * [D] Debug
                 */
                if (key_was_pressed) {
                    switch (keys[key]) {
                        case 'A':
                            // Set mode to operating
                            break;
                            
                        case 'B':
                            setScreen(SC_LOGS_MENU);
                            break;
                            
                        case 'C':
                            setScreen(SC_ABOUT);
                            break;
                            
                        case 'D':
                            setScreen(SC_DEBUG);
                            break;
                    }
                }
                break;
                
            default:
                break;
        }
        
    }
    
    unsigned long tick = 0;
    while(1){
        
        if(tick % 1000 == 0){
            lcd_home();
            printf("%s", msg);
//            msg = (msg == msg1) ? msg2 : msg1; // Alternate the message
        }
        
        tick++;
        __delay_ms(1);
    }
}

//===================== FUNCTIONS =====================//
// State functions
void initialize(void) {
    // RD2 is the character LCD RS
    // RD3 is the character LCD enable (E)
    // RD4-RD7 are character LCD data lines
    TRISD = 0x00;
    LATD = 0x00;
    
    // Set all A/D ports to digital (pg. 222)
    ADCON1 = 0b00001111;
    
    // Enable RB1 (keypad data available) interrupt
    INT1E= 1;
    
    // Enable RB0 (emergency stop) interrupt
    INT2IE = 1;
    
    // Initialize LCD
    initLCD();
    
    // Initialize I2C Master with 100 kHz clock
    I2C_Master_Init(100000);
    
    // Enable interrupts
    ei();
    
    // Set/clear variables
    
}

Status getStatus(void) {
    return CURRENT_STATUS;
}

void setStatus(Status newStatus) {
     // Sets the new status and performs an initial function
    switch (newStatus) {
        case ST_STANDBY:
            setScreen(SC_STANDBY);
            break;
            
        default:
            // Return the function to not set the state if not a valid status
            return;
            break;
    }
    
    CURRENT_STATUS = newStatus;
}

Screen getScreen(void) {
    return CURRENT_SCREEN;
}

void setScreen(Screen newScreen) {
    // Sets the new screen and performs an initial function
    switch (newScreen) {
        case SC_STANDBY:
            displayPage("   Skybot Inc   ",
                        {},
                        {},
                        {});
            break;
            
        case SC_MENU:
            displayPage("[A] Load Tires",
                        "[B] Logs      ",
                        "[C] About     ",
                        "[D] Debug     ");
            break;
            
        case SC_LOGS_MENU:
            displayPage("[A] View Logs ",
                        "[B] Download  ",
                        "              ",
                        "[D] Return    ");
            break;
            
        case SC_DEBUG:
            displayPage("[A] Log Debug ",
                        "[B] Actuators ",
                        "              ",
                        "[D] Return    ");
            break;
            
        default:
            break;
    }
}

void refreshScreen(void) {
    setScreen(getScreen());
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
    displayPage(line1, line2, line3, {});
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

void displayTime(void) {
    unsigned char time[7];
    readTime(time);
    
    // Convert time to decimal from 1-12
    if (time[5] > 0x09) {
        time[5] = time[5] - 6;
    }
    
    lcd_set_ddram_addr(LCD_LINE_2);
    printf("%s ", months[time[5] - 1]);     // Month
    printf("%02X", time[4]);                // Date
    if (time[4] & 0x0F <= 3) {              // Date Suffix
        printf("%s", dateSuffix[time[4] & 0x0F]);
    } else {
        printf("th");
    }
    printf(", 20%02X", time[6]);            // Year
    
    lcd_set_ddram_addr(LCD_LINE_3);
    printf("   %02X:", time[2] > 0x12 ? ((((time[2] >> 4) - 1) << 4) | time[2]) : time[2]); // Hour
    printf("%02X:", time[1]);                       // Minute
    printf("%02X ", time[0]);                       // Second
    printf("%s  ", time[2] > 0x12 ? "PM", "AM");    // AM/PM
}

// Interrupt Functions
void interrupt interruptHandler(void){
// Handles interrupts received from RB0 (emergency stop) and RB1 (keypad)
    if(INT1IE && INT1IF){
        // Set a flag to handle interrupt and clear interrupt flag bit
        key_was_pressed = true;
        key = keys[(PORTB & 0xF0) >> 4];
        INT1IF = 0;

    } else if (INT2IE && INT2IF) {
        emergency_stop_pressed = true;
        INT2IF = 0;
    }
}