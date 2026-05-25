#include "stm32f10x.h"
#include "PWM.h"

// 电机引脚定义，提高可维护性
#define MOTORL_IN1_PIN    GPIO_Pin_12
#define MOTORL_IN2_PIN    GPIO_Pin_13
#define MOTORR_IN1_PIN    GPIO_Pin_14
#define MOTORR_IN2_PIN    GPIO_Pin_15
#define MOTOR_GPIO        GPIOB

/**
  * 函    数：直流电机初始化
  * 参    数：无
  * 返 回 值：无
  */
void Motor_Init(void)
{
    /*开启时钟*/
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 配置电机控制引脚
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = MOTORL_IN1_PIN | MOTORL_IN2_PIN | 
                                 MOTORR_IN1_PIN | MOTORR_IN2_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(MOTOR_GPIO, &GPIO_InitStructure);
    
    // 初始化时设置电机为停止状态
    GPIO_ResetBits(MOTOR_GPIO, MOTORL_IN1_PIN | MOTORL_IN2_PIN | 
                              MOTORR_IN1_PIN | MOTORR_IN2_PIN);
    
    PWM_Init(); // 初始化PWM
}

/**
  * 函    数：直流电机设置PWM
  * 参    数：PWM 要设置的PWM值，范围：-100~100
  * 返 回 值：无
  */
void MotorL_Set_PWM(int8_t PWM)
{
    // 限制PWM范围在-100到100之间
    if (PWM > 60) PWM = 60;
    if (PWM < -60) PWM = -60;
    
    if (PWM > 0)                            // 正转
    {
        GPIO_ResetBits(MOTOR_GPIO, MOTORL_IN1_PIN);    // IN1低电平
        GPIO_SetBits(MOTOR_GPIO, MOTORL_IN2_PIN);      // IN2高电平
        PWM_SetCompare1(PWM);               // 设置PWM占空比
    }
    else if (PWM < 0)                       // 反转
    {
        GPIO_ResetBits(MOTOR_GPIO, MOTORL_IN2_PIN);    // IN2低电平
        GPIO_SetBits(MOTOR_GPIO, MOTORL_IN1_PIN);      // IN1高电平
        PWM_SetCompare1(-PWM);              // 设置PWM占空比（取正值）
    }
    else                                    // 停止
    {
        GPIO_ResetBits(MOTOR_GPIO, MOTORL_IN1_PIN | MOTORL_IN2_PIN); // 两个引脚都低电平
        PWM_SetCompare1(0);                 // PWM输出0
    }
}

/**
  * 函    数：右直流电机设置PWM
  * 参    数：PWM 要设置的PWM值，范围：-100~100
  * 返 回 值：无
  */
void MotorR_Set_PWM(int8_t PWM)
{
    // 限制PWM范围在-100到100之间
    if (PWM > 60) PWM = 60;
    if (PWM < -60) PWM = -60;
    
    if (PWM > 0)                            // 正转
    {
        GPIO_ResetBits(MOTOR_GPIO, MOTORR_IN1_PIN);    // IN1低电平
        GPIO_SetBits(MOTOR_GPIO, MOTORR_IN2_PIN);      // IN2高电平
        PWM_SetCompare2(PWM);               // 设置PWM占空比
    }
    else if (PWM < 0)                       // 反转
    {
        GPIO_ResetBits(MOTOR_GPIO, MOTORR_IN2_PIN);    // IN2低电平
        GPIO_SetBits(MOTOR_GPIO, MOTORR_IN1_PIN);      // IN1高电平
        PWM_SetCompare2(-PWM);              // 设置PWM占空比（取正值）
    }
    else                                    // 停止
    {
        GPIO_ResetBits(MOTOR_GPIO, MOTORR_IN1_PIN | MOTORR_IN2_PIN); // 两个引脚都低电平
        PWM_SetCompare2(0);                 // PWM输出0
    }
}

/**
  * 函    数：设置左右电机PWM
  * 参    数：PWM_L 左电机PWM，PWM_R 右电机PWM
  * 返 回 值：无
  */
void Motor_Set_PWM(int8_t PWM_L, int8_t PWM_R)
{
    MotorL_Set_PWM(PWM_L);
    MotorR_Set_PWM(PWM_R);
}

/**
  * 函    数：急停电机
  * 参    数：无
  * 返 回 值：无
  */
void Motor_Stop(void)
{
    // 刹车模式：两个引脚都置高
    GPIO_SetBits(MOTOR_GPIO, MOTORL_IN1_PIN | MOTORL_IN2_PIN | 
                            MOTORR_IN1_PIN | MOTORR_IN2_PIN);
    PWM_SetCompare1(0);
    PWM_SetCompare2(0);
}