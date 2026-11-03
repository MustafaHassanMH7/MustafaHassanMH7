/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "usb_device.h"
#include "usbd_customhid.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define SBUS_FRAME_SIZE             25U
#define SBUS_DMA_BUF_SIZE           64U
#define HID_JOYSTICK_REPORT_SIZE    15U
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart1_rx;

/* USER CODE BEGIN PV */

extern USBD_HandleTypeDef hUsbDeviceFS;

static uint8_t sbus_frame[SBUS_FRAME_SIZE];
static uint8_t sbus_index = 0;

static uint8_t sbus_flags = 0;
static uint8_t sbus_end_byte = 0;
static uint16_t sbus_channels[16] = {0};
static uint32_t sbus_frames = 0;
static uint32_t sbus_uart_errors = 0;
static uint32_t sbus_last_byte_ms = 0;
static uint32_t sbus_report_ms = 0;
static uint32_t led_toggle_ms = 0;

static char sbus_debug_msg[192];
static uint8_t sbus_dma_buf[SBUS_DMA_BUF_SIZE];
static uint16_t sbus_dma_old_pos = 0U;

static uint8_t hid_report[HID_JOYSTICK_REPORT_SIZE] = {0};
static uint32_t hid_last_send_ms = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

static void SBUS_DecodeFrame(const uint8_t *frame);
static void SBUS_ProcessByte(uint8_t byte);
static void SBUS_ProcessDmaRx(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void SBUS_DecodeFrame(const uint8_t *frame)
{
  uint8_t channel;

  for (channel = 0U; channel < 16U; channel++)
  {
    uint16_t bit_index;
    uint8_t byte_index;
    uint8_t bit_offset;
    uint32_t packed_data;

    /*
     * Each SBUS channel is packed into 11 bits.
     * Channel payload starts at frame[1].
     */
    bit_index = (uint16_t)channel * 11U;
    byte_index = (uint8_t)(1U + (bit_index / 8U));
    bit_offset = (uint8_t)(bit_index % 8U);

    packed_data =
        ((uint32_t)frame[byte_index]) |
        ((uint32_t)frame[byte_index + 1U] << 8U) |
        ((uint32_t)frame[byte_index + 2U] << 16U);

    sbus_channels[channel] =
        (uint16_t)((packed_data >> bit_offset) & 0x07FFU);
  }

  sbus_flags = frame[23];
  sbus_end_byte = frame[24];
}

static void SBUS_ProcessByte(uint8_t byte)
{
  /* Wait for the SBUS frame start byte. */
  if (sbus_index == 0U)
  {
    if (byte != 0x0FU)
    {
      return;
    }
  }

  sbus_frame[sbus_index++] = byte;

  /* A full 25-byte SBUS frame has been received. */
  if (sbus_index >= SBUS_FRAME_SIZE)
  {
    sbus_flags = sbus_frame[23];
    sbus_end_byte = sbus_frame[24];

    if ((sbus_frame[0] == 0x0FU) &&
        (sbus_end_byte == 0x00U))
    {
      SBUS_DecodeFrame(sbus_frame);
      sbus_frames++;
      sbus_last_byte_ms = HAL_GetTick();
    }
    else
    {
      sbus_uart_errors++;
    }

    sbus_index = 0U;
  }
}

static void SBUS_ProcessDmaRx(void)
{
  uint16_t current_pos;

  current_pos = (uint16_t)(
      SBUS_DMA_BUF_SIZE -
      __HAL_DMA_GET_COUNTER(&hdma_usart1_rx)
  );

  if (current_pos == sbus_dma_old_pos)
  {
    return;
  }

  /* DMA has not wrapped around the circular buffer yet. */
  if (current_pos > sbus_dma_old_pos)
  {
    for (uint16_t i = sbus_dma_old_pos; i < current_pos; i++)
    {
      SBUS_ProcessByte(sbus_dma_buf[i]);
    }
  }
  else
  {
    /* DMA wrapped around the end of the circular buffer. */
    for (uint16_t i = sbus_dma_old_pos;
         i < SBUS_DMA_BUF_SIZE;
         i++)
    {
      SBUS_ProcessByte(sbus_dma_buf[i]);
    }

    for (uint16_t i = 0U; i < current_pos; i++)
    {
      SBUS_ProcessByte(sbus_dma_buf[i]);
    }
  }

  sbus_dma_old_pos = current_pos;
}

/* Convert an SBUS channel from about 200-1800 to a HID axis from 0-32767. */
static uint16_t SBUS_To_HID_Axis(uint16_t value)
{
  const int32_t in_min = 200;
  const int32_t in_max = 1800;
  const int32_t out_max = 32767;

  int32_t v = (int32_t)value;

  if (v < in_min)
  {
    v = in_min;
  }
  else if (v > in_max)
  {
    v = in_max;
  }

  return (uint16_t)(((v - in_min) * out_max) / (in_max - in_min));
}

/* Convert a centered channel such as roll/pitch/yaw with a dead zone around 1000. */
static uint16_t SBUS_To_HID_Axis_Centered(uint16_t value)
{
  const int32_t in_min = 200;
  const int32_t in_center = 1000;
  const int32_t in_max = 1800;
  const int32_t deadzone = 25;

  int32_t v = (int32_t)value;

  if ((v > (in_center - deadzone)) &&
      (v < (in_center + deadzone)))
  {
    v = in_center;
  }

  if (v < in_min)
  {
    v = in_min;
  }
  else if (v > in_max)
  {
    v = in_max;
  }

  return (uint16_t)(((v - in_min) * 32767) / (in_max - in_min));
}

/* Store a 16-bit value in the HID report using little-endian byte order. */
static void HID_Put_U16_LE(uint8_t *buf, uint8_t index, uint16_t value)
{
  buf[index] = (uint8_t)(value & 0xFFU);
  buf[index + 1U] = (uint8_t)((value >> 8) & 0xFFU);
}

/* Convert the three-position CH5 switch into buttons. */
static uint8_t CH5_To_Buttons(uint16_t ch5)
{
  uint8_t buttons = 0x00U;

  /*
   * CH5 switch C:
   * Up     ≈ 200   -> Button 1
   * Middle ≈ 1000  -> Button 2
   * Down   ≈ 1800  -> Button 3
   */

  if (ch5 < 600U)
  {
    buttons |= (1U << 0);   /* Button 1 */
  }
  else if (ch5 < 1400U)
  {
    buttons |= (1U << 1);   /* Button 2 */
  }
  else
  {
    buttons |= (1U << 2);   /* Button 3 */
  }

  return buttons;
}

/* Build and send the USB HID joystick report. */
static void HID_Send_Joystick_Report(void)
{
  uint16_t axis_x;
  uint16_t axis_y;
  uint16_t axis_z;
  uint16_t axis_rx;
  uint16_t axis_ry;
  uint16_t axis_rz;
  uint16_t axis_slider;
  uint8_t buttons;

  /*
   * HID report layout - total 15 bytes:
   *
   * Byte 0-1   X       CH1 Roll
   * Byte 2-3   Y       CH2 Pitch
   * Byte 4-5   Z       CH3 Throttle
   * Byte 6-7   Rx      CH6 left knob
   * Byte 8-9   Ry      CH7 rear lever
   * Byte 10-11 Rz      CH4 Yaw
   * Byte 12-13 Slider  CH8 right knob
   * Byte 14    Buttons CH5 switch
   */

  axis_x      = SBUS_To_HID_Axis_Centered(sbus_channels[0]);  /* CH1 */
  axis_y      = SBUS_To_HID_Axis_Centered(sbus_channels[1]);  /* CH2 */
  axis_z      = SBUS_To_HID_Axis(sbus_channels[2]);           /* CH3 */
  axis_rx     = SBUS_To_HID_Axis(sbus_channels[5]);           /* CH6 */
  axis_ry     = SBUS_To_HID_Axis(sbus_channels[6]);           /* CH7 */
  axis_rz     = SBUS_To_HID_Axis_Centered(sbus_channels[3]);  /* CH4 */
  axis_slider = SBUS_To_HID_Axis(sbus_channels[7]);           /* CH8 */
  buttons     = CH5_To_Buttons(sbus_channels[4]);             /* CH5 */

  HID_Put_U16_LE(hid_report, 0U,  axis_x);
  HID_Put_U16_LE(hid_report, 2U,  axis_y);
  HID_Put_U16_LE(hid_report, 4U,  axis_z);
  HID_Put_U16_LE(hid_report, 6U,  axis_rx);
  HID_Put_U16_LE(hid_report, 8U,  axis_ry);
  HID_Put_U16_LE(hid_report, 10U, axis_rz);
  HID_Put_U16_LE(hid_report, 12U, axis_slider);

  hid_report[14] = buttons;

  (void)USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS,
                                   hid_report,
                                   HID_JOYSTICK_REPORT_SIZE);
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

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */

  sbus_dma_old_pos = 0U;
  sbus_index = 0U;

  if (HAL_UART_Receive_DMA(&huart1,
                           sbus_dma_buf,
                           SBUS_DMA_BUF_SIZE) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /*
     * Process all new bytes written by DMA.
     * inside the circular buffer.
     */
    SBUS_ProcessDmaRx();

    {
      uint32_t now = HAL_GetTick();

      /*
       * Send a USB HID joystick report about every 10 ms
       * only when recent SBUS data is available.
       */
      if (((now - hid_last_send_ms) >= 10U) &&
          (sbus_last_byte_ms > 0U) &&
          ((now - sbus_last_byte_ms) <= 100U))
      {
        hid_last_send_ms = now;
        HID_Send_Joystick_Report();
      }

      /*
       * Keep LD2 blinking to show that the firmware is running.
       */
      if ((now - led_toggle_ms) >= 500U)
      {
        led_toggle_ms = now;
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
      }

      /*
       * Send one debug line per second through USART2 to Tera Term.
       */
      if ((now - sbus_report_ms) >= 1000U)
      {
        int message_length;
        uint16_t transmit_length;

        sbus_report_ms = now;

        if (sbus_frames > 0U)
        {
          message_length = snprintf(
              sbus_debug_msg,
              sizeof(sbus_debug_msg),
              "fps=%lu err=%lu | "
              "CH1=%4u CH2=%4u CH3=%4u CH4=%4u "
              "CH5=%4u CH6=%4u CH7=%4u CH8=%4u | "
              "flags=0x%02X end=0x%02X\r\n",
              (unsigned long)sbus_frames,
              (unsigned long)sbus_uart_errors,
              (unsigned int)sbus_channels[0],
              (unsigned int)sbus_channels[1],
              (unsigned int)sbus_channels[2],
              (unsigned int)sbus_channels[3],
              (unsigned int)sbus_channels[4],
              (unsigned int)sbus_channels[5],
              (unsigned int)sbus_channels[6],
              (unsigned int)sbus_channels[7],
              (unsigned int)sbus_flags,
              (unsigned int)sbus_end_byte);
        }
        else
        {
          message_length = snprintf(
              sbus_debug_msg,
              sizeof(sbus_debug_msg),
              "NO SBUS frames errors=%lu\r\n",
              (unsigned long)sbus_uart_errors);
        }

        if (message_length > 0)
        {
          if (message_length >= (int)sizeof(sbus_debug_msg))
          {
            transmit_length =
                (uint16_t)(sizeof(sbus_debug_msg) - 1U);
          }
          else
          {
            transmit_length = (uint16_t)message_length;
          }

          HAL_UART_Transmit(
              &huart2,
              (uint8_t *)sbus_debug_msg,
              transmit_length,
              100U);
        }

        /*
         * Reset only the one-second report counters.
         * The decoded channel values remain stored.
         */
        sbus_frames = 0U;
        sbus_uart_errors = 0U;
      }
    }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB|RCC_PERIPHCLK_USART1
                              |RCC_PERIPHCLK_USART2;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.USBClockSelection = RCC_USBCLKSOURCE_PLL;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
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
  huart1.Init.BaudRate = 100000;
  huart1.Init.WordLength = UART_WORDLENGTH_9B;
  huart1.Init.StopBits = UART_STOPBITS_2;
  huart1.Init.Parity = UART_PARITY_EVEN;
  huart1.Init.Mode = UART_MODE_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_RXINVERT_INIT;
  huart1.AdvancedInit.RxPinLevelInvert = UART_ADVFEATURE_RXINV_ENABLE;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_RXOVERRUNDISABLE_INIT|UART_ADVFEATURE_DMADISABLEONERROR_INIT;
  huart2.AdvancedInit.OverrunDisable = UART_ADVFEATURE_OVERRUN_DISABLE;
  huart2.AdvancedInit.DMADisableonRxError = UART_ADVFEATURE_DMA_DISABLEONRXERROR;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

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
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

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
#ifdef USE_FULL_ASSERT
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
