#include "stm32u3xx.h"
#include "stm32u3xx_hal_rtc_ex.h"
#include "orcos.h"

extern RTC_HandleTypeDef hrtc;

// implement rtc_read()
void rtc_read( 	tm_t * tm, dt_t * dt)
{
    RTC_TimeTypeDef Time;
    RTC_DateTypeDef Date;

    /* Get the RTC calendar time */
    HAL_RTC_GetTime(&hrtc, &Time, RTC_FORMAT_BIN);
    // Do not skip reading the date: this is needed to trigger an update to the shadow registers!
    HAL_RTC_GetDate(&hrtc, &Date, RTC_FORMAT_BIN);

    if (tm != NULL) {
        tm->sec = Time.Seconds;
        tm->min = Time.Minutes;
        tm->hour = Time.Hours;
    }

    if (dt != NULL) {
        dt->day = Date.WeekDay;
        dt->month = Date.Month;
        dt->year = Date.Year;
    }
}