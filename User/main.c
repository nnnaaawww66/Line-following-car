/**************************引脚定义*****************************
PB11-> 蜂鸣器
电机(TB6612驱动)
[PB12] -> [AIN1] 电机1
[PB13] -> [AIN2] 电机1
[PA0 ] -> [PWMA]

[PB14] -> [BIN1] 电机2
[PB15] -> [BIN2] 电机2
[PA1 ] -> [PWMB]

编码器
PB6,PB7 -> 编码器2
PB0,PB1 -> 编码器1

OLED
[PB8 ]-> [SCL]
[PB9 ]-> [SDA]
**************************************************************/

#include "stm32f10x.h"                  // Device header
#include "Voice.h"
#include "Delay.h"
#include "Motor.h"
#include "Encoder.h"
#include "PID.h"
#include "nrf24l01.h"
#include "stdbool.h"
#include "Timer.h"
#include "OLED.h"
#include "usart.h"
#include "sensor.h"
#include "light.h"
#include "jy61p.h"
#include "usart3.h"
#include "string.h"
#include "relay.h"


/*****************************宏定义区域************************************/
#define SENSOR_COUNT 5
#define TURN_STEPS 5             // 转弯调整步数

/*****************************状态定义区域************************************/
typedef enum {
    STATE_INIT,            // 初始化状态
    STATE_WAIT_NUMBER,     // 等待数字识别
    STATE_MOVE_LINE,       // 直线巡线状态
    STATE_Stop_Line,      // 横线处理状态
    STATE_MOVE_CIRCLE,     // 圆形巡线状态
    STATE_FINISH           // 完成状态
} CarState;

/*****************************结构体定义区域************************************/
typedef struct {
    int road[SENSOR_COUNT];    // 传感器状态
    int sensor_num;            // 检测到的传感器数量
    int flag;                  // 巡线偏差值
} SensorData;

/*****************************变量声明区域************************************/
extern uint8_t detected_number;
extern uint8_t actual_angle;
extern int conv_flag;
extern uint16_t tof_distance_mm;


// PID结构体
PID_t L_SpeedPID = {
    .Kp = 2, .Ki = 0.1, .Kd = 0.01,
    .OutMax = 100, .OutMin = -100
};

PID_t R_SpeedPID = {
    .Kp = 2, .Ki = 0.1, .Kd = 0.01,
    .OutMax = 100, .OutMin = -100
};

// 全局状态变量
static CarState car_state = STATE_INIT;
static uint32_t state_timer = 0;      // 状态计时器
static uint32_t run_time = 0;         // 运行时间
static SensorData sensor_data;        // 传感器数据

// 标志位
uint8_t SendFlag, ReceiveFlag;
uint8_t SendSuccessCount, SendFailedCount;
uint8_t ReceiveSuccessCount, ReceiveFailedCount;

/*****************************函数声明区域************************************/
void MotorL_SetSpeed(float speed);
void MotorR_SetSpeed(float speed);
void Update_Sensor_Data(void);
void Car_Stop(void);
void State_Init_Handler(void);
void State_Wait_Number_Handler(void);
void State_Move_Line_Handler(void);
void State_Stop_Line_Handler(void);
void State_Move_Circle_Handler(void);
void State_Turn_Around_Handler(void);
void State_Finish_Handler(void);
void Execute_State_Machine(void);
void Display_Status(void);

/***************************主函数********************************************/
int main(void) {
    // 初始化所有外设
    Timer_Init();
    
    USART1_Init(9600);
	usart3_Init(9600);

    OLED_Init();
    Motor_Init();
    EncoderL_Init();
    EncoderR_Init();
    Voice_Init();
    light_Init();
    Relay_Init();
	
    OLED_Clear();
	Relay_OFF();
	
    // 主循环 - 状态机执行
    while (1) {
        // 更新运行时间
        run_time = Timer_GetTotalMs()/20;
        
        // 更新传感器数据
        Update_Sensor_Data();
        
        // 执行状态机
        Execute_State_Machine();
        
        // 显示状态信息
        Display_Status();
        
        OLED_Update();
    }
}

/*********************************函数定义区域************************************/

/**
  * @brief  更新传感器数据
  * @param  无
  * @retval 无
  */
void Update_Sensor_Data(void) {
    sensor_data.sensor_num = 0;
    sensor_data.flag = 0;
    
    for (int i = 0; i < SENSOR_COUNT; i++) {
        sensor_data.road[i] = Sensor_Read(i + 1);
        sensor_data.sensor_num += sensor_data.road[i];
        
        // 计算偏差权重（中间权重小，两边权重大）
        if (sensor_data.road[i]) {
            switch (i) {
                case 0: sensor_data.flag += -3; break;  // 最左边
                case 1: sensor_data.flag += -1; break;  // 左边
                case 2: sensor_data.flag += 0;  break;  // 中间
                case 3: sensor_data.flag += 1;  break;  // 右边
                case 4: sensor_data.flag += 3;  break;  // 最右边
            }
        }
    }
}

