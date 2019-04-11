#include <SoftwareSerial.h>
//#include "Adafruit_VL53L0X.h"
#include "Wire.h"
#include "VL53L0X.h"

// Macros
#define serialWriteSuccessOrFail(result) {\
    result ? serialCom.write(MSG_A2P_SUCCESS) : serialCom.write(MSG_A2P_FAILED);\
}

// PIN definitions
    // UART
#define rxPin 1
#define txPin 0

    // ENCODER
// #define encoder_master_pinA 4   //master encoder A pin -> the digital pin 3
// #define encoder_master_pinB 2   //master encoder B pin -> the interrupt pin 0
// #define encoder_slave_pinB 3    //B pin -> the interrupt pin 1
// #define encoder_slave_pinA 7    //A pin -> the digital pin 5

    // MOTORS
#define leftmotor_pinA 5
#define leftmotor_pinB 6
#define rightmotor_pinA 9
#define rightmotor_pinB 11

    // SENSOR
// #define XSHUT_sensor_base 2
// #define XSHUT_sensor_tire1 3
// #define XSHUT_sensor_tire2 4

    // OTHER
#define pole_detected_signal_pin 8

// I2C Addresses
#define Sensor_Base_Addr 0x29     // Default address of the sensor is 0x29 (41)
#define Sensor_Tire1_Addr 0x30
#define Sensor_Tire2_Addr 0x31

// Constants
#define MOTORSTATE_OFF 0
#define MOTORSTATE_FORWARD 1
#define MOTORSTATE_BACKWARD 2

#define PS_0T 0
#define PS_1T 1
#define PS_2T 2
#define PS_None 3

#define POLE_DETECTED_RANGE 240         // Maximium distance a pole can be detected
#define TIRE_DETECTED_RANGE 210         // Maximum distance a tire can be detected
#define VALID_VERIFICATION_COUNT 1     // How many times the sensor should detect the pole
#define POLE_DETECTED_BUFFER_REGION 100 // How much distance it must travel before it can detect a new pole
#define FIRST_POLE_MIN_DISTANCE_MM 100  // The minimum distance the first pole will be found 
#define MAX_POLES 10                    // Maximum number of poles in the operation
#define MAX_DISTANCE_MM 4000            // Maximum distance the robot should travel for the operation

#define OPTIMAL_MIN_RANGE 115
#define OPTIMAL_MAX_RANGE 119
#define DRIVESPEED_MM_PER_SECOND 190
// Structs
typedef struct {
     byte totalSuppliedTires;
     byte totalNumberOfPoles;
     byte tiresDeployedOnPole[10];
     byte tiresOnPoleAfterOperation[10];
     unsigned short distanceOfPole[10];

     // Unstored (temporary) information
     // 135 mm/s
     byte tiresRemaining;
     unsigned short position_in_mm;
} Operation;

const Operation emptyOp = { .totalSuppliedTires = 0,
                            .totalNumberOfPoles = 0,
                            .tiresDeployedOnPole = {},
                            .tiresOnPoleAfterOperation = {},
                            .distanceOfPole = {},
                            .tiresRemaining = 5,
                            .position_in_mm = 0
                            };

// Types
typedef enum {
    IDLE = 0,
    DRIVING,
    DEPLOYING,
    POLE_DETECTED,
    ADJUSTING,
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
    MSG_P2A_DEBUG_SENSOR_BASE,
    MSG_P2A_DEBUG_SENSOR_TIRE1,
    MSG_P2A_DEBUG_SENSOR_TIRE2,
    MSG_P2A_OP_DEBUG,
    MSG_P2A_REQUEST_POSITION,
    MSG_P2A_REQUEST_TIRES_FOUND,
    MSG_P2A_REQUEST_TIRES_REMAINING,
    MSG_P2A_REQUEST_INITIALIZE_SENSOR,
    MSG_P2A_REQUEST_STATUS_SENSORS,
    MSG_P2A_ADJUSTMENT_COMPLETE,
    
    // Arduino to PIC Messages
    MSG_A2P_SUCCESS = 100,
    MSG_A2P_FAILED,
    MSG_A2P_DRIVING,
    MSG_A2P_RETURNING,
    MSG_A2P_POLE_DETECTED,
    MSG_A2P_DEPLOY_STEPPER,
    MSG_A2P_COMPLETE_OP,
    MSG_A2P_SENSOR_TIMEOUT,
    MSG_A2P_ADJUST_TOWARDS,
    MSG_A2P_ADJUST_AWAY,

} MSG_CODE;


