#include "stm32f10x.h"

// 仅需2个全局变量
volatile uint8_t SPI_RxData;      // 最后接收的字节
volatile uint8_t SPI_DataReady = 0; // 数据到达标志

// 初始化函数（在现有SPI初始化后调用）
void SPI_RxInt_Init(void) {
    // 只需启用接收中断
    SPI_I2S_ITConfig(SPI1, SPI_I2S_IT_RXNE, ENABLE);
    
    // NVIC配置（优先级根据实际调整）
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = SPI1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

// 中断服务函数（放在stm32f10x_it.c）
void SPI1_IRQHandler(void) {
    if (SPI_I2S_GetITStatus(SPI1, SPI_I2S_IT_RXNE)) {
        SPI_RxData = SPI_I2S_ReceiveData(SPI1); // 读取数据（防溢出）
        SPI_DataReady = 1;                      // 设置标志位
        SPI_I2S_ClearITPendingBit(SPI1, SPI_I2S_IT_RXNE);
    }
}