/**
  * @brief  电机速度控制函数
  * @param  speed: 目标速度
  * @retval 无
  */
void MotorL_SetSpeed(float speed) {
    L_SpeedPID.Target = speed;
    L_SpeedPID.Actual = -EncoderL_Get_Speed();
    PID_Update(&L_SpeedPID);
    MotorL_Set_PWM(L_SpeedPID.Out);
}

void MotorR_SetSpeed(float speed) {
    R_SpeedPID.Target = speed;
    R_SpeedPID.Actual = -EncoderR_Get_Speed();
    PID_Update(&R_SpeedPID);
    MotorR_Set_PWM(R_SpeedPID.Out);
}
	
void Car_Stop(void) {
	MotorR_Set_PWM(5);
    MotorL_Set_PWM(-5);
}	
	

/**
  * @brief  执行状态机
  * @param  无
  * @retval 无
  */
void Execute_State_Machine(void) {
    switch (car_state) {
        case STATE_INIT:
            State_Init_Handler();
            break;
            
        case STATE_WAIT_NUMBER:
            State_Wait_Number_Handler();
            break;
            
        case STATE_MOVE_LINE:
            State_Move_Line_Handler();
            break;
            
        case STATE_Stop_Line:
            State_Stop_Line_Handler();
            break;
            
        case STATE_MOVE_CIRCLE:
            State_Move_Circle_Handler();
            break;
            
        case STATE_FINISH:
            State_Finish_Handler();
            break;
            
        default:
            car_state = STATE_INIT;
            break;
    }
}

/**
  * @brief  初始化状态处理
  * @param  无
  * @retval 无
  */
void State_Init_Handler(void) {
    // 初始化变量
	car_state = STATE_WAIT_NUMBER;
	
//	car_state=STATE_MOVE_LINE;
//	Timer_Start();
	
}

/**
  * @brief  等待数字识别状态处理
  * @param  无
  * @retval 无
  */
void State_Wait_Number_Handler(void) {
    // 如果需要等待数字识别，可以在这里实现
    // 目前直接进入巡线状态
	if(detected_number) {
    car_state = STATE_MOVE_LINE;
	Timer_Start();
	Relay_ON();
	USART1_Init(115200);
	conv_flag=1;
	}
}

/**
  * @brief  直线巡线状态处理
  * @param  无
  * @retval 无
  */
void State_Move_Line_Handler(void) {
    static uint32_t line_detect_counter = 0;
    
    // 检查是否检测到横线
    if (sensor_data.sensor_num >= 4 && line_detect_counter > 200) {
        car_state = STATE_Stop_Line;
        state_timer = run_time;
        line_detect_counter = 0;
        return;
    }
    
    // 巡线控制
	if(line_detect_counter<1){
		MotorR_SetSpeed(-15);
        MotorL_SetSpeed(15);
	}else if (line_detect_counter < 20) {  // 启动阶段
        MotorR_SetSpeed(-15+ 4 * sensor_data.flag);
        MotorL_SetSpeed(15+ 4 * sensor_data.flag);
    } else if(line_detect_counter<225) {  // PID巡线阶段
        MotorR_SetSpeed(-26 + 3 * sensor_data.flag);
        MotorL_SetSpeed(26 + 3 * sensor_data.flag);
    }else {
		MotorR_SetSpeed(-15 + 3 * sensor_data.flag);
        MotorL_SetSpeed(15 + 3 * sensor_data.flag);
	}
    
    line_detect_counter++;
}

/**
  * @brief  横线处理状态
  * @param  无
  * @retval 无
  */
void State_Stop_Line_Handler(void) {
	if(run_time - state_timer < 500)Car_Stop();
	else{
		light_ON();
		Voice_ON();
		 Delay_ms(300);
		 light_OFF();
		Voice_OFF();
		 Delay_ms(900);
		light_ON();
		Voice_ON();
		 Delay_ms(300);
		 light_OFF();
		Voice_OFF();
	
		car_state = STATE_MOVE_CIRCLE;
	}
}

/**
  * @brief  圆形巡线状态处理
  * @param  无
  * @retval 无
  */
