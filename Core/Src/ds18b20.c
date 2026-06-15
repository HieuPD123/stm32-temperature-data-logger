#include "ds18b20.h"

extern TIM_HandleTypeDef htim4;

#define DS18B20_PORT GPIOA
#define DS18B20_PIN  GPIO_PIN_1

static void delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim4,0);
    while(__HAL_TIM_GET_COUNTER(&htim4) < us);
}

static inline void OW_WriteLow(void)
{
    HAL_GPIO_WritePin(
        DS18B20_PORT,
        DS18B20_PIN,
        GPIO_PIN_RESET
    );
}

static inline void OW_Release(void)
{
    HAL_GPIO_WritePin(
        DS18B20_PORT,
        DS18B20_PIN,
        GPIO_PIN_SET
    );
}

uint8_t DS18B20_Start(void)
{
    uint8_t response = 0;

    OW_WriteLow();
    delay_us(480);

    OW_Release();
    delay_us(80);

    if(!HAL_GPIO_ReadPin(
        DS18B20_PORT,
        DS18B20_PIN))
    {
        response = 1;
    }

    delay_us(400);

    return response;
}

void DS18B20_Write(uint8_t data)
{
    for(uint8_t i=0;i<8;i++)
    {
        if(data & (1<<i))
        {
            OW_WriteLow();
            delay_us(2);

            OW_Release();
            delay_us(58);
        }
        else
        {
            OW_WriteLow();
            delay_us(60);

            OW_Release();
            delay_us(2);
        }
    }
}

uint8_t DS18B20_Read(void)
{
    uint8_t value = 0;

    for(uint8_t i=0;i<8;i++)
    {
        OW_WriteLow();
        delay_us(2);

        OW_Release();
        delay_us(10);

        if(HAL_GPIO_ReadPin(
            DS18B20_PORT,
            DS18B20_PIN))
        {
            value |= (1<<i);
        }

        delay_us(50);
    }

    return value;
}

void DS18B20_StartConvert(void)
{
    if(!DS18B20_Start())
        return;

    DS18B20_Write(0xCC);
    DS18B20_Write(0x44);
}
// Cách 750ms
float DS18B20_ReadTemp(void)
{
    uint8_t Temp_Byte1;
    uint8_t Temp_Byte2;
    int16_t RAW_Temp;

    if(!DS18B20_Start())
        return -1000.0f;

    DS18B20_Write(0xCC);
    DS18B20_Write(0xBE);

    Temp_Byte1 = DS18B20_Read();
    Temp_Byte2 = DS18B20_Read();

    RAW_Temp = (Temp_Byte2 << 8) | Temp_Byte1;

    return (float)RAW_Temp / 16.0f;
}
