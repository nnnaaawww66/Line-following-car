#include "stm32f10x.h"

/**
 * @brief 编码器GPIO和定时器初始化
 * @param TIMx 定时器指针(TIM3/TIM4)
 * @param GPIOx GPIO端口指针
 * @param GPIO_Pin1 第一个编码器引脚
 * @param GPIO_Pin2 第二个编码器引脚
 * @param AFIO_Remap 重映射配置
 */
static void Encoder_Init(TIM_TypeDef* TIMx, GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin1, uint16_t GPIO_Pin2, uint32_t AFIO_Remap)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_ICInitTypeDef TIM_ICInitStructure;
    
    // 使能GPIO和定时器时钟
    if (TIMx == TIM3) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
        // 配置重映射
        if (AFIO_Remap != 0) {
            GPIO_PinRemapConfig(AFIO_Remap, ENABLE);
            // 禁用JTAG以释放PB4
            GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
        }
    } else if (TIMx == TIM4) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    }
    
    // 配置GPIO为浮空输入 - 修正：应该使用上拉输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin1 | GPIO_Pin2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 上拉输入，稳定信号电平
    GPIO_Init(GPIOx, &GPIO_InitStructure);
    
    // 时基配置
    TIM_TimeBaseStructure.TIM_Period = 0xFFFF;          // 16位计数器最大值
    TIM_TimeBaseStructure.TIM_Prescaler = 0;            // 不分频
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;        // 时钟分频
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIMx, &TIM_TimeBaseStructure);
    
    // 输入捕获配置 - 修正：移除重复配置
    TIM_ICStructInit(&TIM_ICInitStructure); // 先初始化为默认值
    TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
    TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising; // 编码器模式会自动处理边沿
    TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    TIM_ICInitStructure.TIM_ICFilter = 0x0F;  // 滤波设置，根据编码器特性调整
    TIM_ICInit(TIMx, &TIM_ICInitStructure);
    
    TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
    TIM_ICInit(TIMx, &TIM_ICInitStructure);
    
    // 编码器接口配置 - 模式3：TI1和TI2都计数，边沿都检测
    TIM_EncoderInterfaceConfig(TIMx, TIM_EncoderMode_TI12,
                              TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
    
    // 清除计数器并使能定时器
    TIM_SetCounter(TIMx, 0);
    TIM_Cmd(TIMx, ENABLE);
}

// 左编码器初始化函数 - 修正引脚定义
void EncoderL_Init(void)
{
    // TIM4通道1和2对应PB6和PB7
    Encoder_Init(TIM4, GPIOB, GPIO_Pin_6, GPIO_Pin_7, 0);
}

// 右编码器初始化函数 - 修正引脚定义
void EncoderR_Init(void)
{
    // TIM3通道1和2对应PA6和PA7，或者重映射到PC6和PC7
    // 根据注释应该是PB4和PB5，使用部分重映射
    Encoder_Init(TIM3, GPIOB, GPIO_Pin_4, GPIO_Pin_5, GPIO_PartialRemap_TIM3);
}

// 获取编码器计数值（带方向处理）
int16_t EncoderL_Get_Speed(void)
{
    int16_t temp = (int16_t)TIM_GetCounter(TIM4);
    TIM_SetCounter(TIM4, 0);
    return -temp; // 根据实际安装方向调整符号
}

int16_t EncoderR_Get_Speed(void)
{
    int16_t temp = (int16_t)TIM_GetCounter(TIM3);
    TIM_SetCounter(TIM3, 0);
    return -temp; // 根据实际安装方向调整符号
}

/**
 * @brief 获取编码器累计计数值（不重置）
 */
int32_t EncoderL_Get_Count(void)
{
    return (int16_t)TIM_GetCounter(TIM4);
}

int32_t EncoderR_Get_Count(void)
{
    return (int16_t)TIM_GetCounter(TIM3);
}

/**
 * @brief 重置编码器计数器
 */
void EncoderL_Reset(void)
{
    TIM_SetCounter(TIM4, 0);
}

void EncoderR_Reset(void)
{
    TIM_SetCounter(TIM3, 0);
}