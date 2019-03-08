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
    SC_OVERWRITE_LOG_VERIFICATION_1,
    SC_OVERWRITE_LOG_VERIFICATION_2,
    SC_OVERWRITE_LOG_VERIFICATION_3,
    SC_SAVE_COMPLETED
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
    unsigned short drivingTick = 0;
    unsigned short durationTick = 0;
    unsigned short stepperTick = 0;
    unsigned short tick = 0;
    unsigned char counter = 0;
    unsigned char time[7];

    // Operation Variables
    unsigned char loadedTires = 15;
    unsigned char step = 0;
    unsigned short stepsRemaining = 0;
    
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
                    counter = 0;
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
                    if (tick == 999) {
                        counter++;
                        if (counter == 10) {
                            setStatus(ST_STANDBY);
                        }
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

                    key_was_pressed = false;
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
                                refreshScreen();
                            }
                            
                            break;
                            
                        case '*':
                            if (loadedTires > 0) {
                                loadedTires--;
                                refreshScreen();
                            }
                            
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
                        case '1':
                            CURRENT_OPERATION.totalNumberOfPoles++;
                            CURRENT_OPERATION.distanceOfPole[CURRENT_OPERATION.totalNumberOfPoles - 1] = CURRENT_OPERATION.position;
                            CURRENT_OPERATION.tiresOnPoleAfterOperation[CURRENT_OPERATION.totalNumberOfPoles - 1] = getNumberOfTiresOnPole();
                            setStatus(ST_OPERATE_POLE_DETECTED);
                            break;
                            
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
                        durationTick = 0;
                        drivingTick = 0;
                        stepperTick = 0;
                        setStatus(ST_OPERATE_DRIVING);
                        
                        break;

                    case ST_OPERATE_DRIVING:
                        // Return to the start line if reached max distance or found max number of poles
                        if ((CURRENT_OPERATION.position >= 400) || CURRENT_OPERATION.totalNumberOfPoles >= 10) {
                            setStatus(ST_OPERATE_RETURN);
                        } else if (CURRENT_OPERATION.position - CURRENT_OPERATION.distanceOfPole[CURRENT_OPERATION.totalNumberOfPoles - 1] > SAME_POLE_REGION) {
//                            if (readSensor() < POLE_DETECTED_RANGE) {
//                                CURRENT_OPERATION.totalNumberOfPoles++;
//                                CURRENT_OPERATION.distanceOfPole[CURRENT_OPERATION.totalNumberOfPoles - 1] = CURRENT_OPERATION.position;
//                                CURRENT_OPERATION.tiresOnPoleAfterOperation[CURRENT_OPERATION.totalNumberOfPoles - 1] = getNumberOfTiresOnPole();
//                                setStatus(ST_OPERATE_POLE_DETECTED);
//                            }
                        }

                        if (drivingTick == 100) {
                            CURRENT_OPERATION.position++;
                            drivingTick = 0;
                        }
                        break;

                    case ST_OPERATE_POLE_DETECTED:
                    CURRENT_OPERATION.tiresOnPoleAfterOperation[CURRENT_OPERATION.totalNumberOfPoles - 1] = getNumberOfTiresOnPole();
                    // Deploy tire if tire required on pole
                        if (getNumberOfTiresRequiredForPole() > CURRENT_OPERATION.tiresOnPoleAfterOperation[CURRENT_OPERATION.totalNumberOfPoles - 1]) {
                            if (CURRENT_OPERATION.tiresRemaining == 0) {
                                setStatus(ST_OPERATE_RETURN);
                            } else {
                                stepperTick = 0;
                                stepsRemaining = STEPS_FOR_ONE_REVOLUTION;
                                setStatus(ST_OPERATE_DEPLOYING_TIRE);
                            }
                        } else {
                        // Continue driving if no more tires required
                            if (CURRENT_OPERATION.totalNumberOfPoles == 10) {
                                // Drive back to start if this pole was the 10th pole
                                drivingTick = 100 - drivingTick;
                                setStatus(ST_OPERATE_RETURN);
                            } else {
                                setStatus(ST_OPERATE_DRIVING);
                            }
                        }
                        break;

                    case ST_OPERATE_DEPLOYING_TIRE:
                        // Step stepper every 2 ms
                        if (stepperTick == 2000) {
                            stepStepper(CLOCKWISE, &step);
                            stepsRemaining--;
                            stepperTick = 0;

                            if (stepsRemaining == 0) {
                                CURRENT_OPERATION.totalSuppliedTires++;
                                CURRENT_OPERATION.tiresDeployedOnPole[CURRENT_OPERATION.totalNumberOfPoles - 1]++;
                                CURRENT_OPERATION.tiresRemaining--;
                                setStatus(ST_OPERATE_POLE_DETECTED);
                            }
                        }
                        break;

                    case ST_OPERATE_RETURN:
                        if (CURRENT_OPERATION.position == 0) {
                            setStatus(ST_COMPLETED_OP);
                        } else if (drivingTick == 100) {
                            CURRENT_OPERATION.position--;
                            drivingTick = 0;
                        }

                    default:
                        break;
                }

                durationTick++;
                drivingTick++;
                
                if (durationTick % 100 == 0) {
                    refreshScreen();
                }
                
                if (durationTick == 1000) {
                    durationTick = 0;
                    CURRENT_OPERATION.duration++;
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
                            page = 0;
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
                /* <[*] BCK[0] [#]>
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
                            setScreen(SC_TERMINATED);
                            break;

                        case '#':
                            if (page < (CURRENT_OPERATION.totalNumberOfPoles + 1)) {
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
            case SC_SAVE:
                /* [B] Save
                 * [C] Don't Save
                 * [D] Back
                 */
                if (key_was_pressed) {
                    switch (key) {
                        case 'B':
                            page = 0;
                            setScreen(SC_SELECT_SAVE_SLOT);
                            break;

                        case 'C':
                            setStatus(ST_READY);
                            break;

                        case 'D':
                            setScreen(SC_TERMINATED);
                            break;
                            
                        default:
                            break;
                    }

                    key_was_pressed = false;
                }
                break;

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
                            setScreen(SC_SAVE);
                            break;

                        case '#':
                            if (page < 5) {
                                page++;
                                refreshScreen();
                            }
                            break;

                        case 'A':
                            if (getLogSlot(page * 3) == USED) {
                                setScreen(SC_OVERWRITE_LOG_VERIFICATION_1);
                            } else {
                                storeCondensedOperation(CURRENT_OPERATION);
                                saveCondensedOperationIntoLogs(page * 3, CURRENT_OPERATION);
                                CURRENT_OPERATION.savedSlot = (page * 3) + 1;
                                setScreen(SC_SAVE_COMPLETED);
                            }
                            break;

                        case 'B':
                            if (getLogSlot((page * 3) + 1) == USED) {
                                setScreen(SC_OVERWRITE_LOG_VERIFICATION_2);
                            } else {
                                storeCondensedOperation(CURRENT_OPERATION);
                                saveCondensedOperationIntoLogs((page * 3) + 1, CURRENT_OPERATION);
                                CURRENT_OPERATION.savedSlot = (page * 3) + 2;
                                setScreen(SC_SAVE_COMPLETED);
                            }
                            break;

                        case 'C':
                            if (getLogSlot((page * 3) + 2) == USED) {
                                setScreen(SC_OVERWRITE_LOG_VERIFICATION_3);
                            } else {
                                storeCondensedOperation(CURRENT_OPERATION);
                                saveCondensedOperationIntoLogs((page * 3) + 2, CURRENT_OPERATION);
                                CURRENT_OPERATION.savedSlot = (page * 3) + 3;
                                setScreen(SC_SAVE_COMPLETED);
                            }
                            break;

                        default:
                            break;
                    }

                    key_was_pressed = false;
                }
               
                break;

            //===========================================
            case SC_OVERWRITE_LOG_VERIFICATION_1:
                /* [C] No
                 * [D] Yes
                 */
                if (key_was_pressed) {
                    switch (key) {
                        case 'C':
                            setScreen(SC_SELECT_SAVE_SLOT);
                            break;

                        case 'D':
                            storeCondensedOperation(CURRENT_OPERATION);
                            saveCondensedOperationIntoLogs(page * 3, CURRENT_OPERATION);
                            CURRENT_OPERATION.savedSlot = (page * 3) + 1;
                            setScreen(SC_SAVE_COMPLETED);
                            break;

                        default:
                            break;
                    }
 
                    key_was_pressed = false;
                }
                break;

            //===========================================
            case SC_OVERWRITE_LOG_VERIFICATION_2:
                /* [C] No
                 * [D] Yes
                 */
                if (key_was_pressed) {
                    switch (key) {
                        case 'C':
                            setScreen(SC_SELECT_SAVE_SLOT);
                            break;

                        case 'D':
                            storeCondensedOperation(CURRENT_OPERATION);
                            saveCondensedOperationIntoLogs((page * 3) + 1, CURRENT_OPERATION);
                            CURRENT_OPERATION.savedSlot = (page * 3) + 2;
                            setScreen(SC_SAVE_COMPLETED);
                            break;

                        default:
                            break;
                    }
 
                    key_was_pressed = false;
                }
                break;

            //===========================================
            case SC_OVERWRITE_LOG_VERIFICATION_3:
                /* [C] No
                 * [D] Yes
                 */
                if (key_was_pressed) {
                    switch (key) {
                        case 'C':
                            setScreen(SC_SELECT_SAVE_SLOT);
                            break;

                        case 'D':
                            storeCondensedOperation(CURRENT_OPERATION);
                            saveCondensedOperationIntoLogs((page * 3) + 2, CURRENT_OPERATION);
                            CURRENT_OPERATION.savedSlot = (page * 3) + 3;
                            setScreen(SC_SAVE_COMPLETED);
                            break;

                        default:
                            break;
                    }
 
                    key_was_pressed = false;
                }
                break;

            //===========================================
            case SC_SAVE_COMPLETED:
                /* [D] OK
                 */

                if (key_was_pressed) {
                    switch (key) {
                        case 'D':
                            setStatus(ST_READY);
                            break;

                        default:
                            break;
                    }
 
                    key_was_pressed = false;
                } 
                break;
                break;
            
            //===========================================
            default:
                break;
        }

        // 1ms delay for every iteration
        __delay_ms(1);
        tick++;
        durationTick++;
        
        if (durationTick == 1000) {
            durationTick = 0;
        }
        if (tick == 1000) {
            // This occurs every second
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
    TRISCbits.TRISC1 = 0;
    LATCbits.LATC0 = 0;
    LATCbits.LATC1 = 0;
    
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
    
    // Initialize Status
    setStatus(ST_STANDBY);
    
    // Write time ( DO NOT UNCOMMENT )
//    rtcSetTime(valueToWriteToRTC);
}

