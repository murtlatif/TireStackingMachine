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
#define MOTOR_ERROR_PROPORTIONALITY_CONSTANT 1
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
    SC_DEBUG_LOG,
    SC_DEBUG_MOTOR,
    SC_DEBUG_STEPPER,
    SC_DEBUG_SENSOR,
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
    SC_SAVE_COMPLETED,
    SC_MEMORY_ERROR
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
    0x00, // Seconds 
    0x10, // Minutes
    0x17, // Hours (set to 24 hour mode)
    0x01, // Weekday
    0x24, // Day of month
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
                /*
                 * 
                 * [C] Reset Slots
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
                /* [A] Clockwise
                 * [B] CounterClockwise
                 * [C] Off
                 * [D] Back
                 */

                if (key_was_pressed) {
                    switch (key) {
                        case 'A':
                            driveDCMotor(BOTH, CLOCKWISE);
                            break;

                        case 'B':
                            driveDCMotor(BOTH, COUNTER_CLOCKWISE);
                            break;

                        case 'C':
                            driveDCMotor(BOTH, OFF);
                            break;

                        case 'D':
                            setScreen(SC_DEBUG);
                            break;

                        case '1':
                            setScreen(SC_DEBUG_STEPPER);
                            break;
                            
                        case '2':
                            if (onDuty > 0) {
                                onDuty--;
                            }
                            lcd_home();
                            printf("On Duty:     %03d", onDuty);
                            break;
                            
                        case '3':
                            if (onDuty < 100) {
                                onDuty++;
                            }
                            lcd_home();
                            printf("On Duty:     %03d", onDuty);
                            break;
                            
                        case '4':
                            onDuty = 0;
                            break;
                            
                        case '5':
                            if (onDuty <= 10) {
                                onDuty = 0;
                            } else {
                                onDuty-= 10;
                            }
                            lcd_home();
                            printf("On Duty:     %03d", onDuty);
                            break;
                            
                        case '6':
                            if (onDuty >= 90) {
                                onDuty = 100;
                            } else {
                                onDuty += 10;
                            }
                            lcd_home();
                            printf("On Duty:     %03d", onDuty);
                            break;
                            
                        case '7':
                            onDuty = 50;
                            break;
                            
                        case '0':
                            pwmDrive = 0;
                            break;
                            
                        case '*':
                            pwmDrive = 1;
                            break;
                            
                        case '#':
                            pwmDrive = 2;
                            break;
                            
                        default:
                            break;
                    }

                    key_was_pressed = false;
                }
                
                if (pwmTick < onDuty) {
                    MOTOR1_IN1 = (pwmDrive == 1);
                    MOTOR2_IN1 = (pwmDrive == 1);
                    MOTOR1_IN2 = (pwmDrive == 2);
                    MOTOR2_IN2 = (pwmDrive == 2);
                } else {
                    MOTOR1_IN1 = 0;
                    MOTOR1_IN2 = 0;
                    MOTOR2_IN1 = 0;
                    MOTOR2_IN2 = 0;
                }
                
                if (pwmTick == 100) {
                    pwmTick = 0;
                }
                
                pwmTick++;
                

                break;
                
            
            //===========================================
            case SC_DEBUG_STEPPER:
                
                if (key_was_pressed) {
                    switch (key) {
                        case 'A':
                            stepStepper(step);
                            break;

                        case 'B':
                            if (step == 3) {
                                step = 0;
                            } else {
                                step++;
                            }
                            
                            break;

                        case 'C':
                            if (step == 0) {
                                step = 3;
                            } else {
                                step--;
                            }
                            
                            break;
                            
                        case 'D':
                            setScreen(SC_DEBUG);
                            break;

                        case '1':
                            stepsRemaining = STEPS_FOR_ONE_REVOLUTION;
                            step = 0;
                            
                            break;
                            
                        case '2':
                            stepperTick = 0;
                            while (stepsRemaining > 0) {
                                if (stepperTick == 2) {
                                    stepStepper(step);
                                
                                    if (step == 3) {
                                        step = 0;
                                    } else {
                                        step++;
                                    }
                                
                                    stepperTick = 0;
                                    stepsRemaining--;
                                }
                                __delay_ms(1);
                                stepperTick++;
                            }
                            break;
                            
                        case '4':
                            spinning = 0x1;
                            break;
                            
                        case '5':
                            spinning = 0x2;
                            break;
                            
                        case '6':
                            spinning = 0x0;
                            break;
                            
                        case '7':
                            STEPPER_IN1 = 1;
                            STEPPER_IN3 = 1;
                            STEPPER_IN2 = 0;
                            for (char i = 0; i < 200; i ++) {
                                STEPPER_IN2 = 1;
                                __delay_ms(1);
                                STEPPER_IN2 = 0;
                                __delay_ms(1);
                            }
                            break;
                            
                        case '8':
                            STEPPER_IN1 = 0;
                            STEPPER_IN3 = 1;
                            STEPPER_IN2 = 0;
                            for (char i = 0; i < 200; i ++) {
                                STEPPER_IN2 = 1;
                                __delay_us(550);
                                STEPPER_IN2 = 0;
                                __delay_us(550);
                            }
                            break;
                            
                        case '9':
                            STEPPER_IN1 = 0;
                            STEPPER_IN2 = 0;
                            STEPPER_IN3 = 0;
                            STEPPER_IN4 = 0;
                            break;
                            
                        case '*':
                            STEPPER_IN1 = 1;
                            STEPPER_IN3 = 1;
                            STEPPER_IN2 = 0;
                            for (short i = 0; i < 4000; i ++) {
                                STEPPER_IN2 = 1;
                                __delay_us(600);
                                STEPPER_IN2 = 0;
                                __delay_us(600);
                            }
                            break;
                            
                        case '#':
                            STEPPER_IN1 = 0;
                            STEPPER_IN3 = 1;
                            STEPPER_IN2 = 0;
                            for (short i = 0; i < 4000; i ++) {
                                STEPPER_IN2 = 1;
                                __delay_us(600);
                                STEPPER_IN2 = 0;
                                __delay_us(600);
                            }
                            
                        default:
                            break;
                    }

                    key_was_pressed = false;
                }
                
                switch (spinning) {
                    case 0x1:
                        if (stepperTick == 2) {
                            stepStepper(step);

                            if (step == 3) {
                                step = 0;
                            } else {
                                step++;
                            }

                            stepperTick = 0;
                            stepsRemaining--;
                        }
                        stepperTick++;
                        
                        break;
                        
                    case 0x2:
                        if (stepperTick == 2) {
                            stepStepper(step);

                            if (step == 0) {
                                step = 3;
                            } else {
                                step--;
                            }

                            stepperTick = 0;
                            stepsRemaining--;
                        }
                        stepperTick++;
                        break;
                        
                    default:
                        break;
                        
                        
                }
                break;
                
            //===========================================
            case SC_DEBUG_SENSOR:
                /* Distance:  ###mm
                 * 
                 * [C] Read Sensor
                 * [D] Back
                 */
                if (key_was_pressed) {
                    switch (key) {
                        case 'C':
                            lcd_home();
                            printf("Reading distance");
                            sensorReading = readSensor();
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
                 * 
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
                            if (page > 0) {
                                page--;
                                refreshScreen();
                            }
                            break;

                        case '0':
                            setScreen(SC_LOGS_MENU);
                            break;

                        case '#':
                            if (page < 5) {
                                page++;
                                refreshScreen();
                            }
                            break;

                        case 'A':
                            if (getLogSlot(page * 3) == USED) {
                                if (getCondensedOperationFromLogs(CURRENT_OPERATION, (page * 3)) == FAIL) {
                                    setScreen(SC_MEMORY_ERROR);
                                } else {
                                    unpackCondensedOperation(CURRENT_OPERATION);
                                    page = 0;
                                    setScreen(SC_VIEW_RESULTS);
                                }
                            }
                            break;

                        case 'B':
                            if (page < 5 && getLogSlot((page * 3) + 1) == USED) {
                                if (getCondensedOperationFromLogs(CURRENT_OPERATION, ((page * 3) + 1)) == FAIL) {
                                    setScreen(SC_MEMORY_ERROR);
                                } else {
                                    unpackCondensedOperation(CURRENT_OPERATION);
                                    page = 0;
                                    setScreen(SC_VIEW_RESULTS);
                                }
                            }
                            break;

                        case 'C':
                            if (page < 5 && getLogSlot((page * 3) + 2) == USED) {
                                if (getCondensedOperationFromLogs(CURRENT_OPERATION, ((page * 3) + 2)) == FAIL) {
                                    setScreen(SC_MEMORY_ERROR);
                                } else {
                                    unpackCondensedOperation(CURRENT_OPERATION);
                                    page = 0;
                                    setScreen(SC_VIEW_RESULTS);
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
                            pole_found = true;
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

                        // Give a buffer distance before beginning detecting poles again
                        } else if (CURRENT_OPERATION.position - CURRENT_OPERATION.distanceOfPole[CURRENT_OPERATION.totalNumberOfPoles - 1] > SAME_POLE_REGION) {
                            if (pole_found) {
                                CURRENT_OPERATION.totalNumberOfPoles++;
                                CURRENT_OPERATION.distanceOfPole[CURRENT_OPERATION.totalNumberOfPoles - 1] = CURRENT_OPERATION.position;
                                CURRENT_OPERATION.tiresOnPoleAfterOperation[CURRENT_OPERATION.totalNumberOfPoles - 1] = getNumberOfTiresOnPole();
                                CURRENT_OPERATION.tiresDeployedOnPole[CURRENT_OPERATION.totalNumberOfPoles - 1] = 0;
                                setStatus(ST_OPERATE_POLE_DETECTED);
                                refreshScreen();
                            } else {
                                if (readSensor() <= POLE_DETECTED_RANGE) {
                                    bool consistentOutput = true;
                                    for (char i = 0; i < SENSOR_VERIFICATION_TRIES; i++) {
                                        if (readSensor() > POLE_DETECTED_RANGE) {
                                            consistentOutput = false;
                                        }
                                    }

                                    if (consistentOutput) {
                                        pole_found = true;
                                    }
                                }
                            }
                        }

                        drivingTick++;
                        if (drivingTick % 10 == 0) {
                            // ADJUSTING SPEED TO HAVE STRAIGHT DRIVING
                            // unsigned char error = getEncoderValue(MOTOR1) - getEncoderValue(MOTOR2);
                            // use error to set new speed
                            // speed = error / MOTOR_ERROR_PROPORTIONALITY_CONSTANT;

                            // MEASURING DISTANCE
                            // CURRENT_OPERATION.position += getEncoderValue(MASTER) * someFloatConstant

                        }

                        if (drivingTick == 100) {
                            CURRENT_OPERATION.position++;
                            drivingTick = 0;
                            refreshScreen();
                        }
                        
                        break;

                    case ST_OPERATE_POLE_DETECTED:
//                        CURRENT_OPERATION.tiresOnPoleAfterOperation[CURRENT_OPERATION.totalNumberOfPoles - 1] = getNumberOfTiresOnPole();
                        CURRENT_OPERATION.tiresOnPoleAfterOperation[CURRENT_OPERATION.totalNumberOfPoles - 1] = CURRENT_OPERATION.tiresDeployedOnPole[CURRENT_OPERATION.totalNumberOfPoles - 1];
                        // Deploy tire if tire required on pole
                        if (getNumberOfTiresRequiredForPole(CURRENT_OPERATION) > CURRENT_OPERATION.tiresOnPoleAfterOperation[CURRENT_OPERATION.totalNumberOfPoles - 1]) {
                            if (CURRENT_OPERATION.tiresRemaining == 0) {
                                setStatus(ST_OPERATE_RETURN);
                                refreshScreen();
                            } else {
                                stepperTick = 0;
                                stepsRemaining = STEPS_FOR_ONE_REVOLUTION;
                                setStatus(ST_OPERATE_DEPLOYING_TIRE);
                                refreshScreen();
                            }
                        } else {
                        // Continue driving if no more tires required
                            if (CURRENT_OPERATION.totalNumberOfPoles == 10) {
                                // Drive back to start if this pole was the 10th pole
                                drivingTick = 100 - drivingTick;
                                setStatus(ST_OPERATE_RETURN);
                                refreshScreen();
                            } else {
                                setStatus(ST_OPERATE_DRIVING);
                                refreshScreen();
                            }
                        }
                        
                        pole_found = false;
                        break;

                    case ST_OPERATE_DEPLOYING_TIRE:
                        // Step stepper every 2 ms
                        stepperTick++;
                        if (stepperTick == 2) {
                            
                            stepStepper(step);
                            
                            // Rotate clockwise
                            if (step == 3) {
                                step = 0;
                            } else {
                                step++;
                            }
                            
                            stepsRemaining--;
                            stepperTick = 0;
                            
                            if (stepsRemaining == 0) {
                                CURRENT_OPERATION.totalSuppliedTires++;
                                CURRENT_OPERATION.tiresDeployedOnPole[CURRENT_OPERATION.totalNumberOfPoles - 1]++;
                                CURRENT_OPERATION.tiresRemaining--;
                                setStatus(ST_OPERATE_POLE_DETECTED);
                                refreshScreen();
                            }
                        }
                        break;

                    case ST_OPERATE_RETURN:
                        if (CURRENT_OPERATION.position == 0) {
                            setStatus(ST_COMPLETED_OP);
                        } else if (drivingTick == 100) {
                            CURRENT_OPERATION.position--;
                            drivingTick = 0;
                            refreshScreen();
                        }

                        drivingTick++;

                    default:
                        break;
                }

                durationTick++;
                
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
                            if (getStatus() == ST_READY) {
                                page = 0;
                                setScreen(SC_LOGS_VIEW);
                            } else {
                                setScreen(SC_TERMINATED);
                            }
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
                /*
                 * [A] Slot #<1, 4, 7, 10, 13, 16>
                 * [B] Slot #<2, 5, 8, 11, 14>
                 * [C] Slot #<3, 6, 9, 12, 15>
                 * <[*] BCK[0] [#]>
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
                            if((saveCondensedOperationIntoLogs(page * 3, CURRENT_OPERATION)) == FAIL) {
                                setScreen(SC_MEMORY_ERROR);
                            } else {
                                CURRENT_OPERATION.savedSlot = (page * 3) + 1;
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
                            setScreen(SC_SELECT_SAVE_SLOT);
                            break;

                        case 'D':
                            storeCondensedOperation(CURRENT_OPERATION);
                            if((saveCondensedOperationIntoLogs((page * 3) + 1, CURRENT_OPERATION)) == FAIL) {
                                setScreen(SC_MEMORY_ERROR);
                            } else {
                                CURRENT_OPERATION.savedSlot = (page * 3) + 2;
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
                            setScreen(SC_SELECT_SAVE_SLOT);
                            break;

                        case 'D':
                            storeCondensedOperation(CURRENT_OPERATION);
                            if((saveCondensedOperationIntoLogs((page * 3) + 2, CURRENT_OPERATION)) == FAIL) {
                                setScreen(SC_MEMORY_ERROR);
                            } else {
                                CURRENT_OPERATION.savedSlot = (page * 3) + 3;
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
            
            //===========================================
            case SC_MEMORY_ERROR:
                /* [D] Back
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

            //===========================================
            default:
                break;
        }

        // 1ms delay for every iteration
        __delay_ms(1);
        tick++;
        
        if (tick == 1000) {
            // This occurs every second
            tick = 0;
        }
    }
}

//===================== FUNCTIONS =====================//
// State functions
void initialize(void) {
    
    // C0, C1 = DC Motor 1
    TRISCbits.TRISC0 = 0;
    TRISCbits.TRISC1 = 0;
    LATCbits.LATC0 = 0;
    LATCbits.LATC1 = 0;

    // A0, A1 = DC Motor 2
    TRISAbits.TRISA0 = 0;
    TRISAbits.TRISA1 = 0;
    LATAbits.LATA0 = 0;
    LATAbits.LATA1 = 0;

    // E0, E1, A4, A5 = Stepper Motor
    TRISEbits.TRISE0 = 0;
    TRISEbits.TRISE1 = 0;
    TRISAbits.TRISA4 = 0;
    TRISAbits.TRISA5 = 0;
    LATEbits.LATE0 = 0;
    LATEbits.LATE1 = 0;
    LATAbits.LATA4 = 0;
    LATAbits.LATA5 = 0;

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
    
//    // Initialize I2C Master with 100 kHz clock
//    // Write the address of the slave device, that is, the Arduino Nano. Note
//    // that the Arduino Nano must be configured to be running as a slave with
//    // the same address given here.
    I2C_Master_Init(100000); 
//    I2C_Master_Start();
//    I2C_Master_Write(0b00010000); // 7-bit Arduino slave address + write
//    I2C_Master_Stop();

    // Enable interrupts
    ei();
    
    // Initialize Status
    setStatus(ST_STANDBY);
    
    // Write time ( DO NOT UNCOMMENT )
    rtcSetTime(valueToWriteToRTC);
}

Status getStatus(void) {
    return CURRENT_STATUS;
}

void setStatus(Status newStatus) {
    // Sets the new status and performs an initial function

    // Turn off motors if not driving
    if (newStatus != ST_OPERATE_DRIVING || newStatus != ST_OPERATE_RETURN) {
        driveDCMotor(BOTH, OFF);
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
            driveDCMotor(BOTH, CLOCKWISE);
            
            break;

        case ST_OPERATE_POLE_DETECTED:
            
            break;

        case ST_OPERATE_DEPLOYING_TIRE:
        // Robot deploys pole

            break;

        case ST_OPERATE_RETURN:
        // Robot returns back to the start
            driveDCMotor(BOTH, COUNTER_CLOCKWISE);
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

        case SC_LOGS_VIEW:
        // Menu to view all the logs
        displayMenuPage("                ",
                        "                ",
                        "                ",
                        page > 0, page < 5);

        // First row of slots
        lcd_set_ddram_addr(LCD_LINE1_ADDR);
        if (getLogSlot(page * 3) == USED) {
            // Display whether the slot is taken or not
            printf("[A] Slot %02d     ", (page * 3) + 1);
        } else {
            printf("Slot %02d is empty", (page * 3) + 1);
        }
        if (page < 5) {
            // Second row of slots
            lcd_set_ddram_addr(LCD_LINE2_ADDR);
            if (getLogSlot((page * 3) + 1) == USED) {
                printf("[B] Slot %02d     ", (page * 3) + 2);
            } else {
                printf("Slot %02d is empty", (page * 3) + 2);
            }
            // Third row of slots
            lcd_set_ddram_addr(LCD_LINE3_ADDR);
            if (getLogSlot((page * 3) + 2) == USED) {
                printf("[C] Slot %02d     ", (page * 3) + 3);
            } else {
                printf("Slot %02d is empty", (page * 3) + 3);
            }
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

            lcd_set_ddram_addr(LCD_LINE1_ADDR);
            printf("Logs: %d", getSlotsUsed());
            lcd_set_ddram_addr(LCD_LINE2_ADDR);
            for (char i = 0; i < 16; i++) {
                printf("%c", getLogSlot(i) == USED ? '1' : '0');
            }
            break;
            
        case SC_DEBUG_MOTOR:
        // Motor debug menu
            displayPage("[A] Drive CW    ",
                        "[B] Drive CCW   ",
                        "[C] OFF         ",
                        "[D] Back        ");
            break;
            
        case SC_DEBUG_STEPPER:
        // Stepper debug menu
            displayPage("[A] Send step   ",
                        "[B] Step++      ",
                        "[C] Step--      ",
                        "[D] Back        ");
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
            displayPage(" Operation Done ",
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
                printf("Pole #%02d  %03d cm", page - 1, CURRENT_OPERATION.distanceOfPole[page - 2]);
                lcd_set_ddram_addr(LCD_LINE2_ADDR);
                printf("Tires Stacked: %01d", CURRENT_OPERATION.tiresDeployedOnPole[page - 2]);
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

        case SC_MEMORY_ERROR:
        // Display error
            displayPage("  MEMORY ERROR  ",
                        "                ",
                        "                ",
                        "[D] Back        ");
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