#include "main.h"
#include "orcos.h"

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
  orcos_init();

  uint16_t keycode = scan_keyboard();
  if (keycode == 49)
  { // RESET with "F" button pressed
    // sharp_string("Waiting ST-LINK", &font_24x40, 10, 0);
    // sharp_send_buffer(120, 40);
    // HAL_Delay(30000); // Delay to connect ST-Link probe
    // sharp_clear();
  }

  uint16_t last_keycode = keycode;

  calc_init();

  while (1)
  {
    uint16_t keycode = 0;

    HAL_Delay(10); // Debouncing delay in ms

    keycode = scan_keyboard();

    if (keycode == 54 && !LCD_is_on() && last_keycode == 0)
    { // Calculator was OFF and the ON button was pressed
      LCD_power_on();
      keycode = 0;
    }
    int ret = 1;

    if (LCD_is_on())
    {

      if (keycode)
      {
        ret = calc_on_key(keycode);
      }

      if (!ret)
      { // OFF command received
        LCD_power_off(1);
      }
    }

    last_keycode = keycode;
#if DEBUG
    // This is required to keep RTT debug messages flowing when in STOP mode :shrug:
    // https://community.st.com/t5/stm32-mcus-products/how-to-get-segger-rtt-to-work-with-sleep-modes-on-stm32l0/m-p/121639
    __HAL_RCC_GPDMA1_CLK_ENABLE();
#endif
    sys_sleep(!LCD_is_on());
  }
}