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
		GPIO_InitStruct.Pull = GPIO_NOPULL; // Use external 1M pull-up to minimise current draw
		GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
				|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_13|GPIO_PIN_14;
		GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
		GPIO_InitStruct.Pull = GPIO_NOPULL; // Use external 1M pull-up to minimise current draw
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
		// Disable interrupts for all other keyboard pins
        HAL_NVIC_DisableIRQ(EXTI0_IRQn);
        HAL_NVIC_DisableIRQ(EXTI1_IRQn);
        HAL_NVIC_DisableIRQ(EXTI2_IRQn);
        HAL_NVIC_DisableIRQ(EXTI3_IRQn);
        HAL_NVIC_DisableIRQ(EXTI4_IRQn);
        HAL_NVIC_DisableIRQ(EXTI5_IRQn);
        HAL_NVIC_DisableIRQ(EXTI13_IRQn);
        HAL_NVIC_DisableIRQ(EXTI14_IRQn);
	} else {
		GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
				|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	}
	// Delay 1 ms for all transitional processes to finish
	// e.g. OD pins with external pull-ups take a long time to change state
    delay_us(100);

	// Go back to STOP mode after interrupt completes
	HAL_PWR_EnableSleepOnExit();
	HAL_SuspendTick();

	HAL_DBGMCU_EnableDBGStopMode();
	DEBUG_PRINT("--- sleep (off = %d)---\n", off);
	HAL_PWR_EnterSTOPMode(PWR_LOWPOWERMODE_STOP2, PWR_STOPENTRY_WFI);
	DEBUG_PRINT("--- wake up --- \n");
	// After wakeup, re-enable all interrupts if they were disabled
    if (off) {
        HAL_NVIC_EnableIRQ(EXTI0_IRQn);
        HAL_NVIC_EnableIRQ(EXTI1_IRQn);
        HAL_NVIC_EnableIRQ(EXTI2_IRQn);
        HAL_NVIC_EnableIRQ(EXTI3_IRQn);
        HAL_NVIC_EnableIRQ(EXTI4_IRQn);
        HAL_NVIC_EnableIRQ(EXTI5_IRQn);
        HAL_NVIC_EnableIRQ(EXTI13_IRQn);
        HAL_NVIC_EnableIRQ(EXTI14_IRQn);
    }
}

/**
 * @brief Performs a system reset by triggering the CPU's reset
 */
void sys_reset(void)
{
    DEBUG_PRINT("System reset requested\n");
    HAL_NVIC_SystemReset();
}
