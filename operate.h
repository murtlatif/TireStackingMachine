
#ifndef OPERATE_H
#define OPERATE_H

/********************************* Includes **********************************/
#include <xc.h>
#include "configureBits.h"

/********************************** Macros ***********************************/
#define POLE_DETECTED_RANGE 10
#define SAME_POLE_REGION 10

#define STEPPER_IN1 LATA
#define STEPPER_IN2
#define STEPPER_IN3
#define STEPPER_IN4
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
    STEPPER
} motor

/******************************   Variables **********************************/


/************************ Public Function Prototypes *************************/
void driveDCMotor(motor desiredMotor, motor_setting motorSetting);

void stepStepper(void);

unsigned char getNumberOfTiresOnPole(void);

unsigned char getNumberOfTiresRequiredForPole(void);

unsigned char readSensor(void);

#endif	/* OPERATE_H */