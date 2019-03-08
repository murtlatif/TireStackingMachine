
#ifndef OPERATE_H
#define OPERATE_H

/********************************* Includes **********************************/

/********************************** Macros ***********************************/
#define POLE_DETECTED_RANGE 10
#define SAME_POLE_REGION 10
/******************************** Constants **********************************/


/********************************** Types ************************************/

/************************ Public Function Prototypes *************************/
void driveMotor(void);

unsigned char readSensor(void);

#endif	/* OPERATE_H */