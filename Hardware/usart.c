#include <string.h>
#include "stm32f10x.h"


#define RX_BUFFER_SIZE 64
#define MAX_NUMBER_LENGTH 3

// 全局变量
char uart_rx_buffer[RX_BUFFER_SIZE];
uint8_t rx_index = 0;
uint8_t openmv_data_ready = 0;
uint8_t detected_number = 0;
uint8_t actual_angle=0;
int conv_flag=0;

uint8_t tof_data_ready = 0;
uint16_t tof_distance_mm = 0;

// 函数声明
uint8_t Process_OpenMV_Data(void);
uint8_t String_To_Number(char *str, uint8_t length);
int8_t Parse_OpenMV_Data(char *data, uint8_t *number);
int8_t Parse_TOF250_Data(char *data, uint16_t *distance);

// 手动实现字符串转数字函数
uint8_t String_To_Number(char *str, uint8_t length)
{
    uint8_t result = 0;
    for(uint8_t i = 0; i < length; i++)
    {
        if(str[i] >= '0' && str[i] <= '9')
        {
            result = result * 10 + (str[i] - '0');
        }
    }
    return result;
}

// USART1中断服务函数
void USART1_IRQHandler(void)
{
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        char received_char = USART_ReceiveData(USART1);
        
		if(conv_flag==0){
			// 检查缓冲区是否已满
			if(rx_index < RX_BUFFER_SIZE - 1)
			{
				uart_rx_buffer[rx_index++] = received_char;
				
				// 检测到感叹号，表示一帧数据接收完成
				if(received_char == '!')
				{
					uart_rx_buffer[rx_index] = '\0'; // 添加字符串结束符
					openmv_data_ready = 1;
					
					// 立即处理数据
					detected_number = Process_OpenMV_Data();
					if(detected_number){
						USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
					}
					
					rx_index = 0; // 重置索引
				}
			}
			else
			{
				// 缓冲区溢出，重置
				rx_index = 0;
			}
		}
		else if(conv_flag==1){
			/* USARTx_IRQHandler 中 */
			if (rx_index < RX_BUFFER_SIZE - 1)
			{
				/* 等待帧头：\n */
				if (rx_index == 0)
				{
					if (received_char != '\n')
					{
						return;   // 丢弃非帧头数据
					}
				}

				uart_rx_buffer[rx_index++] = received_char;

				/* 帧结束：\r */
				if (received_char == '\r')
				{
					uart_rx_buffer[rx_index] = '\0';   // 字符串结束
					tof_data_ready = 1;

					/* 立即解析 */
					Parse_TOF250_Data(uart_rx_buffer, &tof_distance_mm);

					/* 清空，准备下一帧 */
					rx_index = 0;
				}
			}
			else
			{
				/* 缓冲区溢出，直接复位 */
				rx_index = 0;
			}

					}
			//		USART_SendData(USART1,received_char);
			//		USART_ClearITPendingBit(USART1, USART_IT_RXNE); // 清除中断标志

				}
}

// 解析数据函数
int8_t Parse_OpenMV_Data(char *data, uint8_t *number)
{
    uint8_t data_length = 0;
    
    // 计算数据长度（不包含结束符）
    while(data[data_length] != '\0' && data_length < RX_BUFFER_SIZE)
    {
        data_length++;
    }
    
    // 格式检查: "#a数字!"
    if(data_length < 4) // 最小长度 "#a1!"
    {
        return -1; // 数据太短
    }
    
    // 检查帧头格式
    if(data[0] != '#' || data[1] != 'a')
    {
        return -2; // 格式错误
    }
    
    // 查找感叹号位置
    uint8_t exclamation_pos = 0;
    for(uint8_t i = 2; i < data_length; i++)
    {
        if(data[i] == '!')
        {
            exclamation_pos = i;
            break;
        }
    }
    
    if(exclamation_pos == 0)
    {
        return -2; // 没有找到感叹号
    }
    
    // 提取数字部分
    uint8_t num_start_index = 2;
    uint8_t num_end_index = exclamation_pos;
    uint8_t num_length = num_end_index - num_start_index;
    
    if(num_length == 0 || num_length > MAX_NUMBER_LENGTH)
    {
        return -3; // 数字长度错误
    }
    
    // 验证数字部分是否都是数字字符
    for(uint8_t i = num_start_index; i < num_end_index; i++)
    {
        if(data[i] < '0' || data[i] > '9')
        {
            return -4; // 非数字字符
        }
    }
    
    // 手动转换数字
    *number = String_To_Number(&data[num_start_index], num_length);
    return 0; // 成功
}


int8_t Parse_TOF250_Data(char *data, uint16_t *distance)
{
    /* 最小长度：\n123\r → 5 */
    uint8_t len = strlen(data);
    if (len < 5)
        return -1;

    /* 帧头帧尾检查 */
    if (data[0] != '\n' || data[len - 1] != '\r')
        return -2;

    /* 数字区必须正好 3 位 */
    if (len != 5)
        return -3;

    /* 校验是否全是数字 */
    for (uint8_t i = 1; i <= 3; i++)
    {
        if (data[i] < '0' || data[i] > '9')
            return -4;
    }

    /* ASCII → 数值 */
    *distance =
        (data[1] - '0') * 100 +
        (data[2] - '0') * 10  +
        (data[3] - '0');

    return 0;   // 成功
}


// 初始化USART1函数
void USART1_Init(int bound)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // 使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
    
    // 配置USART1 Tx (PA9) 为推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 配置USART1 Rx (PA10) 为浮空输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // USART参数配置
    USART_InitStructure.USART_BaudRate = bound;  
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    
    USART_Init(USART1, &USART_InitStructure);
    
    // 使能USART1接收中断
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    
    // 配置NVIC
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // 使能USART1
    USART_Cmd(USART1, ENABLE);
}

// 处理OpenMV数据函数
uint8_t Process_OpenMV_Data(void)
{
    if(openmv_data_ready)
    {
        uint8_t number = 0;
        int8_t result = Parse_OpenMV_Data(uart_rx_buffer, &number);
        
        if(result == 0)
        {
            // 解析成功，返回识别到的数字
            // 可以在这里添加调试信息
            // USART1_SendString("Success:");
            // USART1_SendNumber(number);
            return number;
        }
        else
        {
            // 解析失败，返回0
            // 可以在这里添加错误处理
            // USART1_SendString("Error:");
            // USART1_SendNumber(result);
            return 0;
        }
        
        openmv_data_ready = 0; // 重置标志
    }
    return 0;
}

// 发送数据函数
void USART1_SendString(char *str)
{
    while(*str)
    {
        while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, *str++);
    }
}

// 发送数字函数（用于调试）
void USART1_SendNumber(uint8_t num)
{
    char buffer[4];
    if(num >= 100)
    {
        buffer[0] = '0' + num / 100;
        buffer[1] = '0' + (num % 100) / 10;
        buffer[2] = '0' + num % 10;
        buffer[3] = '\0';
    }
    else if(num >= 10)
    {
        buffer[0] = '0' + num / 10;
        buffer[1] = '0' + num % 10;
        buffer[2] = '\0';
    }
    else
    {
        buffer[0] = '0' + num;
        buffer[1] = '\0';
    }
    USART1_SendString(buffer);
}



// 清除检测到的数字
void Clear_Detected_Number(void)
{
    detected_number = 0;
}
