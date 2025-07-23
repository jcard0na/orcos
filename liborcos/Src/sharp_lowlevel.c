/*
 * sharp_lowlevel.c
 *
 * Low-level hardware operations for Sharp Memory LCD
 */

#include "sharp_lowlevel.h"
#include "sharp.h"
#include "pin_definitions.h"
#include "stm32u3xx_hal.h"

extern RTC_HandleTypeDef hrtc;

void LCD_Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}

void TIM1_Init(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 15;
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 65535;
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
    {
        LCD_Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
    {
        LCD_Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
    {
        LCD_Error_Handler();
    }
}

void SPI2_Init(void)
{
    SPI_AutonomousModeConfTypeDef HAL_SPI_AutonomousMode_Cfg_Struct = {0};

    hspi2.Instance = SPI2;
    hspi2.Init.Mode = SPI_MODE_MASTER;
    hspi2.Init.Direction = SPI_DIRECTION_1LINE;
    hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity = SPI_POLARITY_HIGH;
    hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi2.Init.NSS = SPI_NSS_SOFT;
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    hspi2.Init.FirstBit = SPI_FIRSTBIT_LSB;
    hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi2.Init.CRCPolynomial = 0x7;
    hspi2.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    hspi2.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
    hspi2.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
    hspi2.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
    hspi2.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
    hspi2.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
    hspi2.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
    hspi2.Init.IOSwap = SPI_IO_SWAP_DISABLE;
    hspi2.Init.ReadyMasterManagement = SPI_RDY_MASTER_MANAGEMENT_INTERNALLY;
    hspi2.Init.ReadyPolarity = SPI_RDY_POLARITY_HIGH;
    if (HAL_SPI_Init(&hspi2) != HAL_OK)
    {
        LCD_Error_Handler();
    }
    HAL_SPI_AutonomousMode_Cfg_Struct.TriggerState = SPI_AUTO_MODE_DISABLE;
    HAL_SPI_AutonomousMode_Cfg_Struct.TriggerSelection = SPI_GRP1_GPDMA_CH0_TCF_TRG;
    HAL_SPI_AutonomousMode_Cfg_Struct.TriggerPolarity = SPI_TRIG_POLARITY_RISING;
    if (HAL_SPIEx_SetConfigAutonomousMode(&hspi2, &HAL_SPI_AutonomousMode_Cfg_Struct) != HAL_OK)
    {
        LCD_Error_Handler();
    }
}

void __lcd_init()
{
    SPI2_Init();
    TIM1_Init();
    HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 4095, RTC_WAKEUPCLOCK_RTCCLK_DIV8, 0);
    HAL_TIM_Base_Start_IT(&htim1);
}

void LCD_write_line(uint8_t *buf)
{
    buf[0] = 0x1; // Write Line command
    buf[52] = buf[53] = 0;
    GPIO_WRITE(display_cs, GPIO_PIN_SET);
    delay_us(12);
    HAL_SPI_Transmit(&hspi2, buf, LCD_LINE_BUF_SIZE, HAL_MAX_DELAY);
    delay_us(4);
    GPIO_WRITE(display_cs, GPIO_PIN_RESET);
    delay_us(4);
}

void lcd_refresh()
{
#define CHUNK_SIZE_IN_LINES 240
#define CHUNK_BUFFER_SIZE (1 + CHUNK_SIZE_IN_LINES * (1 + LCD_WIDTH / 8 + 1) + 1)
    static uint8_t frame_buffer[CHUNK_BUFFER_SIZE];

    const int total_lines = LCD_HEIGHT;
    const int chunk_size = CHUNK_SIZE_IN_LINES;
    const int num_chunks = (total_lines + chunk_size - 1) / chunk_size;

    uint8_t nop = 0x00;
    GPIO_WRITE(display_cs, GPIO_PIN_SET);
    delay_us(10);
    HAL_SPI_Transmit(&hspi2, &nop, 1, HAL_MAX_DELAY);
    delay_us(10);
    GPIO_WRITE(display_cs, GPIO_PIN_RESET);
    delay_us(10);

    for (int chunk = 0; chunk < num_chunks; chunk++)
    {
        int pos = 0;
        int start_line = chunk * chunk_size;
        int lines_in_chunk = (chunk == num_chunks - 1) ? (total_lines - start_line) : chunk_size;

        frame_buffer[pos++] = 0x01;
        for (int y = 0; y < lines_in_chunk; y++)
        {
            frame_buffer[pos++] = start_line + y + 1;
            memcpy(&frame_buffer[pos], g_framebuffer[start_line + y], LCD_WIDTH / 8);
            pos += LCD_WIDTH / 8;
            frame_buffer[pos++] = 0x00;
        }
        frame_buffer[pos++] = 0x00;

        GPIO_WRITE(display_cs, GPIO_PIN_SET);
        delay_us(12);
        HAL_SPI_Transmit(&hspi2, frame_buffer, pos, HAL_MAX_DELAY);
        delay_us(4);
        GPIO_WRITE(display_cs, GPIO_PIN_RESET);
        delay_us(4);
    }
}

void delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    while (__HAL_TIM_GET_COUNTER(&htim1) < us);
}
