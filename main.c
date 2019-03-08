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
#include "configureBits.h"

#include "lcd.h"
#include "I2C.h"
#include "logs.h"
#include "operate.h"

#include <stdio.h>
#include <stdbool.h>

//** MACROS **//
#define MAX_TIRE_CAPACITY 15

//** CONSTANTS **//

// Constant values
static const char keys[] = "123A456B789C*0#D";
static const Operation EmptyOperation;

// Enumerations
typedef enum {
    ST_STANDBY = 0,
    ST_READY,
    ST_OPERATE_START,
    ST_OPERATE_IDLE,
    ST_OPERATE_DRIVING,
    ST_OPERATE_POLE_DETECTED,
    ST_OPERATE_DEPLOYING_TIRE,
    ST_OPERATE_RETURN,
    ST_COMPLETED_OP
} Status;

typedef enum {
    SC_STANDBY = 0,
    SC_MENU,
    SC_DEBUG,
    SC_ABOUT,
    SC_LOGS_MENU,
    SC_LOGS_VIEW,
    SC_LOAD_TIRES,
    SC_OPERATING,
    SC_TERMINATED,
    SC_VIEW_RESULTS,
    SC_SAVE,
    SC_SELECT_SAVE_SLOT,
} Screen;

//** VARIABLES **//

// State variables
Status CURRENT_STATUS;
Screen CURRENT_SCREEN;
Operation CURRENT_OPERATION;
unsigned char page;

// Interrupt Variables
volatile bool key_was_pressed = false;
volatile bool emergency_stop_pressed = false;
volatile unsigned char key;