void State_Move_Circle_Handler(void) {
	static int phase=0;
	OLED_Printf(110, 0, OLED_6X8, "%d", phase);

    
    switch (phase) {
		case 0:  //转向第一个对角
			// 根据偏航角判断
			if (Yaw >= 146) {
				MotorR_SetSpeed(5);
				MotorL_SetSpeed(5);
			}
			else{
				Car_Stop();
				state_timer = run_time;
				phase=1;
			}
			break;
			
        case 1:  // 朝第一个对角走
            MotorR_SetSpeed(-15);
            MotorL_SetSpeed(15);
			if(sensor_data.sensor_num>=2||run_time-state_timer>4400){
				Car_Stop();
				phase=2;
				state_timer = run_time;				
			}
            break;
            
		case 2:	  //在第一个对角转向
			if (Yaw <= 160) {
				MotorR_SetSpeed(-8);
				MotorL_SetSpeed(-8);
			}else {
				Car_Stop();
				phase=3;
			}
			break;
			
		case 3:
			MotorR_SetSpeed(-15);
            MotorL_SetSpeed(15);
			if(sensor_data.sensor_num){
				state_timer = run_time;
				phase=4;
			}
			break;
		
        case 4:  // 巡线辅助的圆形运动
            MotorR_SetSpeed(-15 + 4 * sensor_data.flag);
            MotorL_SetSpeed(15 + 4 * sensor_data.flag);
			if(run_time-state_timer>4950&&sensor_data.sensor_num==0){
				Car_Stop();
				phase=5;
				state_timer = run_time;				
			}
            break;
            //*-
        case 5:  // 转向第二个对角
            // 停车
//			if(run_time-state_timer<500){
//				Car_Stop();
//			}
            // 调整
            if ((0<=Yaw&&Yaw <= 17)||(Yaw<=360&&Yaw>=300)) {
                MotorR_SetSpeed(-7);
                MotorL_SetSpeed(-7);
            } else {
                Car_Stop();
				state_timer = run_time;
				phase=6;
            }			
            break;
			
		case 6: //走向第二个对角
			 MotorR_SetSpeed(-15);
            MotorL_SetSpeed(15);
			if(run_time-state_timer>3650||sensor_data.sensor_num>=2){
				Car_Stop();
				phase=7;
				state_timer = run_time;				
			}
            break;
			
		case 7:  //在第二个对角转向
			if (Yaw<120&&Yaw>30) {
				MotorR_SetSpeed(6);
				MotorL_SetSpeed(6);
			}else {
				Car_Stop();
				state_timer = run_time;
				phase=8;
			}
			break;
			
		case 8:  //走入终点线
			MotorR_SetSpeed(-15 + 3 * sensor_data.flag);
            MotorL_SetSpeed(15 + 3 * sensor_data.flag);
			if(tof_distance_mm<=140||run_time-state_timer>=3500){
				for(int i=0;i<=800;i++){
					MotorR_SetSpeed(-10 + 2 * sensor_data.flag);
					MotorL_SetSpeed(10 + 2 * sensor_data.flag);	
				}
				Car_Stop();
				car_state=STATE_FINISH;
				state_timer = run_time;
				Timer_Stop();				
			}
            break;
    }
}


/**
  * @brief  完成状态处理
  * @param  无
  * @retval 无
  */
void State_Finish_Handler(void) {
    // 停止所有电机
    Car_Stop();
    
    // 闪烁提示完成
        light_ON();
        Voice_ON();
        Delay_ms(500);
        light_OFF();
        Voice_OFF();
	    Delay_ms(500);
		light_ON();
        Voice_ON();
        Delay_ms(500);
        light_OFF();
        Voice_OFF();
}

/**
  * @brief  显示状态信息
  * @param  无
  * @retval 无
  */
void Display_Status(void) {
    char state_name[16];
    
    // 根据状态显示不同的名称
    switch (car_state) {
        case STATE_INIT:          strcpy(state_name, "INIT"); break;
        case STATE_WAIT_NUMBER:   strcpy(state_name, "WAITING FOR NUMBER"); break;
        case STATE_MOVE_LINE:     strcpy(state_name, "LINE"); break;
        case STATE_Stop_Line:    strcpy(state_name, "STOP_LINE"); break;
        case STATE_MOVE_CIRCLE:   strcpy(state_name, "CIRCLE"); break;
        case STATE_FINISH:        strcpy(state_name, "FINISH"); break;
        default:                  strcpy(state_name, "UNKNOWN"); break;
    }
    
    // 显示信息
    OLED_Printf(0, 0, OLED_6X8, "State: %s", state_name);
    OLED_Printf(0, 10, OLED_8X16, "Time: %.3f s" , (float)Timer_GetTotalMs()/20000);
    OLED_Printf(0, 28, OLED_6X8, "Yaw: %.1f", Yaw);
	OLED_Printf(0, 38, OLED_6X8, "distance: %d", tof_distance_mm);

    OLED_Printf(0, 46, OLED_8X16, "number: %d", detected_number);
}

/*********************************中断函数区域************************************/
void TIM1_UP_IRQHandler(void) {
    if (TIM_GetITStatus(TIM1, TIM_IT_Update) == SET) {
        TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
    }
}