// Initial Settings
// byte leftmotor_speed = 59;
// byte rightmotor_speed = 70;
byte leftmotor_speed = 72;
byte rightmotor_speed = 81;
bool debugMode = false;

// Setup objects
SoftwareSerial serialCom = SoftwareSerial(rxPin, txPin);
VL53L0X Sensor_Base;
VL53L0X Sensor_Tire1;
VL53L0X Sensor_Tire2;

void setup(void) {
    // Setup pins
        // UART - Communication between PIC and Arduino
    pinMode(rxPin, INPUT);      
    pinMode(txPin, OUTPUT);

        // Encoder
    // pinMode(encoder_master_pinA, INPUT);
    // pinMode(encoder_master_pinB, INPUT);
    // pinMode(encoder_slave_pinA, INPUT);
    // pinMode(encoder_slave_pinB, INPUT);

        // Sensor
    // pinMode(XSHUT_sensor_base, OUTPUT);
    // if (!debugMode) {
        // pinMode(XSHUT_sensor_tire1, OUTPUT);
        // pinMode(XSHUT_sensor_tire2, OUTPUT);
    // }

        // Other
    pinMode(pole_detected_signal_pin, OUTPUT);

    // Initialize serial communication at 9600 baud rate
    serialCom.begin(9600);

    Wire.begin();

    // Progressive LED flash
    digitalWrite(pole_detected_signal_pin, HIGH);
    delay(1000);
    digitalWrite(pole_detected_signal_pin, LOW);

    delay(10);
    // Initialize Sensors
    debugMode = false;

    // // Restart sensors
    // if (debugMode) {
    //     // Change the address of the enabled sensor (Sensor_Tire2)
    //     pinMode(XSHUT_sensor_tire2, INPUT);
    //     delay(10);
    //     Sensor_Tire2.setAddress(Sensor_Tire2_Addr);
    //     delay(10);
    //     // Enable the next sensor
    //     pinMode(XSHUT_sensor_tire1, INPUT);
    //     delay(10);

    //     // Change the address of the enabled sensor (Sensor_Tire1)
    //     Sensor_Tire1.setAddress(Sensor_Tire1_Addr);
    //     delay(10);
    // }

    // // Enable the base sensor
    // delay(100);
    // pinMode(XSHUT_sensor_base, OUTPUT);
    // delay(10);
    // pinMode(XSHUT_sensor_base, INPUT);
    // delay(10);

    // Initialize sensors
    Sensor_Base.init();

    // if (debugMode) {
    //     Sensor_Tire1.init();
    //     Sensor_Tire2.init();
    // }

    // Set timeouts for sensors
    Sensor_Base.setTimeout(500);

    // if (debugMode) {
    //     Sensor_Tire1.setTimeout(1000);
    //     Sensor_Tire2.setTimeout(1000);
    // }

    // Start the sensor continuously
    Sensor_Base.startContinuous(10);
    delay(50);

    // if (debugMode) {
    //     Sensor_Tire1.startContinuous(10);
    //     Sensor_Tire2.startContinuous(10);
    // }

    // Initialization complete, signal to the LED
    digitalWrite(pole_detected_signal_pin, HIGH);
    delay(100);
    digitalWrite(pole_detected_signal_pin, LOW);

    // Motor - Motor Logic for primary DC motors
    pinMode(leftmotor_pinA, OUTPUT);
    pinMode(leftmotor_pinB, OUTPUT);
    pinMode(rightmotor_pinA, OUTPUT);
    pinMode(rightmotor_pinB, OUTPUT);

    randomSeed(analogRead(0));
}

