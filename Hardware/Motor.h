#ifndef __MOTOR_H
#define __MOTOR_H

#include "stm32f10x.h"                  // Device header
#include "PWM.h"

void Motor_Init(void);
void MotorL_Set_PWM(int8_t PWM);
void MotorR_Set_PWM(int8_t PWM);


#endif
