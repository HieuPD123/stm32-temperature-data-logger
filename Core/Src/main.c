/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "oled_ui.h"
#include "ssd1306.h"
#include "ds18b20.h"
#include "fatfs.h"
#include "uart1.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ff.h"
#include <stdio.h>
#include <string.h>
#include "sd_spi_driver.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

RTC_HandleTypeDef hrtc;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim4;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

FATFS fs;
FRESULT fr;
static const char *LOG_FILE_NAME = "log.csv";
char sd_buffer[640] = {0};
uint8_t log_counter = 0;
uint32_t lastSDCheck = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM4_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_RTC_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

void SPI1_SetSpeed(uint32_t prescaler)
{
    // 1. Tắt ngoại vi SPI1 để đảm bảo an toàn
    __HAL_SPI_DISABLE(&hspi1);

    // 2. Cập nhật lại tham số bộ chia (Prescaler) trong cấu trúc dữ liệu HAL
    hspi1.Init.BaudRatePrescaler = prescaler;

    // 3. Gọi hàm HAL_SPI_Init để HAL tự cấu hình lại các thanh ghi theo đúng trình tự chuẩn
    if (HAL_SPI_Init(&hspi1) != HAL_OK)
    {
        Error_Handler();
    }

    // 4. Bật lại ngoại vi SPI1
    __HAL_SPI_ENABLE(&hspi1);
}

void SD_UpdateUsage(void)
{
    FATFS *pfs;
    DWORD fre_clust;

    if(f_getfree("", &fre_clust, &pfs) != FR_OK)
    {
        OLED_SetSD(OLED_SD_ERROR, 0);
        return;
    }

    DWORD total_clust = pfs->n_fatent - 2;

    uint8_t used_pct =
        (uint8_t)
        (((total_clust - fre_clust) * 100)
         / total_clust);

    OLED_SetSD(OLED_SD_OK, used_pct);
}

void SD_LogTemperature(char *bulk_data)
{
    FIL file;
    UINT bw;

    // Mở file (nếu chưa có thì tạo, có rồi thì mở ra)
    if(f_open(&file, LOG_FILE_NAME, FA_OPEN_ALWAYS | FA_WRITE) == FR_OK)
    {
        // Nhảy đến cuối file để ghi tiếp nối đuôi
        f_lseek(&file, f_size(&file));

        // Ghi một phát toàn bộ 10 dòng trong bộ đệm xuống thẻ SD
        f_write(&file, bulk_data, strlen(bulk_data), &bw);

        // Đóng file để giải phóng tài nguyên
        f_close(&file);
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM4_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_FATFS_Init();
  MX_RTC_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */
  float temp = 0.0f;
  UART_Command_t cmd;

  UART1_Init();
  SSD1306_Init();
  OLED_SetTemperature(temp);

    fr = f_mount(&fs, "", 1);

  if(fr == FR_OK)
  {
      // --- FIX LỖI 1: TĂNG TỐC ĐỘ ĐỌC GHI THẺ SD ---
      // Sau khi mount thành công, nâng tốc độ SPI lên để đọc ghi file nhanh gấp 32 lần
      SPI1_SetSpeed(SPI_BAUDRATEPRESCALER_8);
      f_unlink(LOG_FILE_NAME);
      SD_UpdateUsage();
  }
  else
  {
      OLED_SetSD(OLED_SD_ERROR, 0);
  }

  uint32_t lastDisplay = 0; // Biến thời gian cho cập nhật hiển thị/nhiệt độ
  lastSDCheck = HAL_GetTick();

  Update_OLED_Display();
  HAL_TIM_Base_Start(&htim4);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
	      if (HAL_GetTick() - lastDisplay >= 1000)
	      {
	          lastDisplay = HAL_GetTick();

	          temp = DS18B20_GetTemp();
	          OLED_SetTemperature(temp);
	          Update_OLED_Display();

	          RTC_TimeTypeDef sTime;
	          RTC_DateTypeDef sDate;
	          HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	          HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

	          char single_line[64];
	          sprintf(single_line, "20%02d-%02d-%02d %02d:%02d:%02d, %.2f\r\n",
	                  sDate.Year, sDate.Month, sDate.Date,
	                  sTime.Hours, sTime.Minutes, sTime.Seconds, temp);

	          strcat(sd_buffer, single_line);
	          log_counter++;

	          if (log_counter >= 10)
	          {
	              SD_LogTemperature(sd_buffer);

	              memset(sd_buffer, 0, sizeof(sd_buffer));
	              log_counter = 0;
	          }
	      }

	      if (HAL_GetTick() - lastSDCheck >= 300000)
	          {
	              lastSDCheck = HAL_GetTick();

	              if (log_counter < 9)
	              {
	                  SD_UpdateUsage();
	              } else {
	                  lastSDCheck -= 2000;
	              }
	          }

    if (UART1_LineAvailable())
    {
        char *line = UART1_GetLine();

        if (UART1_Parse(line, &cmd))
        {
            switch (cmd.type)
            {
                case CMD_READ:
                {
                    FIL file;
                    char read_buf[128];

                    uint32_t start_line = cmd.value * 10;     // Dòng bắt đầu
                    uint32_t end_line = start_line + 10;      // Dòng kết thúc
                    uint32_t current_line = 0;                // Bộ đếm dòng hiện hành

                    printf("--- START READING LINES %lu TO %lu ---\r\n", start_line, end_line - 1);

                    FRESULT res = f_open(&file, LOG_FILE_NAME, FA_READ);
                                        if (res == FR_OK)
                                        {
                                            while (f_gets(read_buf, sizeof(read_buf), &file) != NULL)
                                            {
                                                if (current_line >= start_line && current_line < end_line)
                                                {
                                                    printf("%s", read_buf);
                                                }
                                                current_line++;
                                                if (current_line >= end_line) break;
                                            }
                                            f_close(&file);

                                            if (current_line <= start_line)  printf("--- END OF FILE ---\r\n");
                                            else                             printf("--- DONE ---\r\n");
                                        }
                                        else
                                        {
                                            // In ra mã lỗi cụ thể (Ví dụ: số 4 là FR_NO_FILE, số 1 là FR_DISK_ERR,...)
                                            printf("Error: Could not open %s. FatFS Code: %d\r\n", LOG_FILE_NAME, res);
                                        }
                                        break;
                                    }

                case CMD_RTC_SET:
                {
                    RTC_TimeTypeDef sTime = {0};
                    RTC_DateTypeDef sDate = {0};

                    if (cmd.year >= 2000) {
                        sDate.Year = cmd.year - 2000;
                    } else {
                        sDate.Year = cmd.year;
                    }
                    sDate.Month = cmd.month;
                    sDate.Date = cmd.day;

                    sTime.Hours = cmd.hour;
                    sTime.Minutes = cmd.minute;
                    sTime.Seconds = cmd.second;

                    HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
                    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

                    printf("System RTC Updated to: 20%02d-%02d-%02d %02d:%02d:%02d\r\n",
                           sDate.Year, sDate.Month, sDate.Date,
                           sTime.Hours, sTime.Minutes, sTime.Seconds);
                    break;
                }

                default:
                    printf("Unknown Command\r\n");
                    break;
            }
        }
        else
        {
            printf("Parse Error: Invalid format\r\n");
        }
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
  hrtc.Init.OutPut = RTC_OUTPUTSOURCE_ALARM;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 71;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 65535;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1|GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
