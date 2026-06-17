#include "uart1.h"
#include <string.h>
#include <stdio.h>
#include <sys/unistd.h>
extern UART_HandleTypeDef huart1;

static uint8_t rx_char;

static char rx_line[128];
static volatile uint16_t rx_index = 0;
static volatile uint8_t line_ready = 0;


void UART1_Init(void)
{
    HAL_UART_Receive_IT(&huart1, &rx_char, 1);
}

void UART1_SendString(char *str)
{
    HAL_UART_Transmit(
        &huart1,
        (uint8_t*)str,
        strlen(str),
        HAL_MAX_DELAY);
}

uint8_t UART1_LineAvailable(void)
{
    return line_ready;
}

char* UART1_GetLine(void)
{
    line_ready = 0;
    return rx_line;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1)
    {
        if(rx_char == '\n')
        {
            rx_line[rx_index] = '\0';
            rx_index = 0;
            line_ready = 1;
        }
        else if(rx_char != '\r')
        {
            if(rx_index < sizeof(rx_line)-1)
            {
                rx_line[rx_index++] = rx_char;
            }
        }

        HAL_UART_Receive_IT(&huart1, &rx_char, 1);
    }
}

uint8_t UART1_Parse(char *line, UART_Command_t *cmd)
{
    memset(cmd, 0, sizeof(UART_Command_t));

    int r_value;
    if(sscanf(line, "READ %d", &r_value) == 1)
    {
        cmd->type = CMD_READ;
        cmd->value = (uint32_t)r_value;
        return 1;
    }

    int r_year, r_month, r_day, r_hour, r_min, r_sec;
    // Sử dụng %2d cho các trường thời gian để ép sscanf chỉ đọc đúng số lượng chữ số cố định
    if(sscanf(line,
              "RTC:%4d-%2d-%2d %2d:%2d:%2d",
              &r_year,
              &r_month,
              &r_day,
              &r_hour,
              &r_min,
              &r_sec) == 6)
    {
        cmd->type   = CMD_RTC_SET;
        cmd->year   = (uint16_t)r_year;
        cmd->month  = (uint8_t)r_month;
        cmd->day    = (uint8_t)r_day;
        cmd->hour   = (uint8_t)r_hour;
        cmd->minute = (uint8_t)r_min;
        cmd->second = (uint8_t)r_sec;
        return 1;
    }

    return 0;
}


int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(
        &huart1,
        (uint8_t *)ptr,
        len,
        HAL_MAX_DELAY
    );

    return len;
}
