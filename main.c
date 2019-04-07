// MAIN FILE - Murtaza Latif

//** INCLUDES **//
#include <xc.h>
#include "configureBits.h"

#include "lcd.h"
#include "I2C.h"
#include "logs.h"
#include "operate.h"
#include "uart.h"

#include <stdio.h>
#include <stdbool.h>

//** MACROS **//
#define MAX_TIRE_CAPACITY 15
#define RETURN_TO_STANDBY_TIME_SECONDS 15

//** CONSTANTS **//

// Constant values
static const char keys[] = "123A456B789C*0#D";
static const Operation EmptyOperation = {
    .savedIntoLogs = false,
    .duration = 0,
    .totalSuppliedTires = 0,
    .totalNumberOfPoles = 0,
    .tiresDeployedOnPole = {},
    .tiresOnPoleAfterOperation = {},
    .distanceOfPole = {},
    .tiresRemaining = 0,
    .position = 0
};

// Enumerations
typedef enum {
    ST_STANDBY = 0,
    ST_READY,
    ST_ERROR,
    ST_OPERATE_START,
    ST_OPERATE_IDLE,
    ST_OPERATE_DRIVING,
    ST_OPERATE_POLE_DETECTED,
    ST_OPERATE_DEPLOYING_TIRE,
    ST_OPERATE_RETURN,
    ST_COMPLETED_OP
} Status;

typedef enum {
    // Standby
    SC_STANDBY = 0,

    // Main Menu
    SC_MENU,
    SC_ABOUT,

    // Debug
    SC_DEBUG,
    SC_DEBUG_LOG,
    SC_DEBUG_MOTOR,
    SC_DEBUG_STEPPER,
    SC_DEBUG_SENSOR,

    // Logs
    SC_LOGS_MENU,
    SC_LOGS_VIEW,
    SC_LOAD_TIRES,

    // Operating
    SC_OPERATING,

    // Termination
    SC_TERMINATED,
    SC_VIEW_RESULTS,
    SC_SAVE,
    SC_SELECT_SAVE_SLOT,
    SC_OVERWRITE_LOG_VERIFICATION_1,
    SC_OVERWRITE_LOG_VERIFICATION_2,
    SC_OVERWRITE_LOG_VERIFICATION_3,
    SC_SAVE_COMPLETED,

    // Errors
    SC_LOG_VIEW_ERROR,
    SC_INVALID_STATE_ERROR,
    SC_INVALID_SCREEN_ERROR,
    SC_UNHANDLED_ARDUINO_MESSAGE_ERROR,
    SC_SAVE_OPERATION_ERROR,

} Screen;

typedef enum {
    // PIC to Arduino Messages
    MSG_P2A_STOP = 0,
    MSG_P2A_START,
    MSG_P2A_DEPLOYMENT_COMPLETE,
    MSG_P2A_DEBUG_DRIVE_FORWARD,
    MSG_P2A_DEBUG_DRIVE_BACKWARD,
    MSG_P2A_DEBUG_STOP,
    MSG_P2A_DEBUG_SENSOR,
    MSG_P2A_REQUEST_POSITION,
    MSG_P2A_REQUEST_TIRES_FOUND,
    MSG_P2A_REQUEST_TIRES_REMAINING,

    // Arduino to PIC Messages
    MSG_A2P_DRIVING = 100,
    MSG_A2P_DEPLOY_STEPPER,
    MSG_A2P_COMPLETE_OP,
    MSG_A2P_RETURNING,

} MSG_CODE;
//** VARIABLES **//

// State variables
Status CURRENT_STATUS;          // Keep track of the current robot status
Screen CURRENT_SCREEN;          // Keep track of the current LCD screen
Operation CURRENT_OPERATION;    // To record the operation in progress
unsigned char page;             // Page number for any page menu LCD

// Interrupt Variables
volatile bool key_was_pressed = false;              // Keeps track of whether keypad input was pressed
volatile bool emergency_stop_pressed = false;       // Keeps track of whether emergency stop was pressed
volatile bool receivedMessageFromArduino = false;   // Keeps track of whether a message was received from arduino
volatile unsigned char messageFromArduino;          // Contains the message received from arduino
volatile unsigned char key;                         // Contains the key pressed from the keypad

// RTC Variables
char valueToWriteToRTC[7] = {               // The time to initialize to if writing to RTC
    0x00, // Seconds 
    0x10, // Minutes
    0x17, // Hours (set to 24 hour mode)
    0x01, // Weekday
    0x24, // Day of month
    0x03, // Month
    0x19  // Year
};

