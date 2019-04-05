#include <SoftwareSerial.h>
#include "Adafruit_VL53L0X.h"

// Constants
const byte POLE_DETECTED_RANGE = 190; 
const byte VALID_VERIFICATION_COUNT = 2; // How many times the sensor should detect the pole
const byte POLE_DETECTED_BUFFER_REGION = 15; // How much distance it must travel before it can detect a new pole
const byte MAX_POLES = 10;
const byte MAX_DISTANCE_CM = 400;

// PIN definitions
    // UART
const byte rxPin = 1;
const byte txPin = 0;
    // ENCODER
const byte encoder_master_pinA = 4; //master encoder A pin -> the digital pin 3
const byte encoder_master_pinB = 2; //master encoder B pin -> the interrupt pin 0
const byte encoder_slave_pinA = 7;  //A pin -> the digital pin 5
const byte encoder_slave_pinB = 3;  //B pin -> the interrupt pin 1
    // MOTORS
const byte leftmotor_pinA = 5;
const byte leftmotor_pinB = 6;
const byte rightmotor_pinA = 9;
const byte rightmotor_pinB = 10;

    // OTHER
const byte pole_detected_signal = 8;

// Structs
typedef struct {
     unsigned char totalSuppliedTires;
     unsigned char totalNumberOfPoles;
     unsigned char tiresDeployedOnPole[10];
     unsigned char tiresOnPoleAfterOperation[10];
     unsigned short distanceOfPole[10];

     // Unstored (temporary) information
     unsigned char tiresRemaining;
     unsigned short position;
} Operation;

const Operation emptyOp = { .totalSuppliedTires = 0,
                            .totalNumberOfPoles = 0,
                            .tiresDeployedOnPole = {},
                            .tiresOnPoleAfterOperation = {},
                            .distanceOfPole = {},
                            .tiresRemaining = 0,
                            .position = 0
                            };

// Types
typedef enum {
    IDLE = 0,
    DRIVING,
    DEPLOYING,
    POLE_DETECTED,
    RETURNING,
    COMPLETE
} STATE;

typedef enum {
    // PIC to Arduino Messages
    MSG_P2A_STOP = 0,
    MSG_P2A_START,
    MSG_P2A_DEPLOYMENT_COMPLETE,
    MSG_P2A_DEBUG_DRIVE_FORWARD,
    MSG_P2A_DEBUG_DRIVE_BACKWARD,
    MSG_P2A_DEBUG_STOP,
    MSG_P2A_DEBUG_SENSOR,

    // Arduino to PIC Messages
    MSG_A2P_DRIVING = 100,
    MSG_A2P_DEPLOY_STEPPER,
    MSG_A2P_COMPLETE_OP,
    MSG_A2P_RETURNING,

} MSG_CODE;

typedef enum {
    OFF = 0,
    FORWARD,
    BACKWARD
} MOTOR_STATE;

// Settings
byte leftmotor_speed = 250;
byte rightmotor_speed = 225;

// Setup objects
SoftwareSerial serialCom = SoftwareSerial(rxPin, txPin);
Adafruit_VL53L0X lox_base = Adafruit_VL53L0X();


void setup(void) {
    // Setup pins
    pinMode(rxPin, INPUT);
    pinMode(txPin, OUTPUT);
    pinMode(leftmotor_pinA, OUTPUT);
    pinMode(leftmotor_pinB, OUTPUT);
    pinMode(rightmotor_pinA, OUTPUT);
    pinMode(rightmotor_pinB, OUTPUT);

    // Initialize serial communication at 9600 baud rate
    serialCom.begin(9600);

    // Initializing the sensor
    while (!lox.begin()) {
        delay(1000);
    }
}

// Variables
VL53L0X_RangingMeasurementData_t measure;   // To store sensor measurement data
STATE currentState = IDLE;                  // Current state of the robot operation
Operation currentOp;                        // Stores current operation data

byte verificationCount;     // Keep track of the number of times the sensor measured a pole
byte tiresRequired;         // Keep track of the remaining number of tires to deploy

