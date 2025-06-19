#include "stm32u3xx_hal.h"
#include "orcos.h"
#if DEBUG
#include "SEGGER_RTT.h"
#endif

static void delay_us(uint16_t us) {
    /* Assuming 16MHz clock */
    uint32_t delay = us * (SystemCoreClock / 1000000);
    while(delay--) {
        __NOP(); // No operation instruction
    }
}

void sys_sleep(int off) {
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	if (off) {
		// Only set the row pin corresponding to ON/OFF key to ext. interrupt mode
		GPIO_InitStruct.Pin = GPIO_PIN_15;
		GPIO_InitStruct.Pull = GPIO_PULLUP; // Use external 1M pull-up to minimise current draw
		GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
				|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_13|GPIO_PIN_14;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		GPIO_InitStruct.Pull = GPIO_PULLUP; // Use external 1M pull-up to minimise current draw
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	} else {
		GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
				|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
		GPIO_InitStruct.Pull = GPIO_PULLUP;
		GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	}
	// Delay 1 ms for all transitional processes to finish
	// e.g. OD pins with external pull-ups take a long time to change state
    delay_us(100);

	// Go back to STOP mode after interrupt completes
	HAL_PWR_EnableSleepOnExit();
	HAL_SuspendTick();

    // /* Disable all used wakeup source */
	// extern RTC_HandleTypeDef hrtc;
    // HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
    // /* Clear all related wakeup flags */
    // __HAL_PWR_CLEAR_FLAG(PWR_FLAG_STOPF);

    // /* Re-enable wakeup source */
    // /* ## Setting the Wake up time ############################################*/
    // /* RTC Wakeup Interrupt Generation:
    //   (2047 + 1) × (16 / 32768) = 1.000 seconds
    // */
    // HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 2047, RTC_WAKEUPCLOCK_RTCCLK_DIV16, 0);
	HAL_DBGMCU_EnableDBGStopMode();
	DEBUG_PRINT("--- sleep (off = %d)---\n", off);
	HAL_PWR_EnterSTOPMode(PWR_LOWPOWERMODE_STOP2, PWR_STOPENTRY_WFI);
	DEBUG_PRINT("--- wake up --- \n");
}