/*
 * internal_rtc.c
 *
 *  Created on: Jun 12, 2026
 *      Author: Admin
 */
#include "internal_rtc.h"
#include "ssd1306.h"  // Quét màn hình OLED của bạn
#include "ds18b20.h"  // Đọc nhiệt độ

static RTC_HandleTypeDef *internal_hrtc;

static const uint8_t DaysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static uint8_t IsLeapYear(uint16_t year) {
    return (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0));
}

// Hàm khởi tạo link bộ phần cứng hrtc
void Internal_RTC_Init(RTC_HandleTypeDef *hrtc) {
    internal_hrtc = hrtc;
}

// Đổi từ cấu trúc Lịch sang Số giây tổng nạp vào thanh ghi Counter
void Internal_RTC_SetTime(Internal_RTC_TimeTypeDef *time) {
    uint32_t total_seconds = 0;
    uint16_t i;

    // Tính giây của các năm trước (Mốc từ năm 2000)
    for (i = 2000; i < time->year; i++) {
        total_seconds += IsLeapYear(i) ? 366 * 86400 : 365 * 86400;
    }
    // Tính giây của các tháng trước trong năm nay
    for (i = 0; i < (uint16_t)(time->month - 1); i++) {
        if (i == 1 && IsLeapYear(time->year)) {
            total_seconds += 29 * 86400;
        } else {
            total_seconds += DaysInMonth[i] * 86400;
        }
    }
    // Cộng nốt số ngày, giờ, phút, giây hiện tại
    total_seconds += (time->dayofmonth - 1) * 86400;
    total_seconds += time->hours * 3600;
    total_seconds += time->minutes * 60;
    total_seconds += time->seconds;

    // Ghi trực tiếp vào thanh ghi đếm RTC
    __HAL_RTC_WRITE_COUNTER(internal_hrtc, total_seconds);
}

// Đọc số giây tổng từ vi điều khiển chia ngược lại thành cấu trúc thời gian
static void Internal_RTC_GetTime(Internal_RTC_TimeTypeDef *time) {
    uint32_t time_count = HAL_RTC_GetCounter(internal_hrtc);
    uint32_t days, seconds_in_day;
    uint16_t year_tmp = 2000;
    uint8_t month_tmp = 0;

    days = time_count / 86400;
    seconds_in_day = time_count % 86400;

    // Giải mã Giờ : Phút : Giây
    time->hours = seconds_in_day / 3600;
    time->minutes = (seconds_in_day % 3600) / 60;
    time->seconds = (seconds_in_day % 3600) % 60;

    // Giải mã Năm
    while (1) {
        uint32_t days_in_year = IsLeapYear(year_tmp) ? 366 : 365;
        if (days >= days_in_year) {
            days -= days_in_year;
            year_tmp++;
        } else {
            break;
        }
    }
    time->year = year_tmp;

    // Giải mã Tháng
    while (1) {
        uint32_t days_in_month = DaysInMonth[month_tmp];
        if (month_tmp == 1 && IsLeapYear(year_tmp)) days_in_month = 29;

        if (days >= days_in_month) {
            days -= days_in_month;
            month_tmp++;
        } else {
            break;
        }
    }
    time->month = month_tmp + 1;
    time->dayofmonth = days + 1;
}

// Hàm cập nhật OLED
void Internal_RTC_Loop_Process(void)
{
    float current_temp = 0.0f;
    Internal_RTC_TimeTypeDef current_time;
    char buffer[30];

    // 1. Đọc dữ liệu thô từ cảm biến và RTC Counter nội
    current_temp = DS18B20_GetTemp();
    Internal_RTC_GetTime(&current_time);

    // 2. Làm sạch và vẽ lại màn hình OLED
    SSD1306_Clear();

    // Dòng 1: Giờ
    SSD1306_SetCursor(0, 0);
    SSD1306_WriteString("Time: ");
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", current_time.hours, current_time.minutes, current_time.seconds);
    SSD1306_SetCursor(40, 0);
    SSD1306_WriteString(buffer);

    // Dòng 2: Ngày tháng
    SSD1306_SetCursor(0, 22);
    SSD1306_WriteString("Date: ");
    snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d", current_time.dayofmonth, current_time.month, current_time.year);
    SSD1306_SetCursor(40, 22);
    SSD1306_WriteString(buffer);

    // Dòng 3: Nhiệt độ
    SSD1306_SetCursor(0, 44);
    SSD1306_WriteString("Temp: ");
    snprintf(buffer, sizeof(buffer), "%.2f", current_temp);
    SSD1306_SetCursor(40, 44);
    SSD1306_WriteString(buffer);

    SSD1306_DrawSmallCircle(40 + 42, 44);
    SSD1306_SetCursor(40 + 48, 44);
    SSD1306_WriteString("C");

    SSD1306_UpdateScreen();
}
