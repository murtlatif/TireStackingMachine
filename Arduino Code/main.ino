#include <SoftwareSerial.h>
#include "Adafruit_VL53L0X.h"

// PIN definitions
    // UART
#define rxPin 1
#define txPin 0

    // ENCODER
#define encoder_master_pinA 4   //master encoder A pin -> the digital pin 3
#define encoder_master_pinB 2   //master encoder B pin -> the interrupt pin 0
#define encoder_slave_pinB 3    //B pin -> the interrupt pin 1
#define encoder_slave_pinA 7    //A pin -> the digital pin 5

    // MOTORS
#define leftmotor_pinA 5
#define leftmotor_pinB 6
#define rightmotor_pinA 9
#define rightmotor_pinB 10

    // SENSOR
#define XSHUT_sensor_base 11
#define XSHUT_sensor_tire1 12
#define XSHUT_sensor_tire2 13

    // OTHER
#define pole_detected_signal_pin 8

// Constants
#define POLE_DETECTED_RANGE 190
#define TIRE_DETECTED_RANGE 190
#define VALID_VERIFICATION_COUNT 2 // How many times the sensor should detect the pole
#define POLE_DETECTED_BUFFER_REGION 15 // How much distance it must travel before it can detect a new pole
#define MAX_POLES 10
#define MAX_DISTANCE_CM 400

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

// Initial Settings
byte leftmotor_speed = 250;
byte rightmotor_speed = 225;

// Setup objects
SoftwareSerial serialCom = SoftwareSerial(rxPin, txPin);
Adafruit_VL53L0X Sensor_Base = Adafruit_VL53L0X();

void setup(void) {
    // Setup pins
        // UART - Communication between PIC and Arduino
    pinMode(rxPin, INPUT);      
    pinMode(txPin, OUTPUT);

        // Motor - Motor Logic for primary DC motors
    pinMode(leftmotor_pinA, OUTPUT);
    pinMode(leftmotor_pinB, OUTPUT);
    pinMode(rightmotor_pinA, OUTPUT);
    pinMode(rightmotor_pinB, OUTPUT);

        // Encoder
    pinMode(encoder_master_pinA, INPUT);
    pinMode(encoder_master_pinB, INPUT);
    pinMode(encoder_slave_pinA, INPUT);
    pinMode(encoder_slave_pinB, INPUT);

        // Sensor
    pinMode(XSHUT_sensor_base, OUTPUT);
    pinMode(XSHUT_sensor_tire1, OUTPUT);
    pinMode(XSHUT_sensor_tire2, OUTPUT);

        // Other
    pinMode(pole_detected_signal_pin, OUTPUT);

    // Initialize serial communication at 9600 baud rate
    serialCom.begin(9600);

    // Initializing the sensor
    // while (!lox.begin()) {
    //     delay(1000);
    // }
}

// Variables
VL53L0X_RangingMeasurementData_t measure;   // To store sensor measurement data
STATE currentState = IDLE;                  // Current state of the robot operation
Operation currentOp;                        // Stores current operation data