void loop(void) {
    // Wait to receive a message from the PIC
    if (serialCom.available() > 0) {
        byte byteReceived = serialCom.read();

        switch (byteReceived) {
            // Receive stop signal, complete operation
            case MSG_P2A_STOP:
                setState(COMPLETE);
                break;

            case MSG_P2A_START:
            // Receive start signal, start operation
                currentOp = emptyOp;
                setState(DRIVING);
                break;

            case MSG_P2A_DEPLOYMENT_COMPLETE:
            // Receive stepper deployment complete signal, revert state to POLE_DETECTED
                setState(POLE_DETECTED);
                break;

            case MSG_P2A_DEBUG_DRIVE_FORWARD:
            // Receive signal to put motors into forward drive state
                driveMotors(FORWARD);
                break;

            case MSG_DEBUG_DRIVE_BACKWARD:
            // Receive signal to put motors into backward drive state
                driveMotors(BACKWARD);
                break;

            case MSG_DEBUG_STOP:
            // Receive signal to stop motors
                driveMotors(OFF);
                break;

            case MSG_P2A_DEBUG_SENSOR:
            // Receive signal to request sensor data
                delay(5);
                serialCom.write(readSensor());
                break;

            default:
                // Turn off motors and disable pole_detected_signal
                driveMotors(OFF);
                digitalWrite(pole_detected_signal, LOW);
                break;

            return;
        }
    }

    int distanceMeasured;   // Measured value of the sensor distance
    bool isSearching;       // Controls if the robot will be searching for poles

    switch (currentState) {
        case IDLE:
        // Do nothing
            break;

        case DRIVING:
            // If the limits of distance travelled or poles detected are reached, return to start
            if (currentOp.position >= MAX_DISTANCE_CM || currentOp.totalNumberOfPoles == MAX_POLES) {
                setState(RETURNING);
            } else {
                if (currentOp.totalNumberOfPoles == 0 || currentOp.position - currentOp.distanceOfPole[currentOp.totalNumberOfPoles - 1] > POLE_DETECTED_BUFFER_REGION) {
                    isSearching = true;
                } else {
                    isSearching = false;
                }
                
                if (isSearching) {
                    // Toggle the pole detected LED while searching
                    digitalWrite(pole_detected_signal, !digitalRead(pole_detected_signal));
                    distanceMeasured = readSensor();

                    if (distanceMeasured < POLE_DETECTED_RANGE) {
                        if (verificationCount == VALID_VERIFICATION_COUNT) {
                            // Pole was detected, update operation data
                            currentOp.totalNumberOfPoles++;
                            currentOp.tiresDeployedOnPole[currentOp.totalNumberOfPoles - 1] = 0;
                            // currentOp.tiresOnPoleAfterOperation[currentOp.totalNumberOfPoles - 1] = 0;
                            currentOp.distanceOfPole[currentOp.totalNumberOfPoles - 1] = currnetOp.position;
                            
                            // Decide whether the pole requires 1 or 2 tires
                            if ((currentOp.totalNumberOfPoles == 0) || currentOp.position - currentOp.distanceOfPole[currentOp.totalNumberOfPoles - 1] < 30) {
                                tiresRequired = 2;
                            } else {
                                tiresRequired = 1;
                            }

                            // Check if tires are currently on the pole
                            if 


                        }
                    }
                }
                    // distanceMeasured = readSensor();
                    currentOp.position++;
                    // if (distanceMeasured < POLE_DETECTED_RANGE) {
                        // if (verificationCount == VALID_VERIFICATION_COUNT) {
                            // Decide whether the pole requires 1 or 2 tires
                            if (currentOp.totalNumberOfPoles == 0 || currentOp.position - currentOp.distanceOfPole[currentOp.totalNumberOfPoles - 1] < 30) {
                                tiresRequired = 2;
                            } else {
                                tiresRequired = 1;
                            }

                            // Update current operation data
                            currentOp.totalNumberOfPoles++;
                            currentOp.tiresDeployedOnPole[currentOp.totalNumberOfPoles - 1] = 0;
                            currentOp.tiresOnPoleAfterOperation[currentOp.totalNumberOfPoles - 1] = 0;
                            currentOp.distanceOfPole[currentOp.totalNumberOfPoles - 1] = currentOp.position;

                            // Set the robot into POLE_DETECTED state
                            setState(POLE_DETECTED);

                            // Reset verification count
                            verificationCount = 0;

                        } else {
                            // Increment verificationCount if detecting object within range
                            verificationCount++;
                        }
                    } else {
                        // Reset verificationCount if no object detected within range
                        verificationCount = 0;
                    }
                // }
            }
            
            break;

        case POLE_DETECTED:
            if (tiresRequired > 0) {
                tiresRequired--;
                setState(DEPLOYING);
            } else {
                setState(DRIVING); 
                delay(500);
            }

            break;

        case DEPLOYING:
            // Deploy a tire (does nothing within loop, all actions are performed once)
            break;

        case RETURNING:
            if (currentOp.position == 0) {
                setState(COMPLETE);
            } else {
                currentOp.position--;
            }
            break;

        default:
            break;
    }

    delay(100);
   
}