Status getStatus(void) {
    return CURRENT_STATUS;
}

void setStatus(Status newStatus) {
    // Sets the new status and performs an initial function

    // Turn off motors if not driving
    if (newStatus != ST_OPERATE_DRIVING || newStatus != ST_OPERATE_RETURN) {
        driveDCMotor(MOTOR1, OFF);
    }

    switch (newStatus) {
        case ST_STANDBY:
        // Places the robot into standby mode (where time displays)
            setScreen(SC_STANDBY);
            break;
            
        case ST_READY:
        // Status to navigate robot menus
            setScreen(SC_MENU);
            break;

        case ST_OPERATE_START:
        // Status used to begin an operation
            CURRENT_OPERATION = EmptyOperation;

            // Initialize operation stats
            unsigned char condensedTime[5];
            condensedReadTime(condensedTime);
            for (char i = 0; i < 5; i++) {
                CURRENT_OPERATION.startTime[i] = condensedTime[i];
            }
            CURRENT_OPERATION.duration = 0;
            CURRENT_OPERATION.position = 0;
            CURRENT_OPERATION.totalNumberOfPoles = 0;
            CURRENT_OPERATION.totalSuppliedTires = 0;

            setScreen(SC_OPERATING);
            break;

        case ST_OPERATE_DRIVING:
        // Status used to drive the robot by powering the motors
            driveDCMotor(MOTOR1, CLOCKWISE);
            break;

        case ST_OPERATE_POLE_DETECTED:
        // Robot detects pole 
            CURRENT_OPERATION.totalNumberOfPoles++;
            CURRENT_OPERATION.distanceOfPole[CURRENT_OPERATION.totalNumberOfPoles - 1] = CURRENT_OPERATION.position;
            CURRENT_OPERATION.tiresDeployedOnPole[CURRENT_OPERATION.totalNumberOfPoles - 1] = 0;

            break;

        case ST_OPERATE_DEPLOYING_TIRE:
        // Robot deploys pole

            break;

        case ST_OPERATE_RETURN:
        // Robot returns back to the start
            driveDCMotor(MOTOR1, COUNTER_CLOCKWISE);
            break;

        case ST_COMPLETED_OP:
        // Robot completes operation
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
        // Standby screen. Time / date displays here
            displayPage("   Skybot Inc   ",
                        "                ",
                        "                ",
                        "                ");
            break;
            
        case SC_MENU:
        // Main navigation menu
            displayPage("[A] Load Tires  ",
                        "[B] Logs        ",
                        "[C] About       ",
                        "[D] Debug       ");
            break;

        case SC_ABOUT:
        // Displays some information about the project
            displayPage("Autonomous tire ",
                        "stacking robot  ",
                        "by Skybot Inc.  ",
                        "[D] Back        ");
            break;
            
        case SC_LOGS_MENU:
        // Navigation menu for logs
            displayPage("[A] View Logs   ",
                        "[B] Download    ",
                        "                ",
                        "[D] Back        ");
            break;
            
        case SC_DEBUG:
        // Debug menu
            displayPage("[A] Motor Debug ",
                        "[B] Log Debug   ",
                        "                ",
                        "[D] Back        ");
            break;

        case SC_LOAD_TIRES:
        // Select how many tires are loaded into the robot
            displayPage("                ",
                        "[*]Less  More[#]",
                        "[C] Start op.   ",
                        "[D] Back        ");
            break;

        case SC_OPERATING:
        // Screen that displays while the robot is operating
            displayPage("                ",
                        "                ",
                        "                ",
                        "[D] EMRGNCY STOP");

            lcd_set_ddram_addr(LCD_LINE1_ADDR);
            switch (getStatus()) {
                case ST_OPERATE_START:
                    printf(" Starting op... ");
                    break;

                case ST_OPERATE_DRIVING:
                    printf("    DRIVING     ");
                    lcd_set_ddram_addr(LCD_LINE2_ADDR);
                    printf("Position:    %03d", CURRENT_OPERATION.position);
                    lcd_set_ddram_addr(LCD_LINE3_ADDR);
                    printf("Poles found:  %02d", CURRENT_OPERATION.totalNumberOfPoles);
                    break;

                case ST_OPERATE_POLE_DETECTED:
                    printf(" POLE DETECTED  ");
                    lcd_set_ddram_addr(LCD_LINE2_ADDR);
                    printf("Tires Found:  %02d", CURRENT_OPERATION.tiresOnPoleAfterOperation);
                    lcd_set_ddram_addr(LCD_LINE3_ADDR);
                    printf("Tires Stacked: %01d", CURRENT_OPERATION.tiresDeployedOnPole[CURRENT_OPERATION.totalNumberOfPoles - 1]);
                    break;

                case ST_OPERATE_DEPLOYING_TIRE:
                    printf(" DEPLOYING TIRE ");
                    lcd_set_ddram_addr(LCD_LINE2_ADDR);
                    printf("Tire Ammo:    %02d", CURRENT_OPERATION.tiresRemaining);
                    lcd_set_ddram_addr(LCD_LINE3_ADDR);
                    printf("Tires Stacked: %01d", CURRENT_OPERATION.tiresDeployedOnPole[CURRENT_OPERATION.totalNumberOfPoles]);
                    break;
                    
                case ST_OPERATE_RETURN:
                    printf("    RETURNING   ");
                    lcd_set_ddram_addr(LCD_LINE2_ADDR);
                    printf("Position:    %03d", CURRENT_OPERATION.position);
                    break;
                    
                default:
                    break;
            }
            break;

        case SC_TERMINATED:
        // Termination screen when the operation is complete
            displayPage("Operation Done. ",
                        "[B] View Results",
                        "[C] Finish Op.  ",
                        "                ");
            break;

        case SC_VIEW_RESULTS:
        // Screen to view the results of the operation (or viewing them through the logs)
            displayMenuPage("                ",
                            "                ",
                            "                ",
                            page > 0, page < (CURRENT_OPERATION.totalNumberOfPoles + 1));

            if (page == 0) {
                lcd_set_ddram_addr(LCD_LINE1_ADDR);
                printf("Day: %s %02X/19", months[CURRENT_OPERATION.startTime[4]], CURRENT_OPERATION.startTime[3]);
                lcd_set_ddram_addr(LCD_LINE2_ADDR);
                printf("Time:   %02X:%02X:%02X", CURRENT_OPERATION.startTime[2], CURRENT_OPERATION.startTime[1], CURRENT_OPERATION.startTime[0]);
                lcd_set_ddram_addr(LCD_LINE3_ADDR);
                printf("Duration:  %02d:%02d", CURRENT_OPERATION.duration / 60, CURRENT_OPERATION.duration % 60);

            } else if (page == 1) {
                lcd_set_ddram_addr(LCD_LINE1_ADDR);
                printf("Poles Found:  %02d", CURRENT_OPERATION.totalNumberOfPoles);
                lcd_set_ddram_addr(LCD_LINE2_ADDR);
                printf("Total Tires:  %02d", CURRENT_OPERATION.totalSuppliedTires);

            } else if (page > 1 && page < CURRENT_OPERATION.totalNumberOfPoles + 2) {
                lcd_set_ddram_addr(LCD_LINE1_ADDR);
                printf("Pole #dd  %03d cm", CURRENT_OPERATION.distanceOfPole[page - 2]);
                lcd_set_ddram_addr(LCD_LINE2_ADDR);
                printf("Tires Stacked: %01d", CURRENT_OPERATION.tiresDeployedOnPole[page - 2]);
                lcd_set_ddram_addr(LCD_LINE3_ADDR);
                printf("Tires on Pole: %01d", CURRENT_OPERATION.tiresOnPoleAfterOperation[page - 2]);
            }
            break;

        case SC_SAVE:
        // Ask user if they would like to save operation or continue without saving
            displayPage("Save operation? ",
                        "[B] Save            ",
                        "[C] Don't Save      ",
                        "[D] Back            ");
            break;

        case SC_SELECT_SAVE_SLOT:
        // Display menu page for user to select slot to save the operation
            displayMenuPage("                ",
                            "                ",
                            "                ",
                            page > 0, page < 5);

            lcd_set_ddram_addr(LCD_LINE1_ADDR);
            printf("[A] Slot %02d %s", (page * 3) + 1, getLogSlot(page * 3) == USED ? "USED" : "    ");
            if (page < 5) {
                lcd_set_ddram_addr(LCD_LINE2_ADDR);
                printf("[B] Slot %02d %s", (page * 3) + 2, getLogSlot((page * 3) + 1) == USED ? "USED" : "    ");
                lcd_set_ddram_addr(LCD_LINE3_ADDR);
                printf("[C] Slot %02d %s", (page * 3) + 3, getLogSlot((page * 3) + 2) == USED ? "USED" : "    ");
            }
            break;

        case SC_OVERWRITE_LOG_VERIFICATION_1:
        // Prompt that appear if the user selects a slot on the first row that is already used
            displayPage("                ",
                        "Overwrite?      ",
                        "[C] No          ",
                        "[D] Yes         ");

            lcd_set_ddram_addr(LCD_LINE1_ADDR);
            printf("Slot %02d in use. ", (page * 3) + 1);
            break;

        case SC_OVERWRITE_LOG_VERIFICATION_2:
        // Prompt that appear if the user selects a slot on the second row that is already used
            displayPage("                ",
                        "Overwrite?      ",
                        "[C] No          ",
                        "[D] Yes         ");

            lcd_set_ddram_addr(LCD_LINE1_ADDR);
            printf("Slot %02d in use. ", (page * 3) + 2);
            break;

        case SC_OVERWRITE_LOG_VERIFICATION_3:
        // Prompt that appear if the user selects a slot on the third row that is already used
            displayPage("                ",
                        "Overwrite?      ",
                        "[C] No          ",
                        "[D] Yes         ");

            lcd_set_ddram_addr(LCD_LINE1_ADDR);
            printf("Slot %02d in use. ", (page * 3) + 3);
            break;

        case SC_SAVE_COMPLETED:
        // Confirmation of save prompt
            displayPage("Saved operation ",
                        "successfully in ",
                        "                ",
                        "[D] OK          ");

            lcd_set_ddram_addr(LCD_LINE3_ADDR);
            printf("    slot %02d     ", CURRENT_OPERATION.savedSlot);
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