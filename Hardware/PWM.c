#include "stm32f10x.h"

// PWM配置宏定义，提高可配置性
#define PWM_TIM               TIM2
#define PWM_TIM_CLK           RCC_APB1Periph_TIM2
#define PWM_GPIO              GPIOA
#define PWM_GPIO_CLK          RCC_APB2Periph_GPIOA
#define PWM_PIN1              GPIO_Pin_0
#define PWM_PIN2              GPIO_Pin_1

#define PWM_ARR                100 - 1      // 自动重装载值
#define PWM_PSC               36 - 1       // 预分频值
#define PWM_FREQUENCY         20000        // PWM频率20kHz

/**
  * 函    数：PWM初始化
  * 参    数：无
  * 返 回 值：无
  */
void PWM_Init(void)
{
    /*开启时钟*/
    RCC_APB1PeriphClockCmd(PWM_TIM_CLK, ENABLE);
    RCC_APB2PeriphClockCmd(PWM_GPIO_CLK, ENABLE);
    
    /*GPIO初始化*/
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = PWM_PIN1 | PWM_PIN2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(PWM_GPIO, &GPIO_InitStructure);
    
    /*配置时钟源*/
    TIM_InternalClockConfig(PWM_TIM);
    
    /*时基单元初始化*/
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_Period = PWM_ARR;          // ARR值
    TIM_TimeBaseInitStructure.TIM_Prescaler = PWM_PSC;       // PSC值
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(PWM_TIM, &TIM_TimeBaseInitStructure);
    
    /*输出比较初始化 - 通道1*/
    TIM_OCInitTypeDef TIM_OCInitStructure;
    TIM_OCStructInit(&TIM_OCInitStructure);                  // 初始化为默认值
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;                       // 初始占空比为0
    
    TIM_OC1Init(PWM_TIM, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(PWM_TIM, TIM_OCPreload_Enable);
    
    /*输出比较初始化 - 通道2*/
    TIM_OCInitStructure.TIM_Pulse = 0;                       // 初始占空比为0
    TIM_OC2Init(PWM_TIM, &TIM_OCInitStructure);
    TIM_OC2PreloadConfig(PWM_TIM, TIM_OCPreload_Enable);
    
    /*使能定时器*/
    TIM_ARRPreloadConfig(PWM_TIM, ENABLE);
    TIM_Cmd(PWM_TIM, ENABLE);
}

/**
  * 函    数：设置通道1的PWM占空比
  * 参    数：Duty 占空比，范围：0~100
  * 返 回 值：无
  */
void PWM_SetCompare1(uint16_t Duty)
{
    // 限制占空比范围
    if (Duty > 100) 
    {
        Duty = 100;
    }
    
    // 计算实际的CCR值
    uint16_t CCR_Value = Duty * (PWM_ARR + 1) / 100;
    TIM_SetCompare1(PWM_TIM, CCR_Value);
}

/**
  * 函    数：设置通道2的PWM占空比
  * 参    数：Duty 占空比，范围：0~100
  * 返 回 值：无
  */
void PWM_SetCompare2(uint16_t Duty)
{
    // 限制占空比范围
    if (Duty > 100) 
    {
        Duty = 100;
    }
    
    // 计算实际的CCR值
    uint16_t CCR_Value = Duty * (PWM_ARR + 1) / 100;
    TIM_SetCompare2(PWM_TIM, CCR_Value);
}

/**
  * 函    数：同时设置两个通道的PWM占空比
  * 参    数：Duty1 通道1占空比，Duty2 通道2占空比
  * 返 回 值：无
  */
void PWM_SetCompare(uint16_t Duty1, uint16_t Duty2)
{
    PWM_SetCompare1(Duty1);
    PWM_SetCompare2(Duty2);
}

/**
  * 函    数：获取当前PWM频率
  * 参    数：无
  * 返 回 值：PWM频率(Hz)
  */
uint32_t PWM_GetFrequency(void)
{
    return SystemCoreClock / ( (PWM_PSC + 1) * (PWM_ARR + 1) );
}

/**
  * 函    数：获取最大占空比分辨率
  * 参    数：无
  * 返 回 值：分辨率(步进数)
  */
uint16_t PWM_GetResolution(void)
{
    return PWM_ARR + 1;
}