#ifndef DS18B20_H_
#define DS18B20_H_

#include "main.h"

uint8_t DS18B20_Start(void);
void DS18B20_Write(uint8_t data);
uint8_t DS18B20_Read(void);
void DS18B20_StartConvert(void);
float DS18B20_ReadTemp(void);

#endif
