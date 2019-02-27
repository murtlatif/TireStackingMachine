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
} Screen;

// Number to String Arrays
static const char * months[] = {"Jan.  ",
                                "Feb.  ",
                                "March ",
                                "April ",
                                "May   ",
                                "June  ",
                                "July  ",
                                "Aug.  ",
                                "Sept. ",
                                "Oct.  ",
                                "Nov.  ",
                                "Dec.  "};

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
                if (key_was_pressed) {
                    // After any key press, go into the menu
                    
                    key_was_pressed = false;
                }
                lcd_set_ddram_addr()
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
            break;
            
        case SC_DEBUG:
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
    printf("%s", months[time[5] - 1]);
    
}

// RTC Functions
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

//    // Print received data on LCD
    lcd_home();
    printf("%02x/%02x/%02x", time[6],time[5],time[4]); // Print date in YY/MM/DD
    lcd_set_ddram_addr(LCD_LINE2_ADDR);
    printf("%02x:%02x:%02x", time[2],time[1],time[0]); // HH:MM:SS
    __delay_ms(1000);
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