// RTC Variables
char valueToWriteToRTC[7] = {
    0x30, // Seconds 
    0x47, // Minutes
    0x19, // Hours (set to 24 hour mode)
    0x04, // Weekday
    0x07, // Day of month
    0x03, // Month
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

void main(void) {
    // Setup variables, pins and peripherals
    initialize();
    
    // Initialize other variables

    // Time Variables
    unsigned short tick = 0;
    unsigned char counter = 0;
    unsigned char time[7];

    // Operation Variables
    unsigned char loadedTires = 15;
    
    // Main Loop
    while (1) {
        
        // Screen based actions
        switch (getScreen()) {
            //==============STATUS: STANDBY==============
            case SC_STANDBY:
                /* [Any Key] - Go into the main menu
                 */
                if (key_was_pressed) {
                    // After any key press, go into the menu
                    setStatus(ST_READY);
                    
                    key_was_pressed = false;
                } else {
                    readTime(time);
                    displayTime(time);
                }

                break;

            //===============STATUS: READY===============
            case SC_MENU:
                /* [A] Load Tires
                 * [B] Logs
                 * [C] About
                 * [D] Debug
                 */
                if (key_was_pressed) {
                    switch (key) {
                        case 'A':
                            setScreen(SC_LOAD_TIRES);
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

                        default:
                            break;
                    }

                    key_was_pressed = false;
                } else {
                    if (counter == 9) {
                        setStatus(ST_STANDBY);
                    }
                }

                break;
            //===========================================
            case SC_ABOUT:
                /* [D] Back
                */
               if (key_was_pressed) {
                   switch (key) {
                        case 'D':
                            setScreen(SC_MENU);
                            break;

                        default:
                            break;
                   }

                   key_was_pressed = false;
               }
               break;
            
            //===========================================
            case SC_DEBUG:
                /* [A] Actuator Debug
                 * [B] Log Debug
                 * 
                 * [D] Back
                 */
                
                if (key_was_pressed) {
                   switch (key) {
                        case 'D':
                            setScreen(SC_MENU);
                            break;

                        default:
                            break;
                   }

                   key_was_pressed = false;
               }
               break;

            //===========================================
            case SC_LOGS_MENU:
                /* [D] Back
                 */
                if (key_was_pressed) {
                    switch (key) {
                        case 'D':
                            setScreen(SC_MENU);
                            break;

                        default:
                            break;
                    }
                }
                break;

            //===========================================
            case SC_LOAD_TIRES:
                /* [*]Less  More[#]
                 * [C] Start Op.
                 * [D] Back
                 */
                lcd_set_ddram_addr(LCD_LINE1_ADDR);
                printf("Loading %02d tires", loadedTires);

                if (key_was_pressed) {
                    switch (key) {
                        case '#':
                            if (loadedTires < MAX_TIRE_CAPACITY) {
                                loadedTires++;
                            }
                            refreshScreen();
                            break;
                            
                        case '*':
                            if (loadedTires > 0) {
                                loadedTires--;
                            }
                            refreshScreen();
                            break;
                            
                        case 'C':
                            setStatus(ST_OPERATE_START);
                            CURRENT_OPERATION.tiresRemaining = loadedTires;
                            loadedTires = MAX_TIRE_CAPACITY;
                            break;

                        case 'D':
                            setStatus(ST_READY);
                            loadedTires = MAX_TIRE_CAPACITY;
                            break;

                        default:
                            break;
                    }

                    key_was_pressed = false;
                }

                break;

            //=============STATUS: OPERATING=============
            case SC_OPERATING:
                if (key_was_pressed) {
                    switch (key) {
                        case 'D':
                            setStatus(ST_COMPLETED_OP);
                            break;

                        default:
                            break;
                    }
 
                    key_was_pressed = false;
                }
                
                switch (getStatus()) {
                    case ST_OPERATE_START:
                        setStatus(ST_OPERATE_DRIVING);
                        tick = 0;
                        break;

                    case ST_OPERATE_DRIVING:
                        // Return to the start line if reached max distance or found max number of poles
                        if ((CURRENT_OPERATION.position >= 400) || CURRENT_OPERATION.totalNumberOfPoles >= 10) {
                            setStatus(ST_OPERATE_RETURN);
                        }

                        if (CURRENT_OPERATION.position - CURRENT_OPERATION.distanceOfPole[CURRENT_OPERATION.totalNumberOfPoles - 1] > SAME_POLE_REGION) {
                            if (readSensor() < POLE_DETECTED_RANGE) {
                                setStatus(ST_OPERATE_POLE_DETECTED);
                            }
                        }
                        if (tick % 100 == 0) {
                            CURRENT_OPERATION.position++;
                        }
                        break;

                    case ST_OPERATE_RETURN:
                        if (CURRENT_OPERATION.position )

                    default:
                        break;
                }

                break;
                
            //=============STATUS: COMPLETED=============
            case SC_TERMINATED:
                /* [B] View Results
                 * [C] Finish Op.
                 */
                if (key_was_pressed) {
                    switch (key) {
                        case 'B':
                            setScreen(SC_VIEW_RESULTS);
                            break;

                        case 'C':
                            setScreen(SC_SAVE);
                            break;

                        default:
                            break;
                    }
 
                    key_was_pressed = false;
                } 
                break;

            //===========================================
            case SC_VIEW_RESULTS:
                /* [D] Back
                 */
                if (key_was_pressed) {
                    switch (key) {
                        case 'D':
                            setScreen(SC_TERMINATED);
                            break;

                        default:
                            break;
                    }

                    key_was_pressed = false;
                }
            //===========================================
            case SC_SAVE:
                /* [C] Save
                 * [D] Don't Save
                 */
                if (key_was_pressed) {
                    switch (key) {
                        case 'C':
                            page = 0;
                            setScreen(SC_SELECT_SAVE_SLOT);
                            break;

                        case 'D':
                            setStatus(ST_READY);
                            break;

                        default:
                            break;
                    }

                    key_was_pressed = false;
                }

            //===========================================
            case SC_SELECT_SAVE_SLOT:
                /* <[*] BCK[0] [#]>
                 * [A] Slot #<1, 4, 7, 10, 13, 16>
                 * [B] Slot #<2, 5, 8, 11, 14>
                 * [C] Slot #<3, 6, 9, 12, 15>
                */
                if (key_was_pressed) {
                    switch (key) {
                        case '*':
                            if (page > 0) {
                                page--;
                                refreshScreen();
                            }
                            break;

                        case '0':
                            setStatus(ST_READY);
                            break;

                        case '#':
                            if (page < 6) {
                                page++;
                                refreshScreen();
                            }
                            break;

                        default:
                            break;
                    }

                    key_was_pressed = false;
                }
               
                break;

            //===========================================
            default:
                break;
        }

        // 1ms delay for every iteration
        __delay_ms(1);
        tick++;
        
        if (tick == 1000) {
            // This occurs every second
            counter++;
            
            if (counter == 10) {
                counter = 0;
            }
            tick = 0;
        }
    }
}

//===================== FUNCTIONS =====================//
// State functions
void initialize(void) {
    // RD2 is the character LCD RS
    // RD3 is the character LCD enable (E)
    // RD4-RD7 are character LCD data lines
    TRISCbits.TRISC0 = 0;
    LATCbits.LATC0 = 0;
    
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
    
    // Write time ( DO NOT UNCOMMENT )
//    rtcSetTime(valueToWriteToRTC);
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
            
        case ST_READY:
            setScreen(SC_MENU);
            break;

        case ST_OPERATE_START:
            CURRENT_OPERATION = EmptyOperation;
            setScreen(SC_OPERATING);
            break;

        case ST_OPERATE_DRIVING:
            driveMotor();
            break;

        case ST_OPERATE_POLE_DETECTED:
            CURRENT_OPERATION.totalNumberOfPoles++;
            CURRENT_OPERATION.distanceOfPole[CURRENT_OPERATION.totalNumberOfPoles - 1] = CURRENT_OPERATION.position;
            CURRENT_OPERATION.tiresDeployedOnPole[CURRENT_OPERATION.totalNumberOfPoles - 1] = 0;

            break;

        case ST_OPERATE_DEPLOYING_TIRE:
            
            break;

        case ST_COMPLETED_OP:
            setScreen(SC_TERMINATED);
            break;


        default:
            // Return the function to not set the state if not a valid status
            return;
            break;
    }
    
    CURRENT_STATUS = newStatus;
}

