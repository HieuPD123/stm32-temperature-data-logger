#ifndef OLED_UI_H
#define OLED_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Trang thai the SD (phia SD bao cho man hinh biet) */
typedef enum {
    OLED_SD_OK     = 0,    /* Mount OK, ghi binh thuong -> hien "OK"   */
    OLED_SD_ERROR  = 1     /* The loi                   -> hien "ERR"  */
} OLED_SDState_t;

/* -------------------- API cho phia NHIET DO (vi dieu khien / cam bien) ----- */
void OLED_SetTemperature(float temp_c);

/* -------------------- API cho phia THE SD (ban cua ban goi ham nay) --------
 * @param state         OLED_SD_OK / OLED_SD_ERROR
 * @param used_percent  % dung luong DA DUNG (0..100). Chi co y nghia khi
 *                      state == OLED_SD_OK (dung de ve thanh tien trinh).
 *
 * Vi du phia SD goi sau khi tinh bang FatFs:
 *     OLED_SetSD(OLED_SD_OK, 27);     // the OK, da dung 27%
 *     OLED_SetSD(OLED_SD_ERROR, 0);   // the loi
 * ------------------------------------------------------------------------- */
void OLED_SetSD(OLED_SDState_t state, uint8_t used_percent);

/* -------------------- Ve lai man hinh (goi dinh ky trong while(1)) ---------
 * Ham nay nhe, khong block (chi doc cache + ve). */
void Update_OLED_Display(void);

#ifdef __cplusplus
}
#endif

#endif /* OLED_UI_H */
