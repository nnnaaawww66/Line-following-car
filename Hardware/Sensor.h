#ifndef __SENSOR_H
#define __SENSOR_H 

void Sensor_Init(void);
uint8_t Sensor_Read(uint8_t sensor_num);
void Sensor_Deinit(void);

#endif

#ifndef SENSOR_PORT
	#define OPTICAL_SENSOR_NUM  5
	#define SENSOR1_PIN         GPIO_Pin_2
	#define SENSOR2_PIN         GPIO_Pin_3  
	#define SENSOR3_PIN         GPIO_Pin_4
	#define SENSOR4_PIN         GPIO_Pin_5
	#define SENSOR5_PIN         GPIO_Pin_6
#endif