byte verificationCount;     // Keep track of the number of times the sensor measured a pole
byte tiresRequired;         // Keep track of the remaining number of tires to deploy
bool isDriving;             // Keep track of whether robot is driving or not

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
                // Turn off motors and disable pole_detected_signal_pin
                driveMotors(OFF);
                digitalWrite(pole_detected_signal_pin, LOW);
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
                    digitalWrite(pole_detected_signal_pin, !digitalRead(pole_detected_signal_pin));
                    distanceMeasured = readSensor();

                    if (distanceMeasured < POLE_DETECTED_RANGE) {
                        if (verificationCount == VALID_VERIFICATION_COUNT) {
                            // If the pole was detected VALID_VERIFICATION_COUNT times

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
                            if ((readSensor(Sensor_Tire1) <= TIRE_DETECTED_RANGE) && readSensor(Sensor_Tire2) <= TIRE_DETECTED_RANGE) {
                                // 2 tires already on pole, do not need to deploy under any circumstance & record data in operation
                                currentOp.tiresOnPoleAfterOperation[currentOp.totalNumberOfPoles - 1] = 2;
                                tiresRequired = 0;
                            } else if (readSensor(Sensor_Tire1 <= TIRE_DETECTED_RANGE) {
                                // 1 tire already on pole, reduce the number of tires required by 1 & record data in operation
                                tiresRequired -= 1;
                                currentOp.tiresOnPoleAfterOperation[currentOp.totalNumberOfPoles - 1] = 1;
                            }

                            // Reset verification count
                            verificationCount = 0;

                            // Signal that a pole was detected by setting state to POLE_DETECTED
                            setState(POLE_DETECTED);

                        } else {
                            // If the pole was detected but not enough times

                            // Increment verification count and stop driving while verifying
                            verificationCount++;
                            if (isDriving) {
                                isDriving = false;
                                driveMotors(OFF);
                            }
                        }
                    } else {
                        // If no objects were sensed in the range, continue driving
                        verificationCount = 0;
                        if (!isDriving) {
                            isDriving = true;
                            driveMotors(FORWARD);
                        }
                    }

                    if (isDriving) {
                        // Update the position if driving
                        currentOp.position++;
                    }
                }
            }
            
            break;

        case POLE_DETECTED:
            if (tiresRequired > 0) {
                // Deploy a tire if one is required
                tiresRequired--;
                setState(DEPLOYING);
            } else {
                // Otherwise continue driving
                setState(DRIVING); 
                // delay(500); (HACKY WAY OF NOT DETECTING THE SAME POLE)
            }

            break;

        case DEPLOYING:
            // Deploy a tire (does nothing within loop, all actions are performed initially)
            break;

        case RETURNING:
            // Drive backwards until returned to start position
            if (currentOp.position == 0) {
                setState(COMPLETE);
            } else {
                currentOp.position--;
            }
            break;

        default:    
            break;
    }

    // Loop delay time
    delay(100);
   
}

void setState(STATE newState) {
    // Changes the state of the robot and performs an initial function upon state change
    currentState = newState;

    // Disable motors and pole detected signal by default
    driveMotors(OFF);
    digitalWrite(pole_detected_signal_pin, LOW);

    // Perform an initial function dependant on the new state
    switch (newState) {
        case IDLE:
            // Do nothing
            break;

        case DRIVING:
            // Tell PIC that robot is driving (to update LCD)
            serialCom.write(MSG_A2P_DRIVING);

            // Set motors to drive forward and reset verificationCount
            driveMotors(FORWARD);
            verificationCount = 0;
            break;

        case POLE_DETECTED:
            // Enable the pole detected signal
            digitalWrite(pole_detected_signal_pin, HIGH);
            break;

        case DEPLOYING:
            // Tell the pic to deploy the stepper
            serialCom.write(MSG_A2P_DEPLOY_STEPPER);
            // Keep the pole detected signal enabled
            digitalWrite(pole_detected_signal_pin, HIGH);

            // Update current operation data
            currentOp.totalSuppliedTires++;
            currentOp.tiresDeployedOnPole[currentOp.totalNumberOfPoles - 1]++;
            currentOp.tiresOnPoleAfterOperation[currentOp.totalNumberOfPoles - 1]++;

            break;

        case RETURNING:
            // Tell PIC that robot is returning (to update LCD)
            serialCom.write(MSG_RECEIVE_RETURNING);

            // Drive motors in reverse
            driveMotors(BACKWARD);
            break;

        case COMPLETE:
            // Tell PIC that the robot completed the operation
            serialCom.write(MSG_RECEIVE_COMPLETE_OP);

            // Send operation data to pic!! (TO DO)
            // for now just reset operation
            currentOp = emptyOp;
            break;

        default:
            break;
    }
}

byte readSensor(Adafruit_VL53L0X sensor) {
    sensor.rangingTest(&measure, false);

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
    byte lastState;
}