// Screen functions
Screen getScreen(void) {
    return CURRENT_SCREEN;
}

void setScreen(Screen newScreen) {
    // Sets the new screen and performs an initial function
    switch (newScreen) {
        case SC_STANDBY:
            displayPage("   Skybot Inc   ",
                        "                ",
                        "                ",
                        "                ");
            break;
            
        case SC_MENU:
            displayPage("[A] Load Tires  ",
                        "[B] Logs        ",
                        "[C] About       ",
                        "[D] Debug       ");
            break;

        case SC_ABOUT:
            displayPage("Autonomous tire ",
                        "stacking robot  ",
                        "by Skybot Inc.  ",
                        "[A] Back        ");
            break;
            
        case SC_LOGS_MENU:
            displayPage("[A] View Logs   ",
                        "[B] Download    ",
                        "                ",
                        "[D] Back        ");
            break;
            
        case SC_DEBUG:
            displayPage("[A] Motor Debug ",
                        "[B] Log Debug   ",
                        "                ",
                        "[D] Back        ");
            break;

        case SC_LOAD_TIRES:
            displayPage("                ",
                        "[*]Less  More[#]",
                        "[C] Start op.   ",
                        "[D] Back        ");
            break;

        case SC_OPERATING:
            displayPage("Operating...    ",
                        "                ",
                        "                ",
                        "[D] EMRGNCY STOP");
            break;

        case SC_TERMINATED:
            displayPage("Operation Done. ",
                        "[B] View Results",
                        "[C] Finish Op.  ",
                        "                ");
            break;

        case SC_SAVE:
            displayPage("Save operation? ",
                        "                ",
                        "[C] Save        ",
                        "[D] Don't Save  ");
            break;

        case SC_SELECT_SAVE_SLOT:
            displayMenuPage("                ",
                            "                ",
                            "                ",
                            page > 0, page < 6);

            lcd_set_ddram_addr(LCD_LINE1_ADDR);
            printf("[A] Slot %02d %s", (page * 3) + 1, getLogSlot(page * 3) == USED ? "USED" : "    ");
            if (page < 6) {
                lcd_set_ddram_addr(LCD_LINE2_ADDR);
                printf("[A] Slot %02d %s", (page * 3) + 2, getLogSlot((page * 3) + 1) == USED ? "USED" : "    ");
                lcd_set_ddram_addr(LCD_LINE3_ADDR);
                printf("[A] Slot %02d %s", (page * 3) + 3, getLogSlot((page * 3) + 2) == USED ? "USED" : "    ");
            }
            break;

        default:
            return;
            break;

    }
    
    CURRENT_SCREEN = newScreen;
}

void refreshScreen(void) {
    setScreen(getScreen());
}

// Interrupt Functions
void __interrupt() interruptHandler(void){
// Handles interrupts received from RB0 (emergency stop) and RB1 (keypad)
    if (INT1IE && INT1IF) {
        // Set a flag to handle interrupt and clear interrupt flag bit
        key_was_pressed = true;
        key = keys[(PORTB & 0xF0) >> 4];
        INT1IF = 0;

    } else if (INT2IE && INT2IF) {
        emergency_stop_pressed = true;
        INT2IF = 0;
    }
}