// Variables
// VL53L0X_RangingMeasurementData_t measure;   // To store sensor measurement data
byte currentState = IDLE;                  // Current state of the robot operation
Operation currentOp;                        // Stores current operation data

byte verificationCount;     // Keep track of the number of times the sensor measured a pole
byte tiresRequired;         // Keep track of the remaining number of tires to deploy
bool isDriving;             // Keep track of whether robot is driving or not
bool dbg;
int tick = 0;
unsigned short measuredAdjustmentDistance;

byte pstates[10];

void setState(byte newState);

void loop(void) {

    // Check for available messages from the PIC
    if (serialCom.available() > 0) {
        byte byteReceived = serialCom.read();

        switch (byteReceived) {
            // Receive stop signal, complete operation
            case MSG_P2A_STOP:
                setState(COMPLETE);
                break;

            case MSG_P2A_START:
            // Receive start signal, start operation
              // Reset current operations
                currentOp.totalNumberOfPoles = 0;
                currentOp.totalSuppliedTires = 0;
                currentOp.position_in_mm = 0;
                currentOp.tiresRemaining = 5;

                setState(DRIVING);
                break;

            case MSG_P2A_DEPLOYMENT_COMPLETE:
            // Receive stepper deployment complete signal, revert state to POLE_DETECTED
                setState(POLE_DETECTED);
                break;

            case MSG_P2A_DEBUG_DRIVE_FORWARD:
            // Receive signal to put motors into forward drive state
                driveMotors(MOTORSTATE_FORWARD);
                break;

            case MSG_P2A_DEBUG_DRIVE_BACKWARD:
            // Receive signal to put motors into backward drive state
                driveMotors(MOTORSTATE_BACKWARD);
                break;

            case MSG_P2A_DEBUG_STOP:
            // Receive signal to stop motors
                driveMotors(MOTORSTATE_OFF);
                break;

            case MSG_P2A_DEBUG_SENSOR_BASE:
            // Receive signal to request sensor_base data
                // Send data for Sensor_Base
                serialCom.write((byte)(readSensor(Sensor_Base) >> 8));
                serialCom.write((byte)(readSensor(Sensor_Base) & 0xFF));

            case MSG_P2A_DEBUG_SENSOR_TIRE1:
            // Receive signal to request sensor_tire1 data
                delay(10);
                // // Send data for Sensor_Tire1
                // serialCom.write((byte)(readSensor(Sensor_Tire1) >> 8));
                // serialCom.write((byte)(readSensor(Sensor_Tire1) & 0xFF));

            case MSG_P2A_DEBUG_SENSOR_TIRE2:
            // Receive signal to request sensor_tire2 data
                delay(10);
                // Send data for Sensor_Tire2
                // serialCom.write((byte)(readSensor(Sensor_Tire2) >> 8));
                // serialCom.write((byte)(readSensor(Sensor_Tire2) & 0xFF));
                break;

            case MSG_P2A_REQUEST_INITIALIZE_SENSOR:
            // Return a success or fail depending on whether the sensor is initialized
                serialWriteSuccessOrFail(Sensor_Base.getAddress() == Sensor_Base_Addr);
                break;

            case MSG_P2A_REQUEST_STATUS_SENSORS:
                serialWriteSuccessOrFail(Sensor_Base.getAddress() == Sensor_Base_Addr);
                // if (debugMode) {
                //     serialWriteSuccessOrFail(Sensor_Tire1.getAddress() == Sensor_Tire1_Addr);
                //     serialWriteSuccessOrFail(Sensor_Tire2.getAddress() == Sensor_Tire2_Addr);
                // }
                
                break;

            case MSG_P2A_REQUEST_POSITION:
                serialCom.write((byte)((currentOp.position_in_mm) >> 8));
                serialCom.write((byte)(currentOp.position_in_mm & 0xFF));
                break;

            case MSG_P2A_REQUEST_TIRES_FOUND:
                serialCom.write(decryptPState(pstates[currentOp.totalNumberOfPoles - 1]));
                break;

            case MSG_P2A_REQUEST_TIRES_REMAINING:
                serialCom.write(currentOp.tiresRemaining);
                break;

            case MSG_P2A_OP_DEBUG:
                for (byte i = 0; i < 10; i++) {
                    while (!serialCom.available()) {continue;}
                    pstates[i] = serialCom.read();
                    dbg = true;
                }
                break;

            case MSG_P2A_ADJUSTMENT_COMPLETE:
                setState(ADJUSTING);
                break;

            default:
                // Turn off motors and disable pole_detected_signal_pin
                driveMotors(MOTORSTATE_OFF);
                digitalWrite(pole_detected_signal_pin, LOW);
                break;

            return;
        }
    }

    // Operation variables
    unsigned short distanceMeasured;   // Measured value of the sensor distance
    bool isSearching;       // Controls if the robot will be searching for poles
    // Operate depending on the state of the robot
    switch (currentState) {

        case IDLE:
        // Do nothing
            break;

        case DRIVING:
            
            // If the limits of distance travelled or poles detected are reached, return to start
            if (currentOp.position_in_mm >= MAX_DISTANCE_MM || currentOp.totalNumberOfPoles == MAX_POLES) {
                setState(RETURNING);
                break;
            }
            
            // Do not search until minimum distance for first pole is reached. Also do not search if recently found a pole.
            if ((currentOp.position_in_mm > FIRST_POLE_MIN_DISTANCE_MM) && 
                (currentOp.totalNumberOfPoles == 0 || currentOp.position_in_mm - currentOp.distanceOfPole[currentOp.totalNumberOfPoles - 1] > POLE_DETECTED_BUFFER_REGION)) {
                
                isSearching = true;
            } else {
                isSearching = false;
                digitalWrite(pole_detected_signal_pin, HIGH);
            }
            
            // Check for poles while searching
            if (isSearching) {

                // Measure the distance seen by the sensor
                distanceMeasured = readSensor(Sensor_Base);

                // Check if the distance measured is within the range of the maximum pole distance
                if (distanceMeasured < POLE_DETECTED_RANGE) {
                    if (verificationCount >= VALID_VERIFICATION_COUNT) {
                        // If the pole was detected VALID_VERIFICATION_COUNT times

                        // Pole was detected, update operation data
                        currentOp.totalNumberOfPoles++;
                        currentOp.tiresDeployedOnPole[currentOp.totalNumberOfPoles - 1] = 0;
                        currentOp.tiresOnPoleAfterOperation[currentOp.totalNumberOfPoles - 1] = random(0, 2);
                        currentOp.distanceOfPole[currentOp.totalNumberOfPoles - 1] = currentOp.position_in_mm;
                        if (dbg) {
                            currentOp.tiresOnPoleAfterOperation[currentOp.totalNumberOfPoles - 1] = pstates[currentOp.totalNumberOfPoles - 1];
                        }
                        
                        // Decide whether the pole requires 1 or 2 tires
                        // The first pole and every pole that is further than 30cm from the previous pole requires 2 tires
                        if ((currentOp.totalNumberOfPoles == 1) || currentOp.position_in_mm - currentOp.distanceOfPole[currentOp.totalNumberOfPoles - 1] >= 300) {
                            tiresRequired = 2;
                        } else {
                            tiresRequired = 1;
                        }

                        tiresRequired -= currentOp.tiresOnPoleAfterOperation[currentOp.totalNumberOfPoles - 1];


                        // // Check if tires are currently on the pole
                        // if ((readSensor(Sensor_Tire1) <= TIRE_DETECTED_RANGE) && readSensor(Sensor_Tire2) <= TIRE_DETECTED_RANGE) {
                        //     // 2 tires already on pole, do not need to deploy under any circumstance & record data in operation
                        //     currentOp.tiresOnPoleAfterOperation[currentOp.totalNumberOfPoles - 1] = 2;
                        //     tiresRequired = 0;
                        // } else if (readSensor(Sensor_Tire1) <= TIRE_DETECTED_RANGE) {
                        //     // 1 tire already on pole, reduce the number of tires required by 1 & record data in operation
                        //     tiresRequired -= 1;
                        //     currentOp.tiresOnPoleAfterOperation[currentOp.totalNumberOfPoles - 1] = 1;
                        // }

                        // Reset verification count
                        verificationCount = 0;

                        // Signal that a pole was detected by setting state to POLE_DETECTED
                        setState(POLE_DETECTED);

                    } else {
                        // If the pole was detected but not enough times

                        // Increment verification count and stop driving while verifying
                        verificationCount++;
                        // if (isDriving) {
                        //     isDriving = false;
                        //     driveMotors(MOTORSTATE_OFF);
                        // }
                    }
                } else {
                    // If no objects were sensed in the range, continue driving
                    verificationCount = 0;
                    // if (!isDriving) {
                    //     isDriving = true;
                    //     driveMotors(MOTORSTATE_FORWARD);
                    // }
                    currentOp.position_in_mm += DRIVESPEED_MM_PER_SECOND / 10;
                }

            } else {
                currentOp.position_in_mm += DRIVESPEED_MM_PER_SECOND / 10;
            }

            break;

        case POLE_DETECTED:
            // Check if can deploy
            if (tiresRequired > 0 && currentOp.tiresRemaining > 0) {
                delay(100);
                setState(ADJUSTING);
            } else {
                // Otherwise continue driving
                setState(DRIVING);
                // delay(500); (HACKY WAY OF NOT DETECTING THE SAME POLE)
            }
            
            break;

        case ADJUSTING:
            break;

        case DEPLOYING:
            // Deploy a tire (does nothing within loop, all actions are performed initially)
            delay(100);
            break;

        case RETURNING:
            // Drive backwards until returned to start position
            if (currentOp.position_in_mm == 0) {
                setState(COMPLETE);
            } else {
                currentOp.position_in_mm-= DRIVESPEED_MM_PER_SECOND / 10;
            }
            break;

        default:    
            break;
    }

    // Loop delay time
    delay(100);
    tick++;
    if (tick == 1000) {
        tick = 0;
    }
   
}

