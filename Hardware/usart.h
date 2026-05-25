#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "stm32f10x.h"  // 替换为F1系列头文件

//#include "pid.h"
//////////////////////////////////////////////////////////////////////////////////	 
//USART1 头文件，适配STM32F103ZET6
//版本：V1.0
////////////////////////////////////////////////////////////////////////////////// 	
void USART1_Init(int bound);  // 声明中断服务函数（可选，增强规范性）
uint8_t Process_OpenMV_Data(void);
void USART1_IRQHandler(void);


#endif
