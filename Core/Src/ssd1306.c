#include "ssd1306.h"
#include "font.h"
#include <stdio.h>
#include <string.h>

extern I2C_HandleTypeDef hi2c1;

static uint8_t SSD1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

static uint8_t CurrentX = 0;
static uint8_t CurrentY = 0;

static void SSD1306_WriteCommand(uint8_t command)
{
    uint8_t data[2];

    data[0] = 0x00;
    data[1] = command;

    HAL_I2C_Master_Transmit(
        &hi2c1,
        SSD1306_I2C_ADDR,
        data,
        2,
        HAL_MAX_DELAY
    );
}

static void SSD1306_WriteData(uint8_t *data, uint16_t size)
{
    uint8_t buffer[129];

    buffer[0] = 0x40;

    memcpy(&buffer[1], data, size);

    HAL_I2C_Master_Transmit(
        &hi2c1,
        SSD1306_I2C_ADDR,
        buffer,
        size + 1,
        HAL_MAX_DELAY
    );
}

void SSD1306_Init(void)
{
    HAL_Delay(100);

    SSD1306_WriteCommand(0xAE); // Display OFF

    SSD1306_WriteCommand(0x20); // Memory addressing mode
    SSD1306_WriteCommand(0x00); // Horizontal addressing mode

    SSD1306_WriteCommand(0xB0); // Page start address

    SSD1306_WriteCommand(0xC8); // COM scan direction
    SSD1306_WriteCommand(0x00); // Low column
    SSD1306_WriteCommand(0x10); // High column

    SSD1306_WriteCommand(0x40); // Start line

    SSD1306_WriteCommand(0x81); // Contrast
    SSD1306_WriteCommand(0x7F);

    SSD1306_WriteCommand(0xA1); // Segment remap
    SSD1306_WriteCommand(0xA6); // Normal display

    SSD1306_WriteCommand(0xA8); // Multiplex ratio
    SSD1306_WriteCommand(0x3F);

    SSD1306_WriteCommand(0xA4); // Display follows RAM
    SSD1306_WriteCommand(0xD3); // Display offset
    SSD1306_WriteCommand(0x00);

    SSD1306_WriteCommand(0xD5); // Display clock divide
    SSD1306_WriteCommand(0x80);

    SSD1306_WriteCommand(0xD9); // Pre-charge
    SSD1306_WriteCommand(0xF1);

    SSD1306_WriteCommand(0xDA); // COM pins
    SSD1306_WriteCommand(0x12);

    SSD1306_WriteCommand(0xDB); // VCOM detect
    SSD1306_WriteCommand(0x40);

    SSD1306_WriteCommand(0x8D); // Charge pump
    SSD1306_WriteCommand(0x14);

    SSD1306_WriteCommand(0xAF); // Display ON

    SSD1306_Clear();
    SSD1306_UpdateScreen();
}

void SSD1306_Clear(void)
{
    memset(SSD1306_Buffer, 0x00, sizeof(SSD1306_Buffer));
}

void SSD1306_UpdateScreen(void)
{
    for(uint8_t page = 0; page < 8; page++)
    {
        SSD1306_WriteCommand(0xB0 + page);
        SSD1306_WriteCommand(0x00);
        SSD1306_WriteCommand(0x10);

        SSD1306_WriteData(
            &SSD1306_Buffer[SSD1306_WIDTH * page],
            SSD1306_WIDTH
        );
    }
}

void SSD1306_DrawPixel(uint8_t x, uint8_t y, uint8_t color)
{
    if(x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT)
    {
        return;
    }

    if(color)
    {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
    }
    else
    {
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
    }
}

void SSD1306_SetCursor(uint8_t x, uint8_t y)
{
    CurrentX = x;
    CurrentY = y;
}

void SSD1306_WriteChar(char ch)
{
    const uint8_t *bitmap = Font5x7_GetChar(ch);

    for(uint8_t i = 0; i < 5; i++)
    {
        uint8_t line = bitmap[i];

        for(uint8_t j = 0; j < 8; j++)
        {
            if(line & 0x01)
            {
                SSD1306_DrawPixel(CurrentX + i, CurrentY + j, 1);
            }

            line >>= 1;
        }
    }

    CurrentX += 6;
}

void SSD1306_WriteString(char *str)
{
    while(*str)
    {
        SSD1306_WriteChar(*str);
        str++;
    }
}

void SSD1306_DrawSmallCircle(uint8_t x, uint8_t y)
{
    SSD1306_DrawPixel(x + 1, y, 1);
    SSD1306_DrawPixel(x + 2, y, 1);

    SSD1306_DrawPixel(x, y + 1, 1);
    SSD1306_DrawPixel(x + 3, y + 1, 1);

    SSD1306_DrawPixel(x, y + 2, 1);
    SSD1306_DrawPixel(x + 3, y + 2, 1);

    SSD1306_DrawPixel(x + 1, y + 3, 1);
    SSD1306_DrawPixel(x + 2, y + 3, 1);
}

void SSD1306_ShowTemperature(float temp)
{
    char buffer[20];

    SSD1306_Clear();

    SSD1306_SetCursor(0, 0);
    SSD1306_WriteString("Temp:");

    snprintf(buffer, sizeof(buffer), "%.2f", temp);

    SSD1306_SetCursor(0, 20);
    SSD1306_WriteString(buffer);

    SSD1306_DrawSmallCircle(42, 20);

    SSD1306_SetCursor(48, 20);
    SSD1306_WriteString("C");

    SSD1306_UpdateScreen();
}
