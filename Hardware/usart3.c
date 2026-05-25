#include "usart3.h"               
#include "jy61p.h"
#include "OLED.h"
/**
 * @brief       串口3初始化函数（使用默认引脚 PB10(TX)/PB11(RX)，无重映射）
 * @param       bound: 波特率（如9600、115200等）
 * @retval      无
 * @note        1. USART3 默认引脚就是 PB10(TX)/PB11(RX)，无需重映射
 *              2. 仅需配置GPIO为复用功能，减少硬件/软件兼容性问题
 */
void usart3_Init(uint32_t bound)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure; 
    
    /* 1. 使能时钟：仅需 GPIOB（PB10/PB11） + USART3 时钟（无需AFIO，无重映射） */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);  // PB10/PB11 属于GPIOB
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE); // 使能USART3时钟
    
    /* 移除所有重映射代码！USART3默认引脚就是PB10/PB11，无需重映射 */
    
    /* 2. 配置 PB10 为 USART3_TX（复用推挽输出，默认引脚原生功能） */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;     // 复用推挽输出（TX引脚专用）
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);              // 端口为GPIOB
    
    /* 3. 配置 PB11 为 USART3_RX（浮空输入，默认引脚原生功能） */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 浮空输入（RX引脚专用）
    GPIO_Init(GPIOB, &GPIO_InitStructure);              // 端口为GPIOB
     
    /* 4. USART3 参数配置（保持原有逻辑，适配陀螺仪9600波特率） */
    USART_InitStructure.USART_BaudRate = bound;          
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART3, &USART_InitStructure); 
    
    /* 5. 开启接收中断 + 使能串口 */
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);  // 开启接收非空中断
    USART_Cmd(USART3, ENABLE);                      // 使能USART3
    
    /* 6. 中断配置（匹配主函数 NVIC_PriorityGroup_2，减少优先级冲突） */
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;  // 低于电机/编码器中断
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/**
 * @brief       USART3 接收中断服务函数（无修改，仅适配USART3）
 * @param       无
 * @retval      无
 */
void USART3_IRQHandler(void)
{
    uint8_t RxData,TxData;
    if (USART_GetITStatus(USART3, USART_IT_RXNE) == SET)
    {
        RxData = USART_ReceiveData(USART3);          // 读取串口3接收数据
        USART_SendData(USART3,RxData);
        jy61p_ReceiveData(RxData);                    // 转发给陀螺仪解析函数
        USART_ClearITPendingBit(USART3, USART_IT_RXNE); // 清除中断标志
    }
}
