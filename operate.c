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

void stepStepper(unsigned char step) {
    unsigned char step1 = 0;
    unsigned char step2 = 0;
    unsigned char step3 = 0;
    unsigned char step4 = 0;
    
    switch (step) {
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
            
        default:
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

unsigned char getNumberOfTiresRequiredForPole(Operation op) {
    if (op.totalNumberOfPoles == 1) {
        return 2;
    }

    if ((op.distanceOfPole[op.totalNumberOfPoles - 1] - op.distanceOfPole[op.totalNumberOfPoles - 2]) < 30) {
        return 1;
    } 
    
    return 2;
}

unsigned char readSensor(void) {
    return 0;
}