void setState(byte newState) {
    // Changes the state of the robot and performs an initial function upon state change
    currentState = newState;

    // Disable motors and pole detected signal by default
    driveMotors(MOTORSTATE_OFF);
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
            driveMotors(MOTORSTATE_FORWARD);
            verificationCount = 0;
            break;

        case POLE_DETECTED:
            // Tell PIC that a pole was detected (to update LCD)
            serialCom.write(MSG_A2P_POLE_DETECTED);

            // Enable the pole detected signal
            digitalWrite(pole_detected_signal_pin, HIGH);
            break;

        case ADJUSTING:
            measuredAdjustmentDistance = takeAverageSensorReading(Sensor_Base, 5);
            if (measuredAdjustmentDistance > OPTIMAL_MAX_RANGE) {
                serialCom.write(MSG_A2P_ADJUST_TOWARDS);
            } else if (measuredAdjustmentDistance < OPTIMAL_MIN_RANGE) {
                serialCom.write(MSG_A2P_ADJUST_AWAY);
            } else {
                setState(DEPLOYING);
            }
            break;

        case DEPLOYING:
            // Tell the pic to deploy the stepper
            serialCom.write(MSG_A2P_DEPLOY_STEPPER);
            // Keep the pole detected signal enabled
            digitalWrite(pole_detected_signal_pin, HIGH);

            tiresRequired--;
            // Update current operation data
            currentOp.totalSuppliedTires++;
            currentOp.tiresDeployedOnPole[currentOp.totalNumberOfPoles - 1]++;
            currentOp.tiresOnPoleAfterOperation[currentOp.totalNumberOfPoles - 1]++;
            currentOp.tiresRemaining--;
            break;

        case RETURNING:
            // Tell PIC that robot is returning (to update LCD)
            serialCom.write(MSG_A2P_RETURNING);

            // Drive motors in reverse
            driveMotors(MOTORSTATE_BACKWARD);

            currentOp.position_in_mm += 500;
            break;

        case COMPLETE:
            // Tell PIC that the robot completed the operation
            serialCom.write(MSG_A2P_COMPLETE_OP);

            serialCom.write(currentOp.totalSuppliedTires);
            serialCom.write(currentOp.totalNumberOfPoles);
            for (byte i = 0; i < 10; i++) {
                serialCom.write(currentOp.tiresDeployedOnPole[i]);
            }
            for (byte i = 0; i < 10; i++) {
                serialCom.write(currentOp.tiresOnPoleAfterOperation[i]);
            }
            for (byte i = 0; i < 10; i++) {
                serialCom.write(currentOp.distanceOfPole[i] >> 8);
                serialCom.write(currentOp.distanceOfPole[i] & 0xFF);
            }
            // Send operation data to pic!! (TO DO)
            // for now just reset operation
            currentOp.totalNumberOfPoles = 0;
            currentOp.totalSuppliedTires = 0;
            currentOp.position_in_mm = 0;
            break;

        default:
            break;
    }
}

