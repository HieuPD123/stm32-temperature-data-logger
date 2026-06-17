#ifndef UART1_H
#define UART1_H

#include "main.h"
typedef enum
{
    CMD_NONE = 0,
    CMD_READ,
    CMD_RTC_SET
} UART_CommandType;

typedef struct
{
    UART_CommandType type;
    uint32_t value;

    uint16_t year;
    uint8_t month;
    uint8_t day;

    uint8_t hour;
    uint8_t minute;
    uint8_t second;

} UART_Command_t;
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
uint8_t UART1_Parse(char *line, UART_Command_t *cmd);
void UART1_Init(void);

void UART1_SendString(char *str);

uint8_t UART1_LineAvailable(void);
char* UART1_GetLine(void);

#endif
