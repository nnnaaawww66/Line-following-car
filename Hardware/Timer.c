#include "timer.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_rcc.h"



// 静态变量（模块内部使用）
static TimerState timer_state = TIMER_STOP;  // 计时状态
static uint64_t total_ms = 0;                // 总毫秒数（核心计时变量）

// 定时器2中断服务函数（1ms触发一次）
void TIM2_IRQHandler(void) {
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);  // 清除中断标志
		
        
        if (timer_state == TIMER_RUNNING) {
            total_ms++;  // 每毫秒+1
        }
    }
}

// 初始化TIM2定时器，配置为1ms中断
void Timer_Init(void) {
    // 1. 使能定时器和GPIO时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    
    // 2. 配置定时器基本参数
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_Period = 999;  // 自动重装载值（计数到1000触发中断）
    TIM_TimeBaseStructure.TIM_Prescaler = 71; // 预分频器（72MHz / (71+1) = 1MHz）
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;  // 时钟分频
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  // 向上计数
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    
    // 3. 使能定时器更新中断
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    
    // 4. 配置NVIC中断优先级
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  // 抢占优先级1
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;         // 子优先级1
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // 5. 启动定时器
    TIM_Cmd(TIM2, ENABLE);
	
}

// 启动计时
void Timer_Start(void) {
    timer_state = TIMER_RUNNING;
}

// 停止计时
void Timer_Stop(void) {
    timer_state = TIMER_STOP;
}

// 重置计时（清零）
void Timer_Reset(void) {
    uint8_t was_running = (timer_state == TIMER_RUNNING);
    Timer_Stop();       // 先停止计时
    total_ms = 0;       // 清零总毫秒数
    if (was_running) {
        Timer_Start();  // 如果之前在运行，重置后继续运行
    }
}

// 获取当前计时状态
TimerState Timer_GetState(void) {
    return timer_state;
}

// 获取当前计时值（转换为时分秒毫秒）
void Timer_GetTime(TimerTime *time) {
    if (time == 0) return;
    
    uint64_t ms = total_ms;  // 读取总毫秒数（避免中断中修改导致的误差）
    
    time->ms = ms % 1000;        // 毫秒：总毫秒数 % 1000
    uint32_t total_sec = ms / 1000;  // 总秒数
    
    time->sec = total_sec % 60;  // 秒：总秒数 % 60
    uint32_t total_min = total_sec / 60;  // 总分钟数
    
    time->min = total_min % 60;  // 分钟：总分钟数 % 60
    time->hour = total_min / 60; // 小时：总分钟数 / 60
}

// 获取总毫秒数（适合精确计算时间差）
uint64_t Timer_GetTotalMs(void) {
    return total_ms;

}
