#ifndef __TIMER_H
#define __TIMER_H

#include "stm32f10x.h"

// 计时状态枚举
typedef enum {
    TIMER_STOP = 0,  // 停止状态
    TIMER_RUNNING    // 运行状态
} TimerState;

// 计时数据结构体（存储毫秒、秒、分钟、小时）
typedef struct {
    uint32_t ms;     // 毫秒 (0-999)
    uint32_t sec;    // 秒 (0-59)
    uint32_t min;    // 分钟 (0-59)
    uint32_t hour;   // 小时 (0-...)
} TimerTime;

// 初始化计时模块（TIM2定时器，1ms中断一次）
void Timer_Init(void);

// 启动计时
void Timer_Start(void);

// 停止计时
void Timer_Stop(void);

// 重置计时（清零计时值）
void Timer_Reset(void);

// 获取当前计时状态（运行/停止）
TimerState Timer_GetState(void);

// 获取当前计时值（返回毫秒、秒、分钟、小时）
void Timer_GetTime(TimerTime *time);

// 获取总毫秒数（适合需要精确差值计算的场景）
uint64_t Timer_GetTotalMs(void);

#endif
