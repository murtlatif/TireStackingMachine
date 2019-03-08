/**
 * @file
 * @author Murtaza Latif
 *
 * Created on January 15th, 2019, 5:21 PM
 * @ingroup CharacterLCD
 */

/********************************* Includes **********************************/
#include "operate.h"

/******************************** Constants **********************************/

/***************************** Private Functions *****************************/

/***************************** Public Functions ******************************/
void driveDCMotor(motor desiredMotor, motor_setting motorSetting) {
    if (desiredMotor == MOTOR1 || desiredMotor == MOTOR2) {
        unsigned char forwardInput;
        unsigned char backwardInput;

        switch (motorSetting) {
            case CLOCKWISE:
                forwardInput = 1;
                backwardInput = 0;
                break;

            case COUNTER_CLOCKWISE:
                forwardInput = 0;
                backwardInput = 1;
                break;

            case OFF:
                forwardInput = 0;
                backwardInput = 0;
                break;

            default:
                forwardInput = 0;
                backwardInput = 0;
                break;

        }

        switch (desiredMotor) {
            case MOTOR1:
                LATCbits.LATC0 = forwardInput;
                LATCbits.LATC1 = backwardInput;
                break;

            case MOTOR2:
                LATAbits.LATA0 = forwardInput;
                LATAbits.LATA1 = backwardInput;
                break;
                
            default:
                break;
        }
    }
}

void stepStepper(motor_setting motorSetting, unsigned char *step) {
    unsigned char step1 = 0;
    unsigned char step2 = 0;
    unsigned char step3 = 0;
    unsigned char step4 = 0;
    
    switch (motorSetting) {
        case CLOCKWISE:
            if ((*step) == 3) {
                (*step) = 0;
            } else {
                (*step) += 1;
            }
            break;
            
        case COUNTER_CLOCKWISE:
            if ((*step) == 0) {
                (*step) = 3;
            } else {
                (*step) -= 1;
            }
            
        default:
            return;
            break;
    }
    
    switch (*step) {
        case 0:
            step1 = 1;
            break;
            
        case 1:
            step2 = 1;
            break;
            
        case 2:
            step3 = 1;
            break;
            
        case 3:
            step4 = 1;
            break;
    }
    
    STEPPER_IN1 = step1;
    STEPPER_IN2 = step2;
    STEPPER_IN3 = step3;
    STEPPER_IN4 = step4;
    
}

unsigned char getNumberOfTiresOnPole(void) {
    return 0;
}

unsigned char getNumberOfTiresRequiredForPole(void) {
    return 2;
}

unsigned char readSensor(void) {
    return 0;
}