#ifndef OLED_UI_H
#define OLED_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Trang thai the SD (phia SD bao cho man hinh biet) */
typedef enum {
    OLED_SD_ABSENT = 0,   /* Chua gan / khong co the   -> hien "SD:--"  */
    OLED_SD_OK     = 1,    /* Mount OK, ghi binh thuong -> hien "SD:OK"  */
    OLED_SD_ERROR  = 2     /* The loi                   -> hien "SD:ERR" */
} OLED_SDState_t;

/* -------------------- API cho phia NHIET DO (vi dieu khien / cam bien) ----- */
void OLED_SetTemperature(float temp_c);

/* -------------------- API cho phia THE SD (ban cua ban goi ham nay) --------
 * @param state         OLED_SD_ABSENT / OLED_SD_OK / OLED_SD_ERROR
 * @param free_percent  % dung luong CON TRONG (0..100). Chi co y nghia khi
 *                       state == OLED_SD_OK (dung de ve thanh tien trinh).
 *
 * Vi du phia SD goi sau khi tinh bang FatFs:
 *     OLED_SetSD(OLED_SD_OK, 73);     // the OK, con trong 73%
 *     OLED_SetSD(OLED_SD_ERROR, 0);   // the loi
 *     OLED_SetSD(OLED_SD_ABSENT, 0);  // chua gan the
 * ------------------------------------------------------------------------- */
void OLED_SetSD(OLED_SDState_t state, uint8_t free_percent);

/* -------------------- Ve lai man hinh (goi dinh ky trong while(1)) ---------
 * Ham nay nhe, khong block (chi doc cache + ve). */
void Update_OLED_Display(void);

#ifdef __cplusplus
}
#endif

#endif /* OLED_UI_H */
