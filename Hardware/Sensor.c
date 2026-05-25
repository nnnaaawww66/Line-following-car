#include "stm32f10x.h"                  // Device header
#include "Sensor.h"

#define SENSOR_PORT

/**
  * @brief  光电传感器初始化
  * @param  无
  * @retval 无
  */
void Sensor_Init(){
	GPIO_InitTypeDef GPIO_InitStructure;
    
    // 使能GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    // 配置传感器引脚为输入模式
    GPIO_InitStructure.GPIO_Pin = SENSOR1_PIN | SENSOR2_PIN|SENSOR3_PIN | SENSOR4_PIN | SENSOR5_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  // 浮空输入
    // 或者使用上拉输入：GPIO_Mode_IPU，根据传感器输出特性选择
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;       // 输入模式速度可较低
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}



/**
  * @brief  读取指定传感器状态
  * @param  sensor_num: 传感器编号(1-5)
  * @retval 传感器状态: 0-未触发, 1-触发
  */
uint8_t Sensor_Read(uint8_t sensor_num)
{
    uint16_t sensor_pin;
    
    // 参数检查
    if(sensor_num < 1 || sensor_num > OPTICAL_SENSOR_NUM) {
        return 0;
    }
    
    // 根据传感器编号选择对应引脚
    switch(sensor_num) {
        case 1: sensor_pin = SENSOR1_PIN; break;
        case 2: sensor_pin = SENSOR2_PIN; break;
        case 3: sensor_pin = SENSOR3_PIN; break;
        case 4: sensor_pin = SENSOR4_PIN; break;
        case 5: sensor_pin = SENSOR5_PIN; break;
        default: return 0;
    }
    // 读取引脚状态
    // 根据传感器逻辑决定返回值：
    // - 反射式：有反射时输出高电平 → 直接返回引脚状态
    // - 对射式：有遮挡时输出低电平 → 返回引脚状态的取反
    if(GPIO_ReadInputDataBit(GPIOA, sensor_pin) == Bit_SET) {
        return 1;  // 传感器触发
    } else {
        return 0;  // 传感器未触发
    }
}

/**
  * @brief  传感器去初始化
  * @param  无
  * @retval 无
  */
void Sensor_Deinit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    // 将引脚配置为模拟输入以降低功耗
    GPIO_InitStructure.GPIO_Pin = SENSOR1_PIN | SENSOR2_PIN | SENSOR3_PIN | SENSOR4_PIN | SENSOR5_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;  // 模拟输入
    
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}