// Read the value of the specified sensor
unsigned short readSensor(VL53L0X sensor) {
    // Read the sensor
    unsigned int measuredVal = sensor.readRangeContinuousMillimeters();
    if (sensor.timeoutOccurred()) {
        serialCom.write(MSG_A2P_SENSOR_TIMEOUT);
    }

    if (measuredVal > 999) {
        return (unsigned short)999;
    } else {
        return (unsigned short)measuredVal;
    }
}

unsigned short takeAverageSensorReading(VL53L0X sensor, byte numberOfReadings) {
    // Keep track of the total readings
    unsigned long summedReadings = 0;

    // Add each reading to the total
    for (byte i = 0; i < numberOfReadings; i++) {
        summedReadings += readSensor(sensor);
        // Provide a small delay for better data
        delay(1);
    }

    // Take the average by dividing by the number of readings
    unsigned int averageReading = summedReadings / numberOfReadings;

    // If the average reading is more than 0xFF, return 0xFF otherwise return average value
    if (averageReading > 999) {
        return 999;
    } else {
        return (unsigned short)averageReading;
    }
}

void driveMotors(byte motorState) {
  if (motorState == MOTORSTATE_FORWARD) {
    analogWrite(leftmotor_pinA, leftmotor_speed);
    analogWrite(leftmotor_pinB, 0);
    analogWrite(rightmotor_pinA, rightmotor_speed);
    analogWrite(rightmotor_pinB, 0);
  } else if (motorState == MOTORSTATE_BACKWARD) {
    analogWrite(leftmotor_pinA, 0);
    analogWrite(leftmotor_pinB, leftmotor_speed);
    analogWrite(rightmotor_pinA, 0);
    analogWrite(rightmotor_pinB, rightmotor_speed);
  } else {
    analogWrite(leftmotor_pinA, 0);
    analogWrite(leftmotor_pinB, 0);
    analogWrite(rightmotor_pinA, 0);
    analogWrite(rightmotor_pinB, 0);
  }
}

byte decryptPState(byte pstate) {
    if (pstate == PS_0T || pstate == PS_1T || pstate == PS_2T) {
        return pstate;
    } else if (pstate == PS_None) {
        return 0;
    } else {
        return 0;
    }
}

// void initializeEncoder(void) {
//     pinMode(encoder_master_pinB, INPUT);
//     pinMode(encoder_slave_pinB, INPUT);
//     attachInterrupt(digitalPinToInterrupt(encoder_master_pinB), masterWheelSpeed, CHANGE);
//     attachInterrupt(digitalPinToInterrupt(encoder_slave_pinB), slaveWheelSpeed, CHANGE);
// }

// void masterWheelSpeed(void) {
//     driveMotors(OFF);
// }

// void slaveWheelSpeed(void) {
//     driveMotors(OFF);
// }
