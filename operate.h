/**
 * operate.h
 * Author: Murtaza Latif
 */

#ifndef OPERATE_H
#define OPERATE_H

/********************************* Includes **********************************/
#include <xc.h>
#include "configureBits.h"

/********************************** Macros ***********************************/
#define CYCLES_FOR_ONE_REVOLUTION 200 
#define REVOLUTIONS_TO_DROP_ONE_TIRE 24

// Stepper pin assignments
#define STEPPER_EN LATEbits.LATE0
#define STEPPER_PULSE LATEbits.LATE1
#define STEPPER_DIR LATAbits.LATA4

#define BACKWARD 0  // Stepper motor direction
#define FORWARD 1   // Stepper motor direction

#define MOTOR_BACK1 LATCbits.LATC0
#define MOTOR_BACK2 LATCbits.LATC1

#define MOTOR_FRONT1 LATAbits.LATA1
#define MOTOR_FRONT2 LATAbits.LATA3

#define MOTOR_OFF 0
#define MOTOR_TOWARDS 1
#define MOTOR_AWAY 2
/******************************** Constants **********************************/

/********************************** Types ************************************/

/******************************   Variables **********************************/

/************************ Public Function Prototypes *************************/
void driveStepper(unsigned char revolutions, unsigned char dir);

void driveMotors(unsigned char state);

#endif	/* OPERATE_H */