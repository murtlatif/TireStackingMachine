/**
 * operate.c
 * Author: Murtaza Latif
 */

/********************************* Includes **********************************/
#include "operate.h"

/******************************** Constants **********************************/

/***************************** Private Functions *****************************/

/***************************** Public Functions ******************************/
void driveStepper(unsigned char revolutions, unsigned char dir) {
    // Enable stepper motor and set direction
    STEPPER_EN = 1;
    STEPPER_DIR = dir;

    // Provide pulses to drive stepper
    STEPPER_PULSE = 0;
    for (int i = 0; i < (CYCLES_FOR_ONE_REVOLUTION * revolutions); i++) { // 200 cycles is one revolution (360 degrees)
        STEPPER_PULSE = 1;
        __delay_ms(1);
        STEPPER_PULSE = 0;
        __delay_ms(1);
    }

    // Disable stepper motor
    STEPPER_EN = 0;
    STEPPER_DIR = 0;
}

void driveMotors(unsigned char state) {
    switch (state) {
        case MOTOR_OFF:
            MOTOR_BACK1 = 0;
            MOTOR_BACK2 = 0;
            MOTOR_FRONT1 = 0;
            MOTOR_FRONT2 = 0;
            break;

        case MOTOR_TOWARDS:
            MOTOR_BACK1 = 1;
            MOTOR_BACK2 = 0;
            MOTOR_FRONT1 = 1;
            MOTOR_FRONT2 = 0;
            break;

        case MOTOR_AWAY:
            MOTOR_BACK1 = 0;
            MOTOR_BACK2 = 1;
            MOTOR_FRONT1 = 0;
            MOTOR_FRONT2 = 1;
            break;

        default:
            break;

    }
}