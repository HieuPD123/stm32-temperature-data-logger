/**
 ******************************************************************************
 * @file    oled_ui.c
 * @brief   Giao dien 3 phan tren OLED SSD1306 128x64.
 *          KHONG phu thuoc FatFs - chi nhan du lieu qua setter va ve.
 ******************************************************************************
 */
#include "oled_ui.h"
#include "ssd1306.h"        /* SSD1306_DrawPixel/SetCursor/WriteString/Clear/UpdateScreen/DrawSmallCircle */
#include "font.h"           /* Font5x7_GetChar(char) -> 5 byte (moi byte la 1 cot, bit LSB = pixel tren) */
#include "internal_rtc.h"   /* Internal_RTC_GetTime() */
#include <string.h>
#include <stdio.h>

/* ====================== ICON NHIET KE 16x16 (row-major, bit cao = cot trai) ===== */
static const uint8_t Thermo16x16[] = {
    0x01, 0x80, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40,
    0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x03, 0xC0,
    0x03, 0xC0, 0x03, 0xC0, 0x07, 0xE0, 0x0F, 0xF0,
    0x0F, 0xF0, 0x0F, 0xF0, 0x07, 0xE0, 0x03, 0xC0,
};

/* ====================== CACHE DU LIEU (cap nhat qua setter) ==================== */
static float          s_temp_c      = 0.0f;
static OLED_SDState_t s_sd_state    = OLED_SD_ABSENT;
static uint8_t        s_sd_free_pct = 0;

void OLED_SetTemperature(float t) { s_temp_c = t; }

void OLED_SetSD(OLED_SDState_t state, uint8_t free_pct)
{
    s_sd_state    = state;
    s_sd_free_pct = (free_pct > 100) ? 100 : free_pct;
}

/* ====================== CAC HAM VE BO SUNG (dung tren SSD1306_DrawPixel) ======== */

/* Khung chu nhat (vien) */
static void UI_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    for (uint8_t i = 0; i < w; i++) { SSD1306_DrawPixel(x + i, y, 1); SSD1306_DrawPixel(x + i, y + h - 1, 1); }
    for (uint8_t j = 0; j < h; j++) { SSD1306_DrawPixel(x, y + j, 1); SSD1306_DrawPixel(x + w - 1, y + j, 1); }
}

/* Chu nhat dac */
static void UI_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    for (uint8_t j = 0; j < h; j++)
        for (uint8_t i = 0; i < w; i++)
            SSD1306_DrawPixel(x + i, y + j, 1);
}

/* Ve bitmap row-major (bit cao = cot trai) */
static void UI_DrawBitmap(uint8_t x, uint8_t y, const uint8_t *bmp, uint8_t w, uint8_t h)
{
    uint8_t bytesPerRow = (w + 7) / 8;
    for (uint8_t r = 0; r < h; r++)
        for (uint8_t c = 0; c < w; c++) {
            uint8_t b = bmp[r * bytesPerRow + (c >> 3)];
            if (b & (0x80 >> (c & 7)))
                SSD1306_DrawPixel(x + c, y + r, 1);
        }
}

/* Ve chuoi font 5x7 phong to theo he so 'scale' (scale=2 -> chu cao gap doi) */
static void UI_DrawStringScaled(uint8_t x, uint8_t y, const char *str, uint8_t scale)
{
    while (*str) {
        const uint8_t *glyph = Font5x7_GetChar(*str);   /* 5 cot */
        for (uint8_t col = 0; col < 5; col++) {
            uint8_t line = glyph[col];
            for (uint8_t row = 0; row < 8; row++) {
                if (line & 0x01)
                    UI_FillRect(x + col * scale, y + row * scale, scale, scale);
                line >>= 1;
            }
        }
        x += 6 * scale;
        str++;
    }
}

/* In chuoi font thuong, can giua theo chieu ngang tai dong y */
static void UI_DrawStringCenter(uint8_t y, const char *s)
{
    uint8_t w = (uint8_t)(strlen(s) * 6);               /* font 5x7 -> 6px/ky tu */
    uint8_t x = (w < SSD1306_WIDTH) ? (uint8_t)((SSD1306_WIDTH - w) / 2) : 0;
    SSD1306_SetCursor(x, y);
    SSD1306_WriteString((char *)s);
}

/* ====================== HAM CHINH: VE GIAO DIEN ================================ */
void Update_OLED_Display(void)
{
    char buf[16];

    /* ---- thoi gian thuc tu RTC ---- */
    Internal_RTC_TimeTypeDef t;
    Internal_RTC_GetTime(&t);

    SSD1306_Clear();

    /* ===== DONG 1: THOI GIAN "HH:MM:SS" can giua ===== */
    snprintf(buf, sizeof(buf), "%02u:%02u:%02u", t.hours, t.minutes, t.seconds);
    UI_DrawStringCenter(2, buf);

    /* ===== DONG 2: ICON NHIET KE + GIA TRI (chu to gap doi) + °C ===== */
    UI_DrawBitmap(6, 22, Thermo16x16, 16, 16);

    /* dinh dang 1 chu so thap phan KHONG dung float-printf */
    int32_t v = (int32_t)(s_temp_c * 10.0f + (s_temp_c >= 0 ? 0.5f : -0.5f));
    char sign[2] = {0};
    if (v < 0) { sign[0] = '-'; v = -v; }
    snprintf(buf, sizeof(buf), "%s%ld.%ld", sign, (long)(v / 10), (long)(v % 10));

    uint8_t tx = 28, ty = 21;
    UI_DrawStringScaled(tx, ty, buf, 2);
    uint8_t after = (uint8_t)(tx + strlen(buf) * 12);
    SSD1306_DrawSmallCircle(after + 2, ty);
    UI_DrawStringScaled(after + 8, ty, "C", 2);

    /* ===== DONG 3: trai = trang thai SD | phai = thanh dung luong ===== */
    const char *sd_txt;
    switch (s_sd_state) {
        case OLED_SD_OK:    sd_txt = "SD:OK";  break;
        case OLED_SD_ERROR: sd_txt = "SD:ERR"; break;
        default:            sd_txt = "SD:--";  break;   /* ABSENT */
    }
    SSD1306_SetCursor(2, 55);
    SSD1306_WriteString((char *)sd_txt);

    /* thanh tien trinh (nua phai): khung luon ve; chi lap day khi the OK */
    const uint8_t bx = 66, by = 54, bw = 58, bh = 9;
    UI_DrawRect(bx, by, bw, bh);
    if (s_sd_state == OLED_SD_OK) {
        uint8_t innerW = bw - 4;
        uint8_t fill   = (uint8_t)(((uint16_t)innerW * s_sd_free_pct) / 100u);
        if (fill > 0) UI_FillRect(bx + 2, by + 2, fill, bh - 4);
    }

    SSD1306_UpdateScreen();
}
