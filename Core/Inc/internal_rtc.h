/*
 * internal_rtc.h
 *
 *  Created on: Jun 12, 2026
 *      Author: Admin
 */

#ifndef INC_INTERNAL_RTC_H_
#define INC_INTERNAL_RTC_H_


#include "main.h"

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t dayofmonth;
    uint8_t month;
    uint16_t year; // Ví dụ: 2026
} Internal_RTC_TimeTypeDef;

/* --- CÁC HÀM CÔNG KHAI --- */

// Khởi tạo liên kết bộ RTC nội
void Internal_RTC_Init(RTC_HandleTypeDef *hrtc);

// Hàm đặt giờ ban đầu (Chỉ cần gọi 1 lần khi cấu hình)
void Internal_RTC_SetTime(Internal_RTC_TimeTypeDef *time);

// Hàm điều hành: Đọc số giây, dịch sang ngày giờ và tự quét OLED liên tục
void Internal_RTC_Loop_Process(void);

void Internal_RTC_GetTime(Internal_RTC_TimeTypeDef *time);

#endif /* INC_INTERNAL_RTC_H_ */
