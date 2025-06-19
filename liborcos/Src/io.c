#include "stm32u3xx_hal.h"
#include "io.h"
#include <stm32u3xx_ll_adc.h>
#include "orcos.h"

void UpdateSysTick(uint32_t new_HCLK_freq) {
    HAL_SYSTICK_Config(new_HCLK_freq / 1000);  // Ensure 1 ms SysTick tick
}

int get_vbat() {

	// Datasheet Section 3.20.2
	// Internal voltage reference (VREFINT)
	// The VREFINT provides a stable (bandgap) voltage output for the ADC and the comparators. The VREFINT is
	// internally connected to ADC1 and ADC2 input channels.
	// The precise voltage of VREFINT is individually measured for each part by STMicroelectronics during production
	// test and stored in the system memory area. It is accessible in read-only mode.` 
	if (HAL_ADC_Start(&hadc1) == HAL_OK) {
		if (HAL_ADC_PollForConversion(&hadc1,100) == HAL_OK) {
			// Read the V_REFINT ADC channel.  Should be close to the one stored during
 			// calibration in VREF_CAL_ADDRESS
			// Assumes: hadc1 has been correcly configured to read from ADC_CHANNEL_VREFINT
			// with a minimum conversion time of 12.65 us, as specified in Table 30 of datasheet
			// If we configure the ADC to run at 1MHz the ADC needs to be configure to sample
			// for at least 12.65 cycles.  The closest configuration is ADC_SAMPLETIME_23CYCLES_5
			uint16_t ADC_measure_VREF = HAL_ADC_GetValue(&hadc1);
			// Read the VREF value that was stored after initial calibration,
			// during manufacturing.
			uint16_t ADC_cal_value = (*VREFINT_CAL_ADDR);
			uint16_t VREF_VOLTAGE_AT_CALIBRATION = 3000;
			uint16_t VREF_VOLTAGE = (VREF_VOLTAGE_AT_CALIBRATION*ADC_cal_value)/ADC_measure_VREF;
			return VREF_VOLTAGE;
		}
	}
	return 1;
}