#ifndef __ENCODER_H
#define __ENCODER_H

#include "stm32f10x.h"

// 编码器初始化函数 - 配置PB6和PB7 (TIM4)
void EncoderL_Init(void);
// 编码器初始化函数 - 配置PB0和PB1 (TIM3)
void EncoderR_Init(void);
//以蜂鸣器朝向为正向
//左路编码器
int16_t EncoderL_Get_Speed(void);
//右路编码器
int16_t EncoderR_Get_Speed(void);

#endif
