/**
 ******************************************************************************
 * @file    oled_ui.c
 * @brief   Giao dien 3 phan tren OLED SSD1306 128x64.
 ******************************************************************************
 */
#include "oled_ui.h"
#include "ssd1306.h"
#include "font.h"
#include "main.h"
#include <string.h>
#include <stdio.h>

/* ====================== ICON NHIET KE 16x16 ====================== */
static const uint8_t Thermo16x16[] = {
    0x01, 0x80, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40,
    0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x03, 0xC0,
    0x03, 0xC0, 0x03, 0xC0, 0x07, 0xE0, 0x0F, 0xF0,
    0x0F, 0xF0, 0x0F, 0xF0, 0x07, 0xE0, 0x03, 0xC0,
};

/* ====================== CACHE DU LIEU ====================== */
static float          s_temp_c      = 0.0f;
static OLED_SDState_t s_sd_state    = OLED_SD_ERROR;
static uint8_t        s_sd_used_pct = 0;

void OLED_SetTemperature(float t)
{
    s_temp_c = t;
}

void OLED_SetSD(OLED_SDState_t state, uint8_t used_pct)
{
    s_sd_state    = state;
    s_sd_used_pct = (used_pct > 100) ? 100 : used_pct;
}

/* ====================== DRAW HELPERS ====================== */

static void UI_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    for(uint8_t i = 0; i < w; i++)
    {
        SSD1306_DrawPixel(x + i, y, 1);
        SSD1306_DrawPixel(x + i, y + h - 1, 1);
    }

    for(uint8_t j = 0; j < h; j++)
    {
        SSD1306_DrawPixel(x, y + j, 1);
        SSD1306_DrawPixel(x + w - 1, y + j, 1);
    }
}

static void UI_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    for(uint8_t j = 0; j < h; j++)
    {
        for(uint8_t i = 0; i < w; i++)
        {
            SSD1306_DrawPixel(x + i, y + j, 1);
        }
    }
}

static void UI_DrawBitmap(uint8_t x, uint8_t y,
                          const uint8_t *bmp,
                          uint8_t w,
                          uint8_t h)
{
    uint8_t bytesPerRow = (w + 7) / 8;

    for(uint8_t r = 0; r < h; r++)
    {
        for(uint8_t c = 0; c < w; c++)
        {
            uint8_t b = bmp[r * bytesPerRow + (c >> 3)];

            if(b & (0x80 >> (c & 7)))
            {
                SSD1306_DrawPixel(x + c, y + r, 1);
            }
        }
    }
}

static void UI_DrawStringScaled(uint8_t x,
                                uint8_t y,
                                const char *str,
                                uint8_t scale)
{
    while(*str)
    {
        const uint8_t *glyph = Font5x7_GetChar(*str);

        for(uint8_t col = 0; col < 5; col++)
        {
            uint8_t line = glyph[col];

            for(uint8_t row = 0; row < 8; row++)
            {
                if(line & 0x01)
                {
                    UI_FillRect(
                        x + col * scale,
                        y + row * scale,
                        scale,
                        scale
                    );
                }

                line >>= 1;
            }
        }

        x += 6 * scale;
        str++;
    }
}

static void UI_DrawStringCenter(uint8_t y, const char *s)
{
    uint8_t w = strlen(s) * 6;

    uint8_t x =
        (w < SSD1306_WIDTH) ?
        ((SSD1306_WIDTH - w) / 2) :
        0;

    SSD1306_SetCursor(x, y);
    SSD1306_WriteString((char *)s);
}

/* ====================== OLED UPDATE ====================== */

void Update_OLED_Display(void)
{
    char buf[16];

    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;

    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    SSD1306_Clear();

    /* -------- TIME -------- */

    snprintf(buf,
             sizeof(buf),
             "%02u:%02u:%02u",
             sTime.Hours,
             sTime.Minutes,
             sTime.Seconds);

    UI_DrawStringCenter(2, buf);

    /* -------- TEMPERATURE -------- */

    UI_DrawBitmap(6, 22, Thermo16x16, 16, 16);

    int32_t v =
        (int32_t)(s_temp_c * 10.0f +
        (s_temp_c >= 0 ? 0.5f : -0.5f));

    char sign[2] = {0};

    if(v < 0)
    {
        sign[0] = '-';
        v = -v;
    }

    snprintf(buf,
             sizeof(buf),
             "%s%ld.%ld",
             sign,
             (long)(v / 10),
             (long)(v % 10));

    uint8_t tx = 28;
    uint8_t ty = 21;

    UI_DrawStringScaled(tx, ty, buf, 2);

    uint8_t after =
        tx + strlen(buf) * 12;

    SSD1306_DrawSmallCircle(after + 2, ty);
    UI_DrawStringScaled(after + 8, ty, "C", 2);

    /* -------- SD STATUS -------- */

    const char *sd_txt;

    switch(s_sd_state)
    {
        case OLED_SD_OK:
            sd_txt = "SD: OK";
            break;

        case OLED_SD_ERROR:
            sd_txt = "SD: ERR";
            break;

        default:
            sd_txt = "SD: ERR";
            break;
    }

    SSD1306_SetCursor(2, 55);
    SSD1306_WriteString((char *)sd_txt);

    /* -------- FREE SPACE BAR -------- */

    const uint8_t bx = 66;
    const uint8_t by = 54;
    const uint8_t bw = 58;
    const uint8_t bh = 9;

    UI_DrawRect(bx, by, bw, bh);

    if(s_sd_state == OLED_SD_OK)
    {
        uint8_t innerW = bw - 4;

        uint8_t fill =
            ((uint16_t)innerW * s_sd_used_pct)
            / 100;

        if(fill > 0)
        {
            UI_FillRect(
                bx + 2,
                by + 2,
                fill,
                bh - 4
            );
        }
    }

    SSD1306_UpdateScreen();
}