unsigned short returnToStandbyTick = 0;     // Timer variable (milliseconds) for returning to standby mode
unsigned char returnToStandbyCounter = 0;   // Timer variable (seconds) for returning to standby mode


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
    unsigned char pwmTick = 0;
    unsigned short durationTick = 0;
    unsigned char stepperTick = 0;
    unsigned short tick = 0;
    unsigned char counter = 0;
    unsigned char time[7];

    // Operation Variables
    unsigned char loadedTires = 15;
    unsigned char step = 0;
    unsigned short stepsRemaining = 0;
    bool pole_found = false;
    unsigned char spinning = 0;
    unsigned char slaveMotorSpeed = 0;
    unsigned char masterMotorSpeed = 0;
    unsigned char onDuty = 0;
    unsigned char pwmDrive = 0x0;
    unsigned char sensorReading = 0;
    unsigned short currentPos = 0;

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
                    // Read and display the time while on standby screen
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
                /* [A] Motor Debug
                 * [B] Log Debug
                 * [C] Sensor Debug
                 * [D] Back
                 */
                
                if (key_was_pressed) {
                   switch (key) {
                        case 'A':
                            setScreen(SC_DEBUG_MOTOR);
                            break;
                       
                        case 'B':
                            setScreen(SC_DEBUG_LOG);
                            break;

                        case 'C':
                            setScreen(SC_DEBUG_SENSOR);
                            break;
                           
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
            case SC_DEBUG_LOG:
                /* [C] Reset Slots
                 * [D] Back
                 */
                
                if (key_was_pressed) {
                   switch (key) {
                        case 'C':
                            writeAllLogSlots(0x0000);
                            refreshScreen();
                            break;

                        case 'D':
                            setScreen(SC_DEBUG);
                            break;

                        default:
                            break;
                   }

                   key_was_pressed = false;
               }
               break;

            //===========================================
            case SC_DEBUG_MOTOR:
                /* [A] Forward
                 * [B] Backward
                 * [C] Off
                 * [D] Back
                 */

                if (key_was_pressed) {
                    switch (key) {
                        case 'A':
                            // Tell Arduino to drive motors forward
                            UART_Write(MSG_DEBUG_DRIVE_FORWARD);
                            break;

                        case 'B':
                            // Tell Arduino to drive motors backward
                            UART_Write(MSG_DEBUG_DRIVE_BACKWARD);
                            break;

                        case 'C':
                            // Tell Arduino to stop driving motors
                            UART_Write(MSG_DEBUG_STOP);
                            break;

                        case 'D':
                            setScreen(SC_DEBUG);
                            break;

                        case '1':
                            setScreen(SC_DEBUG_STEPPER);
                            break;
                            
                        default:
                            break;
                    }

                    key_was_pressed = false;
                }

                break;

            //===========================================
            case SC_DEBUG_STEPPER:
                /* [1] 1 Forward
                 * [3] 1 Back
                 * [4] Tire Forward
                 * [6] Tire Back
                 * [0] Stop
                 * [D] Return
                 */
                if (key_was_pressed) {
                    switch (key) {                        
                        case 'D':
                            setScreen(SC_DEBUG);
                            break;
                            
                        case '1':
                            // Drive stepper 1 revolution forward
                            driveStepper(1, FORWARD);
                            break;
                            
                        case '3':
                            // Drive stepper 1 revolution backward
                            driveStepper(1, BACKWARD);
                            break;
                            
                        case '0':
                            STEPPER_EN = 0;
                            STEPPER_DIR = 0;
                            STEPPER_PULSE = 0;
                            break;
                            
                        case '4':
                            // Make enough revolutions to deploy a tire (forward)
                            driveStepper(REVOLUTIONS_TO_DROP_ONE_TIRE, FORWARD);
                            break;
                            
                        case '6':
                            // Make enough revolutions for one tire (backward)
                            driveStepper(REVOLUTIONS_TO_DROP_ONE_TIRE, BACKWARD);
                            break;
                            
                        default:
                            break;
                    }

                    key_was_pressed = false;
                }
                break;
                
            //===========================================
            case SC_DEBUG_SENSOR:
                /* [C] Read Sensor
                 * [D] Back
                 */
                if (key_was_pressed) {
                    switch (key) {

                        case 'C':
                            // Print first two lines of screen
                            lcd_home();
                            printf("Reading distance");
                            lcd_set_ddram_addr(LCD_LINE2_ADDR);
                            printf("                ");

                            // Retrieve sensor data from Arduino
                            sensorReading = UART_Request_Byte(MSG_DEBUG_SENSOR);

                            // Write sensor data to LCD
                            lcd_home();
                            if (sensorReading) {
                                printf("Distance:  %03dmm", sensorReading);
                            } else {
                                printf("No reading found");
                            }
                            break;

                        case 'D':
                            setScreen(SC_DEBUG);
                            break;

                        default:
                            break;
                    }

                    key_was_pressed = false;
                }
                
                break;

            //===========================================
            case SC_LOGS_MENU:
                /* [A] View Logs
                 * [B] Download
                 * [D] Back
                 */
                if (key_was_pressed) {
                    switch (key) {
                        case 'A':
                            page = 0;
                            setScreen(SC_LOGS_VIEW);
                            break;

                        case 'B':
                            break;

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
            case SC_LOGS_VIEW:
                /*
                 * [A] Slot #<1, 4, 7, 10, 13, 16>
                 * [B] Slot #<2, 5, 8, 11, 14>
                 * [C] Slot #<3, 6, 9, 12, 15>
                 * <[*] BCK[0] [#]>
                */
                if (key_was_pressed) {
                    switch (key) {
                        case '*':
                            // Go to the previous page
                            if (page > 0) {
                                page--;
                                refreshScreen();
                            }
                            break;

                        case '0':
                            // Return to the logs menu
                            setScreen(SC_LOGS_MENU);
                            break;

                        case '#':
                            // Go to the next page
                            if (page < 5) {
                                page++;
                                refreshScreen();
                            }
                            break;

                        case 'A':
                            // View operation from log slot A
                            if (getLogSlot(page * 3) == SLOT_USED) {
                                if (getOperationFromLogs(&CURRENT_OPERATION, (page * 3)) == SUCCESSFUL) {
                                    page = 0;
                                    setScreen(SC_VIEW_RESULTS);
                                } else {
                                    setStatus(ST_ERROR);
                                    setScreen(SC_LOG_VIEW_ERROR);
                                }
                            }
                            break;

                        case 'B':
                            // View operation from log slot B
                            if ((getLogSlot(page * 3) + 1) == SLOT_USED) {
                                if (getOperationFromLogs(&CURRENT_OPERATION, (page * 3) + 1) == SUCCESSFUL) {
                                    page = 0;
                                    setScreen(SC_VIEW_RESULTS);
                                } else {
                                    setStatus(ST_ERROR);
                                    setScreen(SC_LOG_VIEW_ERROR);
                                }
                            }
                            break;

                        case 'C':
                            // View operation from log slot C
                            if ((getLogSlot(page * 3) + 2) == SLOT_USED) {
                                if (getOperationFromLogs(&CURRENT_OPERATION, (page * 3) + 2) == SUCCESSFUL) {
                                    page = 0;
                                    setScreen(SC_VIEW_RESULTS);
                                } else {
                                    setStatus(ST_ERROR);
                                    setScreen(SC_LOG_VIEW_ERROR);
                                }
                            }
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
                            // Increase the number of tires loaded
                            if (loadedTires < MAX_TIRE_CAPACITY) {
                                loadedTires++;
                                refreshScreen();
                            }
                            
                            break;
                            
                        case '*':
                            // Decrease the number of tires loaded
                            if (loadedTires > 0) {
                                loadedTires--;
                                refreshScreen();
                            }
                            
                            break;
                            
                        case 'C':
                            // Start the operation with desired amount of tires
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
                            UART_Write(MSG_STOP);
                            break;

                        default:
                            break;
                    }
 
                    key_was_pressed = false;
                }

                // Listen for messages from the Arduino
                if (UART_Data_Ready()) {
                    messageFromArduino = UART_Read();

                    switch (messageFromArduino) {


                        case MSG_A2P_DRIVING:
                        // Set the status of the robot to driving and refresh the screen
                            setStatus(ST_OPERATE_DRIVING);
                            refreshScreen();
                            break;

                        case MSG_A2P_RETURNING:
                        // Set the status of the robot to returning and refresh the screen
                            setStatus(ST_OPERATE_RETURN);
                            refreshScreen();
                            break;

                        case MSG_A2P_DEPLOY_STEPPER:
                        // Set the status to deploying a tire
                            setStatus(ST_OPERATE_DEPLOYING_TIRE);
                            break;

                        case MSG_A2P_COMPLETE_OP:
                        // Complete the operation on the robot
                            setStatus(ST_COMPLETED_OP);
                            break;

                        default:
                        // Invalid arduino message, throw error
                            setStatus(ST_ERROR);
                            setScreen(SC_UNHANDLED_ARDUINO_MESSAGE_ERROR);

                            lcd_set_ddram_addr(LCD_LINE3_ADDR);
                            printf("%X", messageFromArduino);
                            break;
                    }
                }

                break;
            
            //=============STATUS: COMPLETED=============
            case SC_TERMINATED:
                /* [C] View Results
                 * [D] Finish Op.
                 */
                if (key_was_pressed) {
                    switch (key) {
                        case 'C':
                            // View the results of the operation
                            page = 0;
                            setScreen(SC_VIEW_RESULTS);
                            break;

                        case 'D':
                            // Prompt the user to save
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
                            // Go to the previous page
                            if (page > 0) {
                                page--;
                                refreshScreen();
                            }
                            break;

                        case '0':
                            // Return to the previous screen
                            if (getStatus() == ST_READY) {
                                page = 0;
                                setScreen(SC_LOGS_VIEW);
                            } else {
                                setScreen(SC_TERMINATED);
                            }
                            break;

                        case '#':
                            // Go to the next page
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
                            // Go to save selection screen
                            page = 0;
                            setScreen(SC_SELECT_SAVE_SLOT);
                            break;

                        case 'C':
                            // Return to main menu
                            setStatus(ST_READY);
                            break;

                        case 'D':
                            // Go back to termination screen
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
                /* [A] Slot #<1, 4, 7, 10, 13, 16>
                 * [B] Slot #<2, 5, 8, 11, 14>
                 * [C] Slot #<3, 6, 9, 12, 15>
                 * <[*] BCK[0] [#]>
                */
                if (key_was_pressed) {
                    switch (key) {
                        case '*':
                            // Go to the previous page
                            if (page > 0) {
                                page--;
                                refreshScreen();
                            }
                            break;

                        case '0':
                            // Return to the save screen
                            setScreen(SC_SAVE);
                            break;

                        case '#':
                            // Go to the next page
                            if (page < ((MAX_LOGS - 1) / 2)) {
                                page++;
                                refreshScreen();
                            }
                            break;

                        case 'A':
                            // Save into the first row slot
                            if (getLogSlot(page * 3) == SLOT_USED) {
                                // If the slot is used, prompt user for verification to overwrite
                                setScreen(SC_OVERWRITE_LOG_VERIFICATION_1);
                            } else {
                                // Try to save the operation into the logs and throw an error if failed
                                if (storeOperationIntoLogs(CURRENT_OPERATION, page * 3) == UNSUCCESSFUL) {
                                    setState(ST_ERROR);
                                    setScreen(SC_SAVE_OPERATION_ERROR);
                                } else {
                                    CURRENT_OPERATION.saveSlot = (page * 3) + 1;
                                    setScreen(SC_SAVE_COMPLETED);
                                }
                            }
                            break;

                        case 'B':
                            // Save into the second row slot
                            if (getLogSlot((page * 3) + 1) == SLOT_USED) {
                                // If the slot is used, prompt user for verification to overwrite
                                setScreen(SC_OVERWRITE_LOG_VERIFICATION_2);
                            } else {
                                // Try to save the operation into the logs and throw an error if failed
                                if (storeOperationIntoLogs(CURRENT_OPERATION, (page * 3) + 1) == UNSUCCESSFUL) {
                                    setState(ST_ERROR);
                                    setScreen(SC_SAVE_OPERATION_ERROR);
                                } else {
                                    CURRENT_OPERATION.saveSlot = (page * 3) + 2;
                                    setScreen(SC_SAVE_COMPLETED);
                                }
                            }
                            break;

                        case 'C':
                            // Save into the third row slot
                            if (getLogSlot((page * 3) + 2) == SLOT_USED) {
                                // If the slot is used, prompt user for verification to overwrite
                                setScreen(SC_OVERWRITE_LOG_VERIFICATION_3);
                            } else {
                                // Try to save the operation into the logs and throw an error if failed
                                if (storeOperationIntoLogs(CURRENT_OPERATION, (page * 3) + 2) == UNSUCCESSFUL) {
                                    setState(ST_ERROR);
                                    setScreen(SC_SAVE_OPERATION_ERROR);
                                } else {
                                    CURRENT_OPERATION.saveSlot = (page * 3) + 3;
                                    setScreen(SC_SAVE_COMPLETED);
                                }
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
                            // Return to save slot selection screen
                            setScreen(SC_SELECT_SAVE_SLOT);
                            break;

                        case 'D':
                            // Try to save the operation into the logs and throw an error if failed
                            if (storeOperationIntoLogs(CURRENT_OPERATION, page * 3) == UNSUCCESSFUL) {
                                setState(ST_ERROR);
                                setScreen(SC_SAVE_OPERATION_ERROR);
                            } else {
                                CURRENT_OPERATION.saveSlot = (page * 3) + 1;
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
            case SC_OVERWRITE_LOG_VERIFICATION_2:
                /* [C] No
                 * [D] Yes
                 */
                if (key_was_pressed) {
                    switch (key) {
                        case 'C':
                            // Return to save slot selection screen
                            setScreen(SC_SELECT_SAVE_SLOT);
                            break;

                        case 'D':
                            // Try to save the operation into the logs and throw an error if failed
                            if (storeOperationIntoLogs(CURRENT_OPERATION, (page * 3) + 1) == UNSUCCESSFUL) {
                                setState(ST_ERROR);
                                setScreen(SC_SAVE_OPERATION_ERROR);
                            } else {
                                CURRENT_OPERATION.saveSlot = (page * 3) + 2;
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
            case SC_OVERWRITE_LOG_VERIFICATION_3:
                /* [C] No
                 * [D] Yes
                 */
                
                switch (key) {
                        case 'C':
                            // Return to save slot selection screen
                            setScreen(SC_SELECT_SAVE_SLOT);
                            break;

                        case 'D':
                            // Try to save the operation into the logs and throw an error if failed
                            if (storeOperationIntoLogs(CURRENT_OPERATION, (page * 3) + 2) == UNSUCCESSFUL) {
                                setState(ST_ERROR);
                                setScreen(SC_SAVE_OPERATION_ERROR);
                            } else {
                                CURRENT_OPERATION.saveSlot = (page * 3) + 3;
                                setScreen(SC_SAVE_COMPLETED);
                            }
                            break;

                        default:
                            break;
                    }
 
                    key_was_pressed = false;
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
            
            //============= STATUS: ERROR ===============
            case SC_LOG_VIEW_ERROR:
                /* [D] OK
                 */

                if (key_was_pressed) {
                    switch (key) {
                        case 'D':
                            // Return to the main menu
                            setStatus(ST_READY);
                            break;

                        default:
                            break;
                    }
 
                    key_was_pressed = false;
                } 
                break;

            //===========================================
            case SC_INVALID_STATE_ERROR:
                /* [D] OK
                 */

                if (key_was_pressed) {
                    switch (key) {
                        case 'D':
                            // Return to the main menu
                            setStatus(ST_READY);
                            break;

                        default:
                            break;
                    }
 
                    key_was_pressed = false;
                } 
                break;

            //===========================================
            case SC_INVALID_SCREEN_ERROR:
                /* [D] OK
                 */

                if (key_was_pressed) {
                    switch (key) {
                        case 'D':
                            // Return to the main menu
                            setStatus(ST_READY);
                            break;

                        default:
                            break;
                    }
 
                    key_was_pressed = false;
                } 
                break;

            //===========================================
            case SC_UNHANDLED_ARDUINO_MESSAGE_ERROR:
                /* [D] OK
                 */

                if (key_was_pressed) {
                    switch (key) {
                        case 'D':
                            // Return to the main menu
                            setStatus(ST_READY);
                            break;

                        default:
                            break;
                    }
 
                    key_was_pressed = false;
                } 
                break;
            //===========================================
            case SC_SAVE_OPERATION_ERROR:
                /* [D] OK
                 */

                if (key_was_pressed) {
                    switch (key) {
                        case 'D':
                            // Return to the operation complete screen
                            setStatus(ST_COMPLETED_OP);
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

        if (getStatus() == ST_READY) {
            // Increment timer for returning to standby while in any menu state
            returnToStandbyTick++;
            if (returnToStandbyTick == 1000) {
                returnToStandbyTick = 0;

                returnToStandbyCounter++;

                if (returnToStandbyCounter == RETURN_TO_STANDBY_TIME_SECONDS) {
                    // After RETURN_TO_STANDBY_TIME_SECONDS, return to Standby status
                    returnToStandbyCounter = 0;
                    setStatus(ST_STANDBY);
                }
            }
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

    // E0, E1, A4 = Stepper Motor
    TRISEbits.TRISE0 = 0;
    TRISEbits.TRISE1 = 0;
    TRISAbits.TRISA4 = 0;
    LATEbits.LATE0 = 0;
    LATEbits.LATE1 = 0;
    LATAbits.LATA4 = 0;

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
    INT0E = 1;
    
    // Enable RC7 (RX) interrupt
    RCIE = 1;
    
    // Initialize LCD
    initLCD();
    
    // Initialize UART
    UART_Init(9600);

   // Initialize I2C Master with 100 kHz clock
    I2C_Master_Init(100000); 

    // Enable interrupts
    ei();
    
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

    // Update the status
    CURRENT_STATUS = newStatus;

    switch (newStatus) {
        case ST_STANDBY:
        // Places the robot into standby mode (where time displays)
            setScreen(SC_STANDBY);
            break;
            
        case ST_READY:
        // Status to navigate robot menus
            setScreen(SC_MENU);

            // Disable emergency_stop_pressed flag
            emergency_stop_pressed = false;
            break;

        case ST_ERROR:
        // Status when an error occurs (prevents screen from automatically returning over time)
            // do nothing
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

            setScreen(SC_OPERATING);
            break;

        case ST_OPERATE_DRIVING:
        // Status used while the robot is driving/scanning for poles
            // do nothing
            break;

        case ST_OPERATE_POLE_DETECTED:
        // Status used when a pole is detected
            // do nothing
            break;

        case ST_OPERATE_DEPLOYING_TIRE:
        // Robot is deploying a tire using the stepper motor
            // drive the stepper motor forward
            driveStepper(REVOLUTIONS_TO_DROP_ONE_TIRE, FORWARD);

            // tell arduino that deployment was completed
            UART_Write(MSG_DEPLOYMENT_COMPLETE);
            break;

        case ST_OPERATE_RETURN:
        // Status used when the robot is returning to the start
        // do nothing
            break;

        case ST_COMPLETED_OP:
        // Status used when the operation is completed
            setScreen(SC_TERMINATED);
            break;

        default:
        // For an invalid state, throw an error
            setState(ST_ERROR);
            setScreen(SC_INVALID_SCREEN_ERROR)
            break;
    }
}

// Screen functions
Screen getScreen(void) {
    return CURRENT_SCREEN;
}

void setScreen(Screen newScreen) {
    // Sets the new screen and performs an initial function

    CURRENT_SCREEN = newScreen;

    if (getStatus() == ST_READY) {
        // Refresh returnToStandby timers when screen changes
        returnToStandbyTick = 0;
        returnToStandbyCounter = 0;
    }

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
                        "[B] UNAVAILABLE ",
                        "                ",
                        "[D] Back        ");
            break;

        case SC_LOGS_VIEW:
            // Menu to view all the logs
            displayMenuPage("                ",
                            "                ",
                            "                ",
                            page > 0, page < (MAX_LOGS - 1) / 3);

            // First row of slots
            lcd_set_ddram_addr(LCD_LINE1_ADDR);

            // Display whether the slot is taken or not
            if (getLogSlot(page * 3) == SLOT_USED) {
                printf("[A] Slot %02d     ", (page * 3) + 1);
            } else {
                printf("Slot %02d is empty", (page * 3) + 1);
            }

            // Second row of slots
            lcd_set_ddram_addr(LCD_LINE2_ADDR);

            // Display whether the slot is taken or not
            if (getLogSlot((page * 3) + 1) == SLOT_USED) {
                printf("[B] Slot %02d     ", (page * 3) + 2);
            } else {
                printf("Slot %02d is empty", (page * 3) + 2);
            }

            // Third row of slots
            lcd_set_ddram_addr(LCD_LINE3_ADDR);

            // Display whether the slot is taken or not
            if (getLogSlot((page * 3) + 2) == SLOT_USED) {
                printf("[C] Slot %02d     ", (page * 3) + 3);
            } else {
                printf("Slot %02d is empty", (page * 3) + 3);
            }

        break;
            
        case SC_DEBUG:
        // Debug menu
            displayPage("[A] Motor Debug ",
                        "[B] Log Debug   ",
                        "[C] Sensor Debug",
                        "[D] Back        ");
            break;

        case SC_DEBUG_LOG:
        // Log debug menu
            displayPage("                ",
                        "                ",
                        "[C] Reset Slots ",
                        "[D] Back        ");

            // Display the number of log slots available
            lcd_set_ddram_addr(LCD_LINE1_ADDR);
            printf("Slots Open:    %01d", getSlotsAvailable());

            // Display the slots taken
            lcd_set_ddram_addr(LCD_LINE2_ADDR);
            for (char i = 0; i < 16; i++) {
                printf("%c", getLogSlot(i) == SLOT_USED ? '1' : '0');
            }

            break;
            
        case SC_DEBUG_MOTOR:
        // Motor debug menu
            displayPage("[A] Forward     ",
                        "[B] Backward    ",
                        "[C] Off         ",
                        "[D] Return      ");
            break;
            
        case SC_DEBUG_STEPPER:
        // Stepper debug menu
            displayPage("[1] 1 Forward   ",
                        "[3] 1 Back      ",
                        "[4] Tire Forward",
                        "[6] Tire Back   ");
            break;

        case SC_DEBUG_SENSOR:
        // Sensor debug menu
            displayPage("                ",
                        "                ",
                        "[C] Read Sensor ",
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

            // Display something based on the status
            switch (getStatus()) {
                case ST_OPERATE_START:
                    printf(" Starting op... ");
                    break;

                case ST_OPERATE_DRIVING:
                    // Display position while driving
                    printf("    DRIVING     ");
                    lcd_set_ddram_addr(LCD_LINE2_ADDR);

                    // Request and print the position from the Arduino onto the LCD
                    printf("Position:    %03d", UART_Request_Short(MSG_P2A_REQUEST_POSITION));
                    break;

                case ST_OPERATE_POLE_DETECTED:
                    // Display message while pole is detected`
                    printf(" POLE DETECTED  ");
                    lcd_set_ddram_addr(LCD_LINE2_ADDR);

                    // Request and print the number of tires found from the Arduino onto the LCD
                    printf("Tires Found:  %02d", UART_Request_Byte(MSG_P2A_REQUEST_TIRES_FOUND);

                    break;

                case ST_OPERATE_DEPLOYING_TIRE:
                    // Display tires remaining while robot is deploying
                    printf(" DEPLOYING TIRE ");
                    lcd_set_ddram_addr(LCD_LINE2_ADDR);

                    // Request and print the number of tires remaining on the robot
                    printf("Tire Ammo:    %02d", UART_Request_Byte(MSG_P2A_REQUEST_TIRES_REMAINING));
                    break;
                    
                case ST_OPERATE_RETURN:
                    // Display the position while returning
                    printf("    RETURNING   ");
                    lcd_set_ddram_addr(LCD_LINE2_ADDR);

                    // Request and print the position from the Arduino onto the LCD
                    printf("Position:    %03d", UART_Request_Short(MSG_P2A_REQUEST_POSITION));
                    break;
                    
                default:
                    break;
            }
            break;

        case SC_TERMINATED:
        // Termination screen when the operation is complete
            displayPage("                ",
                        "                ",
                        "[C] View Results",
                        "[D] Finish Op.  ");

            lcd_set_ddram_addr(LCD_LINE1_ADDR);

            // Display a termination message dependant on whether the emergency stop was pressed
            if (emergency_stop_pressed) {
                printf(" Op. Terminated ");
                lcd_set_ddram_addr(LCD_LINE2_ADDR);
                printf("EMERGNCY STOPPED");
            } else {
                printf(" Op. Completed  ");
            }

            break;

        case SC_VIEW_RESULTS:
            // Screen to view the results of the operation (or viewing them through the logs)

            // Display menu framework (max pages is 2 + number of poles detected)
            displayMenuPage("                ",
                            "                ",
                            "                ",
                            page > 0, page < (CURRENT_OPERATION.totalNumberOfPoles + 1));

            // Page 1 (indexed at 0): displays temporal information
            if (page == 0) {
                // Display the month, day and year
                lcd_set_ddram_addr(LCD_LINE1_ADDR);
                printf("Day: %s %02X/19", months[CURRENT_OPERATION.startTime[4]], CURRENT_OPERATION.startTime[3]);
                // Display the time the operation started
                lcd_set_ddram_addr(LCD_LINE2_ADDR);
                printf("Time:   %02X:%02X:%02X", CURRENT_OPERATION.startTime[2], CURRENT_OPERATION.startTime[1], CURRENT_OPERATION.startTime[0]);
                // Display the duration of the operation
                lcd_set_ddram_addr(LCD_LINE3_ADDR);
                printf("Duration:  %02d:%02d", CURRENT_OPERATION.duration / 60, CURRENT_OPERATION.duration % 60);

            // Page 2 (indexed at 1): displays information about the total number of poles/tires
            } else if (page == 1) {
                // Display the total number of poles
                lcd_set_ddram_addr(LCD_LINE1_ADDR);
                printf("Poles Found:  %02d", CURRENT_OPERATION.totalNumberOfPoles);
                // Display the total number of supplied tires
                lcd_set_ddram_addr(LCD_LINE2_ADDR);
                printf("Total Tires:  %02d", CURRENT_OPERATION.totalSuppliedTires);

            // Page 3-12 (indexed at 2-11): displays information about specific poles
            } else if (page > 1 && page < CURRENT_OPERATION.totalNumberOfPoles + 2) {
                // Display the pole being described
                lcd_set_ddram_addr(LCD_LINE1_ADDR);
                printf("Pole #%02d  %03d cm", page - 1, CURRENT_OPERATION.distanceOfPole[page - 2]);
                // Display the number of tires stacked onto the pole
                lcd_set_ddram_addr(LCD_LINE2_ADDR);
                printf("Tires Stacked: %01d", CURRENT_OPERATION.tiresDeployedOnPole[page - 2]);
                // Display the number of tires on the pole after the operation
                lcd_set_ddram_addr(LCD_LINE3_ADDR);
                printf("Tires on Pole: %01d", CURRENT_OPERATION.tiresOnPoleAfterOperation[page - 2]);
            }
            break;

        case SC_SAVE:
        // Ask user if they would like to save operation or continue without saving
            displayPage("Save operation?     ",
                        "[B] Save            ",
                        "[C] Don't Save      ",
                        "[D] Back            ");
            break;

        case SC_SELECT_SAVE_SLOT:
        // Display menu page for user to select slot to save the operation

            // Display the menu framework (max pages holds up to MAX_LOGS entries)
            displayMenuPage("                ",
                            "                ",
                            "                ",
                            page > 0, page < (MAX_LOGS - 1) / 3);

            // Display the slots available and add "USED" if the slot is taken
            lcd_set_ddram_addr(LCD_LINE1_ADDR);
            printf("[A] Slot %02d %s", (page * 3) + 1, getLogSlot(page * 3) == SLOT_USED ? "USED" : "    ");
            lcd_set_ddram_addr(LCD_LINE2_ADDR);
            printf("[B] Slot %02d %s", (page * 3) + 2, getLogSlot((page * 3) + 1) == SLOT_USED ? "USED" : "    ");
            lcd_set_ddram_addr(LCD_LINE3_ADDR);
            printf("[C] Slot %02d %s", (page * 3) + 3, getLogSlot((page * 3) + 2) == SLOT_USED ? "USED" : "    ");

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
            printf("    slot %02d     ", CURRENT_OPERATION.saveSlot);
            break;

        case SC_LOG_VIEW_ERROR:
        // Display unsuccessful retrieval of operation error
            displayPage("     ERROR      ",
                        " Unable to read ",
                        " operation data ",
                        "[D] OK          ");
            break;

        case SC_INVALID_SCREEN_ERROR:
        // Display invalid screen error
            displayPage("     ERROR      ",
                        " Invalid screen ",
                        "                ",
                        "[D] OK          ");
            break;

        case SC_INVALID_STATE_ERROR:
        // Display invalid state error
            displayPage("     ERROR      ",
                        " Invalid state  ",
                        "                ",
                        "[D] OK          ");
            break;

        case SC_SAVE_OPERATION_ERROR:
        // Display saving operation error
            displayPage("     ERROR      ",
                        " Unable to save ",
                        "  the operation ",
                        "[D] OK          ");
            break;

        default:
        // For an invalid screen, throw an error
            setStatus(ST_ERROR);
            setScreen(SC_INVALID_SCREEN_ERROR);
            break;
    }
}

void refreshScreen(void) {
    setScreen(getScreen());
}

void activateEmergencyStop(void) {
    // Disable all actuators and display a terminated screen

    // Tell arduino to stop DC motors
    UART_Write(MSG_P2A_STOP);

    // Disable stepper motor
    STEPPER_EN = 0;

    // Display termination screen
    setStatus(ST_COMPLETED_OP);
}

// Interrupt Functions
void __interrupt() interruptHandler(void){
// Handles interrupts received from RB0 (emergency stop) and RB1 (keypad)
    if (INT1IE && INT1IF) {
        // Set a flag to handle interrupt and clear interrupt flag bit
        key_was_pressed = true;
        key = keys[(PORTB & 0xF0) >> 4];
        INT1IF = 0;

    } else if (INT0IE && INT0IF) {
        emergency_stop_pressed = true;
        activateEmergencyStop();
        INT0IF = 0;
    }
}