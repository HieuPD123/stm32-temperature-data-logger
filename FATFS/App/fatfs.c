/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file   fatfs.c
  * @brief  Code for fatfs applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
#include "fatfs.h"
#include "stm32f1xx_hal.h"
uint8_t retUSER;    /* Return value for USER */
char USERPath[4];   /* USER logical drive path */
FATFS USERFatFS;    /* File system object for USER logical drive */
FIL USERFile;       /* File object for USER */

/* USER CODE BEGIN Variables */
extern RTC_HandleTypeDef hrtc;
/* USER CODE END Variables */

void MX_FATFS_Init(void)
{
  /*## FatFS: Link the USER driver ###########################*/
  retUSER = FATFS_LinkDriver(&USER_Driver, USERPath);

  /* USER CODE BEGIN Init */
  /* additional user code for init */
  /* USER CODE END Init */
}

/**
  * @brief  Gets Time from RTC
  * @param  None
  * @retval Time in DWORD
  */
DWORD get_fattime(void)
	{
	  /* USER CODE BEGIN get_fattime */
	  RTC_TimeTypeDef sTime;
	  RTC_DateTypeDef sDate;

	  // Lấy thời gian từ phần cứng RTC
	  // Lưu ý: Phải lấy Time trước, Date sau theo đúng document của HAL
	  HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	  HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

	  // Tính toán năm: FatFs lấy mốc từ 1980.
	  // RTC của STM32 lưu số 0 - 99 tương ứng năm 2000 - 2099.
	  // Vậy Năm_FatFs = (Năm_RTC + 2000) - 1980 = Năm_RTC + 20.
	  DWORD fat_time = 0;

	  fat_time |= ((DWORD)(sDate.Year + 20) << 25); // Năm
	  fat_time |= ((DWORD)sDate.Month << 21);       // Tháng
	  fat_time |= ((DWORD)sDate.Date << 16);        // Ngày
	  fat_time |= ((DWORD)sTime.Hours << 11);       // Giờ
	  fat_time |= ((DWORD)sTime.Minutes << 5);      // Phút
	  fat_time |= ((DWORD)(sTime.Seconds / 2));     // Giây (FatFs yêu cầu giây chia 2)

	  return fat_time;
	  /* USER CODE END get_fattime */
	}


/* USER CODE BEGIN Application */

/* USER CODE END Application */
