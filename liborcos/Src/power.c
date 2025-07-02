#include "stm32u3xx_hal.h"
#include "pin_definitions.h"
#include "orcos.h"
#if DEBUG
#include "SEGGER_RTT.h"
#endif

static void delay_us(uint16_t us)
{
	/* Assuming 16MHz clock */
	uint32_t delay = us * (SystemCoreClock / 1000000);
	while (delay--)
	{
		__NOP(); // No operation instruction
	}
}

void sys_sleep(int off)
{
	if (off)
	{

		// First set all rows to input and disable the internal pull-up
		// The high-valued external pull-ups are sufficient to
		// trigger interrupts and should draw less current.
		GPIO_INIT_ARRAY(row_pin_array,
						GPIO_MODE_INPUT,
						GPIO_NOPULL,
						GPIO_SPEED_FREQ_LOW);

		// Second, set the row corresponding to ON/OFF key to ext. interrupt mode
		// (This is the last row in the matrix)
		GPIO_INIT_SINGLE(row_pin_array[row_pin_array_count - 1],
						 GPIO_MODE_IT_RISING_FALLING,
						 GPIO_NOPULL,
						 GPIO_SPEED_FREQ_LOW);

		// Third, disable interrupts for all other keyboard pins
		HAL_NVIC_DisableIRQ(EXTI0_IRQn);
		HAL_NVIC_DisableIRQ(EXTI1_IRQn);
		HAL_NVIC_DisableIRQ(EXTI2_IRQn);
		HAL_NVIC_DisableIRQ(EXTI3_IRQn);
		HAL_NVIC_DisableIRQ(EXTI4_IRQn);
		HAL_NVIC_DisableIRQ(EXTI5_IRQn);
		HAL_NVIC_DisableIRQ(EXTI13_IRQn);
		HAL_NVIC_DisableIRQ(EXTI14_IRQn);
	}
	else
	{
		// Set all rows to ext. interrupt mode
		GPIO_INIT_ARRAY(row_pin_array,
						GPIO_MODE_IT_RISING_FALLING,
						GPIO_NOPULL,
						GPIO_SPEED_FREQ_LOW);
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
	// Restore interrupts for all other keyboard pins
	HAL_NVIC_EnableIRQ(EXTI0_IRQn);
	HAL_NVIC_EnableIRQ(EXTI1_IRQn);
	HAL_NVIC_EnableIRQ(EXTI2_IRQn);
	HAL_NVIC_EnableIRQ(EXTI3_IRQn);
	HAL_NVIC_EnableIRQ(EXTI4_IRQn);
	HAL_NVIC_EnableIRQ(EXTI5_IRQn);
	HAL_NVIC_EnableIRQ(EXTI13_IRQn);
	HAL_NVIC_EnableIRQ(EXTI14_IRQn);

	// Re-enable the internal pull ups on all rows to make keyboard scanning
	// more responsive
	GPIO_INIT_ARRAY(row_pin_array,
					GPIO_MODE_IT_RISING_FALLING,
					GPIO_NOPULL,
					GPIO_SPEED_FREQ_LOW);
}

/**
 * @brief Performs a system reset by triggering the CPU's reset
 */
void sys_reset(void)
{
	DEBUG_PRINT("System reset requested\n");
	HAL_NVIC_SystemReset();
}