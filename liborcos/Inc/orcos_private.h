/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ORCOS_PRIVATE_H
#define __ORCOS_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32u3xx_hal.h"

#define LOW_SPEED_CRYSTAL_INTERNAL 0
#define LOW_SPEED_CRYSTAL_EXTERNAL 1
#define SELECTED_LOW_SPEED_CRYSTAL LOW_SPEED_CRYSTAL_INTERNAL

#define RTC_ASYNCH_PREDIV    0x7F
#define RTC_SYNCH_PREDIV     0x00F9  /* 32 kHz RC/128 - 1 */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __ORCOS_PRIVATE_H */
