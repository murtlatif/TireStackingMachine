
#ifndef OPERATE_H
#define OPERATE_H

/********************************* Includes **********************************/
#include <xc.h>
#include "configureBits.h"
#include "logs.h"
/********************************** Macros ***********************************/
#define POLE_DETECTED_RANGE 10
#define SAME_POLE_REGION 10
#define STEPS_FOR_ONE_REVOLUTION 2048
#define SENSOR_VERIFICATION_TRIES 3

#define MOTOR1_IN1 LATCbits.LATC0
#define MOTOR1_IN2 LATCbits.LATC1

#define MOTOR2_IN1 LATAbits.LATA0
#define MOTOR2_IN2 LATAbits.LATA1

#define STEPPER_IN1 LATEbits.LATE0
#define STEPPER_IN2 LATEbits.LATE1
#define STEPPER_IN3 LATAbits.LATA4
#define STEPPER_IN4 LATAbits.LATA5

#define STEPPER_EN LATEbits.LATE0
#define STEPPER_PULSE LATEbits.LATE1
#define STEPPER_DIR LATAbits.LATA4
/******************************** Constants **********************************/


/********************************** Types ************************************/
/** @brief Settings for motors */
typedef enum {
	CLOCKWISE = 0,
    COUNTER_CLOCKWISE,
    OFF
} motor_setting;

typedef enum {
    MOTOR1 = 0,
    MOTOR2,
    BOTH,
    STEPPER
} motor;

/******************************   Variables **********************************/


/************************ Public Function Prototypes *************************/
void driveDCMotor(motor desiredMotor, motor_setting motorSetting);

void stepStepper(unsigned char step);

unsigned char getNumberOfTiresOnPole(void);

unsigned char getNumberOfTiresRequiredForPole(Operation op);

unsigned char readSensor(void);

void driveStepper(unsigned char revolutions, unsigned char dir);

#endif	/* OPERATE_H */