byte readSensor(void) {
    lox.rangingTest(&measure, false);

    // if (measure.RangeStatus != 4 && measure.RangeMilliMeter < POLE_DETECTED_RANGE) {
    if (measure.RangeStatus != 4) {
        if (measure.RangeMilliMeter > 0xFF) {
            return 0xFF;
        } else {
            return measure.RangeMilliMeter;
        }
    }

    return 0xFF;
}

void driveMotors(MOTOR_STATE motorState) {
    switch (motorState) {
        case OFF:
            analogWrite(leftmotor_pinA, 0);
            analogWrite(leftmotor_pinB, 0);
            analogWrite(rightmotor_pinA, 0);
            analogWrite(rightmotor_pinB, 0);
            break;

        case FORWARD:
            analogWrite(leftmotor_pinA, leftmotor_speed);
            analogWrite(leftmotor_pinB, 0);
            analogWrite(rightmotor_pinA, rightmotor_speed);
            analogWrite(rightmotor_pinB, 0);
            
            break;

        case BACKWARD:
            analogWrite(leftmotor_pinA, 0);
            analogWrite(leftmotor_pinB, leftmotor_speed);
            analogWrite(rightmotor_pinA, 0);
            analogWrite(rightmotor_pinB, rightmotor_speed);
            break;

    }
}

void setState(STATE newState) {
    currentState = newState;
    driveMotors(OFF);

    switch (newState) {
        case IDLE:
            break;

        case DRIVING:
            serialCom.write(MSG_RECEIVE_DRIVING);
            driveMotors(FORWARD);
            verificationCount = 0;
            serialCom.write(
            break;

        case POLE_DETECTED:
            driveMotors(OFF);
            digitalWrite(pole_detected_signal, HIGH);
            break;

        case DEPLOYING:
            driveMotors(OFF);
            serialCom.write(MSG_RECEIVE_DEPLOY_STEPPER);
            digitalWrite(pole_detected_signal, HIGH);
            currentOp.totalSuppliedTires++;
            currentOp.tiresDeployedOnPole[currentOp.totalNumberOfPoles - 1]++;
            currentOp.tiresOnPoleAfterOperation[currentOp.totalNumberOfPoles - 1]++;
            break;

        case RETURNING:
            driveMotors(BACKWARD);
            serialCom.write(MSG_RECEIVE_RETURNING);
            break;

        case COMPLETE:
            driveMotors(OFF);
            digitalWrite(pole_detected_signal, LOW);
            serialCom.write(MSG_RECEIVE_COMPLETE_OP);
            currentOp = emptyOp;
            break;

        default:
            driveMotors(OFF);
            break;
    }
}

void initializeEncoder() {
    encoder_direction = true
    pinMode(encoder_master_pinB, INPUT);
    pinMode(encoder_slave_pinB, INPUT);
    attachInterrupt(digitalPinToInterrupt(encoder_master_pinB), modifyWheelSpeed, CHANGE);
    attachInterrupt(digitalPinToInerrupt(encoder_slave_pinB), modifyWheelSpeed, CHANGE);
}

void masterWheelSpeed() {
    byte lastState = 