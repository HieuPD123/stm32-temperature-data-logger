#include "font.h"

static const uint8_t font_space[5] = {0x00,0x00,0x00,0x00,0x00};
static const uint8_t font_dot[5]   = {0x00,0x60,0x60,0x00,0x00};
static const uint8_t font_minus[5] = {0x08,0x08,0x08,0x08,0x08};
static const uint8_t font_colon[5] = {0x00,0x36,0x36,0x00,0x00};

static const uint8_t font_0[5] = {0x3E,0x51,0x49,0x45,0x3E};
static const uint8_t font_1[5] = {0x00,0x42,0x7F,0x40,0x00};
static const uint8_t font_2[5] = {0x42,0x61,0x51,0x49,0x46};
static const uint8_t font_3[5] = {0x21,0x41,0x45,0x4B,0x31};
static const uint8_t font_4[5] = {0x18,0x14,0x12,0x7F,0x10};
static const uint8_t font_5[5] = {0x27,0x45,0x45,0x45,0x39};
static const uint8_t font_6[5] = {0x3C,0x4A,0x49,0x49,0x30};
static const uint8_t font_7[5] = {0x01,0x71,0x09,0x05,0x03};
static const uint8_t font_8[5] = {0x36,0x49,0x49,0x49,0x36};
static const uint8_t font_9[5] = {0x06,0x49,0x49,0x29,0x1E};

static const uint8_t font_T[5] = {0x01,0x01,0x7F,0x01,0x01};
static const uint8_t font_e[5] = {0x38,0x54,0x54,0x54,0x18};
static const uint8_t font_m[5] = {0x7C,0x04,0x18,0x04,0x78};
static const uint8_t font_p[5] = {0x7C,0x14,0x14,0x14,0x08};
static const uint8_t font_C[5] = {0x3E,0x41,0x41,0x41,0x22};

const uint8_t* Font5x7_GetChar(char c)
{
    switch(c)
    {
        case ' ': return font_space;
        case '.': return font_dot;
        case '-': return font_minus;
        case ':': return font_colon;

        case '0': return font_0;
        case '1': return font_1;
        case '2': return font_2;
        case '3': return font_3;
        case '4': return font_4;
        case '5': return font_5;
        case '6': return font_6;
        case '7': return font_7;
        case '8': return font_8;
        case '9': return font_9;

        case 'T': return font_T;
        case 'e': return font_e;
        case 'm': return font_m;
        case 'p': return font_p;
        case 'C': return font_C;

        default: return font_space;
    }
}
