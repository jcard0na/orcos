/*
 * sharp.c
 *
 * Functions to work with Sharp Memory LCD of type LS027B7DH01.
 *
 * The display in OpenRPNCalc is installed upside down for convenience of PCB routing,
 * so this has to be taken into account in the firmware.
 *
 *  Created on: Mar 9, 2021
 *      Author: apolu
 */

#include "stm32u3xx_hal.h"
#include "fonts.h"
#include "sharp.h"
#include "orcos.h"
#include "stm32u3xx_hal_rtc_ex.h"

#if DEBUG
#include "SEGGER_RTT.h"
#endif

extern RTC_HandleTypeDef hrtc;
TIM_HandleTypeDef htim1;
SPI_HandleTypeDef hspi2;
LPTIM_HandleTypeDef hlptim1;

// 1bpp packed buffer (400x240 / 8 = 12,000 bytes)
static uint8_t g_framebuffer[LCD_HEIGHT][LCD_WIDTH / 8];
extern int off;
static int timeout_counter = 0;
#define OFF_TIMEOUT (5 * 60) // 5 min timeout before switching off

static void LCD_Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}


static void TIM1_Init(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 15; // 16MHz/(15+1) = 1MHz → 1µs per tick (ideal for delay_us)
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

static void SPI2_Init(void)
{

    SPI_AutonomousModeConfTypeDef HAL_SPI_AutonomousMode_Cfg_Struct = {0};

    /* SPI2 parameter configuration*/
    hspi2.Instance = SPI2;
    hspi2.Init.Mode = SPI_MODE_MASTER;
    hspi2.Init.Direction = SPI_DIRECTION_1LINE;
    hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity = SPI_POLARITY_HIGH;
    hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi2.Init.NSS = SPI_NSS_SOFT;
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
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

/* String buffer (maximum 64 pixels high) */
uint8_t buffer[BUFFER_SIZE];

/* Microsecond delay */
void delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim1, 0); // set the counter value a 0
    while (__HAL_TIM_GET_COUNTER(&htim1) < us)
        ; // wait for the counter to reach the us input in the parameter
}

void lcd_keep_alive()
{
    timeout_counter = 0;
}

/* Clear display */
void sharp_clear()
{
    uint8_t b[2] = {0x04, 0x00};
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
    delay_us(12);
    HAL_SPI_Transmit(&hspi2, b, 2, 100);
    delay_us(4);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
    delay_us(4);
}

/* Send the string buffer content to the display via SPI interface.
 *   y : vertical coordinate of the top pixel of the string (0...239)
 *   lines : number of lines to send from the buffer (up to 64)
 */
void sharp_send_buffer(uint16_t y, uint16_t lines)
{
    return;
    buffer[0] = 0x01; // Write line command
    uint16_t size = (3 + lines * 52);

    // Set line numbers
    for (int j = 0; j < lines; j++)
    {
        if (y + j < 240)
            buffer[j * 52 + 1] = (uint8_t)(y + j + 1);
    }

    // Dump buffer contents
    DEBUG_PRINT("\n--- sharp_send_buffer(y=%u, lines=%u) ---\n", y, lines);
    DEBUG_PRINT("Cmd: 0x%02X\n", buffer[0]);

    for (int j = 0; j < lines; j++)
    {
        int offset __attribute__((unused)) = j * 52;
        DEBUG_PRINT("Line %u: ", buffer[offset + 1]);

        // Print first few bytes of each line
        for (int b = 0; b < 8; b++)
        {
            DEBUG_PRINT("%02X ", buffer[offset + 2 + b]);
        }
        DEBUG_PRINT("...\n");
    }

    // Send to display
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
    delay_us(12);
    HAL_SPI_Transmit(&hspi2, buffer, size, 100);
    delay_us(4);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
    delay_us(4);
}

void sharp_clear_buffer(uint16_t lines, unsigned char value)
{
    size_t size = (2 + lines * 52);
    memset(buffer, value, size);
}

void sharp_invert_buffer(uint16_t lines)
{
    size_t size = (2 + lines * 52);
    for (size_t i = 0; i < size; i++)
    {
        buffer[i] ^= 0xFF;
    }
}

/* Draw string in the string buffer (without sending it to screen).
 *   str : string to be drawn
 *   font: pointer to the font structure
 *   dx: x coordinate of the start of the string
 *   dy: y coordinate of the start of the string in the buffer
 *       (keep in mind that the buffer itself can be drawn at the arbitrary
 *       location later in the call to sharp_send_buffer(). )
 *   */
void sharp_string(char *str, FontDef_t *font, uint16_t dx, uint16_t dy)
{
    uint16_t width = font->FontWidth;
    uint16_t height = font->FontHeight;
    uint16_t bytes = (width + 7) / 8;
    const char (*data)[bytes * height] = font->data;
    uint16_t xpos = dx;
    uint16_t dy52 = dy * 52;
    int i = 0;

    union
    {
        uint16_t word;
        uint8_t byte[2];
    } split_word;

    if (height + dy > BUFFER_LINES)
        height = BUFFER_LINES - dy;

    while (xpos < 400)
    {
        uint8_t c = str[i];
        if (c == 0x00)
            break;
        const char *cdata = data[c];
        uint16_t xaddr = xpos >> 3;
        uint16_t xshift = xpos & 0x0007;
        uint16_t jdy52 = 2 + dy52;
        uint16_t faddr = 0;
        for (uint16_t j = 0; j < height; j++)
        {
            uint16_t xaddrb = jdy52 + xaddr;
            jdy52 += 52;
            for (uint16_t b = 0; b < bytes; b++)
            {
                split_word.word = (uint8_t)cdata[faddr];
                split_word.word <<= xshift;
                if (xaddrb < jdy52)
                    buffer[xaddrb] &= ~split_word.byte[0];
                xaddrb++;
                if (xaddrb < jdy52)
                    buffer[xaddrb] &= ~split_word.byte[1];
                faddr++;
            }
        }
        xpos += width;
        i++;
    }
}

void sharp_char(uint8_t c, FontDef_t *font, uint16_t dx, uint16_t dy)
{
    uint16_t width = font->FontWidth;
    uint16_t height = font->FontHeight;
    uint16_t bytes = (width + 7) / 8;
    const char (*data)[bytes * height] = font->data;
    uint16_t dy52 = dy * 52;
    union
    {
        uint16_t word;
        uint8_t byte[2];
    } split_word;

    if (height + dy > BUFFER_LINES)
        height = BUFFER_LINES - dy;
    const char *cdata = data[c];
    uint16_t xaddr = dx >> 3;
    uint16_t xshift = dx & 0x0007;
    uint16_t jdy52 = 2 + dy52;
    uint16_t faddr = 0;
    for (uint16_t j = 0; j < height; j++)
    {
        uint16_t xaddrb = jdy52 + xaddr;
        jdy52 += 52;
        for (uint16_t b = 0; b < bytes; b++)
        {
            split_word.word = (uint8_t)cdata[faddr];
            split_word.word <<= xshift;
            if (xaddrb < jdy52)
                buffer[xaddrb] &= ~split_word.byte[0];
            xaddrb++;
            if (xaddrb < jdy52)
                buffer[xaddrb] &= ~split_word.byte[1];
            faddr++;
        }
    }
}

void sharp_string_fast_16x26(char *str, uint8_t col, uint8_t dy)
{
    FontDef_t *font = &font_16x26;
    uint8_t height = font->FontHeight;
    const char (*data)[2 * height] = font->data;
    uint8_t c = col;
    int i = 0;
    if (height + dy > BUFFER_LINES)
        height = BUFFER_LINES - dy;
    uint16_t dy52 = dy * 52;
    while (c < 50)
    {
        uint8_t ch = str[i];
        if (ch == 0x00)
            break;
        const char *cdata = data[ch];
        uint16_t xaddr = dy52 + 2 + c;
        uint8_t j2 = 0;
        for (uint8_t j = 0; j < height; j++)
        {
            buffer[xaddr] = ~cdata[j2++];
            buffer[xaddr + 1] = ~cdata[j2++];
            xaddr += 52;
        }
        i++;
        c += 2;
    }
}

void sharp_char_fast_16x26(uint8_t ch, uint8_t col, uint8_t dy)
{
    FontDef_t *font = &font_16x26;
    uint8_t height = font->FontHeight;
    const char (*data)[2 * height] = font->data;
    if (height + dy > BUFFER_LINES)
        height = BUFFER_LINES - dy;
    const char *cdata = data[ch];
    uint16_t xaddr = dy * 52 + 2 + col;
    uint8_t j2 = 0;
    for (uint8_t j = 0; j < height; j++)
    {
        buffer[xaddr] = ~cdata[j2++];
        buffer[xaddr + 1] = ~cdata[j2++];
        xaddr += 52;
    }
}

void sharp_string_fast_24x40(char *str, uint8_t col, uint8_t dy)
{
    FontDef_t *font = &font_24x40;
    uint8_t height = font->FontHeight;
    const char (*data)[3 * height] = font->data;
    uint8_t c = col;
    int i = 0;
    if (height + dy > BUFFER_LINES)
        height = BUFFER_LINES - dy;
    uint16_t dy52 = dy * 52;
    while (c < 50)
    {
        uint8_t ch = str[i];
        if (ch == 0x00)
            break;
        const char *cdata = data[ch];
        uint16_t xaddr = dy52 + 2 + c;
        uint8_t j3 = 0;
        for (uint8_t j = 0; j < height; j++)
        {
            buffer[xaddr] = ~cdata[j3++];
            buffer[xaddr + 1] = ~cdata[j3++];
            buffer[xaddr + 2] = ~cdata[j3++];
            xaddr += 52;
        }
        i++;
        c += 3;
    }
}

void sharp_char_fast_24x40(uint8_t ch, uint8_t col, uint8_t dy)
{
    FontDef_t *font = &font_24x40;
    uint8_t height = font->FontHeight;
    const char (*data)[3 * height] = font->data;
    if (height + dy > BUFFER_LINES)
        height = BUFFER_LINES - dy;
    const char *cdata = data[ch];
    uint16_t xaddr = dy * 52 + 2 + col;
    uint8_t j3 = 0;
    for (uint8_t j = 0; j < height; j++)
    {
        buffer[xaddr] = ~cdata[j3++];
        buffer[xaddr + 1] = ~cdata[j3++];
        buffer[xaddr + 2] = ~cdata[j3++];
        xaddr += 52;
    }
}

// Draws a filled rectangle in a monochrome buffer.
// Parameters:
// - dx: X-coordinate of the top-left corner of the rectangle.
// - dy: Y-coordinate of the top-left corner of the rectangle.
// - width: Width of the rectangle in pixels.
// - height: Height of the rectangle in pixels.
// - color: 1 for white, 0 for black, any other value for invert
void sharp_filled_rectangle(size_t dx, size_t dy, size_t width, size_t height,
                            uint8_t color)
{
    int x = dx;
    int y = dy;
    if (width == 0 || height == 0)
        return;

    // Limit the rectangle to the buffer boundaries
    size_t max_x = x + width + 1 > BUFFER_WIDTH ? BUFFER_WIDTH : x + width + 1;
    size_t max_y = y + height > BUFFER_LINES ? BUFFER_LINES : y + height;

    size_t start_offset = x % 8;
    size_t end_offset = max_x % 8;

    for (size_t row = y; row < max_y; row++)
    {
        size_t start_byte = row * 52 + x / 8 + 2;
        size_t end_byte = row * 52 + (max_x - 1) / 8 + 2;

        // Handle the first partial byte if necessary
        if (start_byte == end_byte)
        {
            uint8_t mask = (((uint8_t)0xFF << start_offset) & ((uint8_t)0xFF >> (8 - end_offset)));
            if (color == 0)
            {
                buffer[start_byte] &= ~mask;
            }
            else if (color == 1)
            {
                buffer[start_byte] |= mask;
            }
            else
            {
                buffer[start_byte] ^= mask;
            }
        }
        else
        {
            if (start_offset != 0)
            {
                uint8_t start_mask = ((uint8_t)0xFF << start_offset);
                if (color == 0)
                {
                    buffer[start_byte] &= ~start_mask;
                }
                else if (color == 1)
                {
                    buffer[start_byte] |= start_mask;
                }
                else
                {
                    buffer[start_byte] ^= start_mask;
                }
                start_byte++;
            }

            // Fill whole bytes in between
            for (size_t byte = start_byte; byte < end_byte; byte++)
            {
                buffer[byte] = color ? 0xFF : 0x00;
            }

            // Handle the last partial byte if necessary
            if (end_offset != 0)
            {
                uint8_t end_mask = (0xFF >> (8 - end_offset));
                if (color == 0)
                {
                    buffer[end_byte] &= ~end_mask;
                }
                else if (color == 1)
                {
                    buffer[end_byte] |= end_mask;
                }
                else
                {
                    buffer[end_byte] ^= end_mask;
                }
            }
        }
    }
}

void sharp_test_font(FontDef_t *font, char start_symbol)
{
    int w, h;
    if (font == NULL)
    {
        w = 16;
        h = 26;
    }
    else
    {
        w = font->FontWidth;
        h = font->FontHeight;
    }
    int rows = 240 / h;
    int columns = 400 / w;
    char str[67];
    char symbol = start_symbol;
    sharp_clear();
    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < columns; c++)
        {
            str[c] = (symbol == 0 ? ' ' : symbol);
            symbol++;
        }
        str[columns] = 0x00;
        memset(buffer, 0xFF, 2 + h * 52);
        if (font)
            sharp_string(str, font, 0, 0);
        else
            sharp_string_fast_16x26(str, 0, 0);
        sharp_send_buffer(r * h, h);
    }
}

void WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
    // DEBUG_PRINT("-- LPTIM1: EXTIN toggle ---\n");
    RTC_TimeTypeDef Time;
    RTC_DateTypeDef Date;
    /* Get the RTC calendar time */
    HAL_RTC_GetTime(hrtc, &Time, RTC_FORMAT_BIN);
    // Do not skip reading the date: This triggers an update to the shadow registers!
    HAL_RTC_GetDate(hrtc, &Date, RTC_FORMAT_BIN);  
    SEGGER_RTT_printf(0, "wake up timer event %02d:%02d! (%d)\n", Time.Minutes, Time.Seconds, timeout_counter);
    if (!off)
    {
        timeout_counter++;
        if (timeout_counter > OFF_TIMEOUT)
        {
            LCD_power_off(1);
            off = 1;
        }

        // DEBUG_PRINT("\n--- EXTIN toggle ---\n");
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_9); // Toggle LCD refresh signal (EXTIN)
    }
}

void LCD_power_on()
{
    DEBUG_PRINT("\n--- LDC_power_on() ---\n");
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET); // 5V booster enable
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET); // DISP signal to "ON"
    HAL_TIM_Base_Start_IT(&htim1);
    /* Configure wakeup interrupt */
    /* RTC Wakeup Interrupt Generation:
      (2047 + 1) × (16 / 32768) = 1.000 seconds
    */
    if (HAL_RTC_RegisterCallback(&hrtc, HAL_RTC_WAKEUPTIMER_EVENT_CB_ID, WakeUpTimerEventCallback) != HAL_OK) {
        LCD_Error_Handler();
    }
    HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 2047, RTC_WAKEUPCLOCK_RTCCLK_DIV16, 0);
}

void LCD_power_off(int clear)
{
    DEBUG_PRINT("\n--- LDC_power_off() ---\n");
    // XXX: this prevents waking up form STOP2
    // HAL_TIM_Base_Stop_IT(&htim1); // Stop the timer
    delay_us(30);
    if (clear)
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET); // DISP signal to "OFF"
    delay_us(30);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);  // EXTCOMIN signal of "OFF"
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); // 5V booster disable
    HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
}

/* Send the entire frame buffer content to the display via SPI interface.
 * Uses a configurable number of SPI transfers in case we need to reduce RAM usage.
 * Reference: https://www.embeddedartists.com/wp-content/uploads/2018/06/Memory_LCD_Programming.pdf
 */
void lcd_refresh()
{
// This should be a divisor of 240.  240 means a single transfer, which would mean fastest
// speed but highest RAM usage
#define CHUNK_SIZE_IN_LINES 240

// Calculate buffer size based on chunk size
#define CHUNK_BUFFER_SIZE (1 + CHUNK_SIZE_IN_LINES * (1 + LCD_WIDTH / 8 + 1) + 1)
    static uint8_t frame_buffer[CHUNK_BUFFER_SIZE];

    const int total_lines = LCD_HEIGHT;
    const int chunk_size = CHUNK_SIZE_IN_LINES;
    const int num_chunks = (total_lines + chunk_size - 1) / chunk_size; // Round up

    // Send NOP command to initialize interface
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
    delay_us(10);
    uint8_t nop = 0x00;
    HAL_SPI_Transmit(&hspi2, &nop, 1, HAL_MAX_DELAY);
    delay_us(10);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
    delay_us(10);

    for (int chunk = 0; chunk < num_chunks; chunk++)
    {
        int pos = 0;
        int start_line = chunk * chunk_size;
        int lines_in_chunk = (chunk == num_chunks - 1) ? (total_lines - start_line) : chunk_size;

        // Command byte
        frame_buffer[pos++] = 0x01; // Write command

        // Fill chunk data
        for (int y = 0; y < lines_in_chunk; y++)
        {
            frame_buffer[pos++] = start_line + y + 1; // 1-based line number
            memcpy(&frame_buffer[pos], g_framebuffer[start_line + y], LCD_WIDTH / 8);
            pos += LCD_WIDTH / 8;
            frame_buffer[pos++] = 0x00; // Line trailer
        }

        // Chunk trailer
        frame_buffer[pos++] = 0x00;

        // Send chunk
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
        delay_us(12);
        HAL_SPI_Transmit(&hspi2, frame_buffer, pos, HAL_MAX_DELAY);
        delay_us(4);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
        delay_us(4);
    }
}

/**
 * Draws a 1bpp image to the LCD framebuffer
 * @param img   Pointer to 1bpp bitmap data (row-major, packed LSB-first)
 * @param w     Width of image (in pixels)
 * @param h     Height of image (in pixels)
 * @param x     X position on LCD
 * @param y     Y position on LCD
 */
void lcd_draw_img(const uint8_t *img, uint32_t w, uint32_t h, uint32_t x, uint32_t y)
{
    // Calculate image stride (bytes per row)
    uint32_t img_stride = (w + 7) / 8;
    uint8_t x_align = x % 8;

    for (uint32_t dy = 0; dy < h; dy++)
    {
        uint32_t dest_y = y + dy;
        if (dest_y >= LCD_HEIGHT)
            continue;

        // Handle unaligned x positions by processing bytes
        for (uint32_t src_byte = 0; src_byte < img_stride; src_byte++)
        {
            uint8_t img_byte = img[dy * img_stride + src_byte];
            // Reverse bit order for endianness swap
            img_byte = (img_byte & 0xF0) >> 4 | (img_byte & 0x0F) << 4;
            img_byte = (img_byte & 0xCC) >> 2 | (img_byte & 0x33) << 2;
            img_byte = (img_byte & 0xAA) >> 1 | (img_byte & 0x55) << 1;

            uint32_t dest_byte = (x / 8) + src_byte;

            if (dest_byte >= (LCD_WIDTH / 8))
                continue;

            if (x_align == 0)
            {
                // Simple case - aligned x position
                g_framebuffer[dest_y][dest_byte] = img_byte;
            }
            else
            {
                // Unaligned case - shift bits between bytes
                uint8_t shift = x_align;
                g_framebuffer[dest_y][dest_byte] |= (img_byte << shift);
                if (dest_byte + 1 < (LCD_WIDTH / 8))
                {
                    g_framebuffer[dest_y][dest_byte + 1] |= (img_byte >> (8 - shift));
                }
            }
        }
    }
}

/**
 * Inverts all pixels in the global framebuffer (black becomes white and vice versa)
 */
void lcd_invert_framebuffer(void)
{
    for (int y = 0; y < LCD_HEIGHT; y++)
    {
        for (int x_byte = 0; x_byte < (LCD_WIDTH / 8); x_byte++)
        {
            g_framebuffer[y][x_byte] = ~g_framebuffer[y][x_byte];
        }
    }
}

void lcd_draw_test_pattern(uint8_t square_size)
{
    // Ensure square_size is at least 1 and not too large
    if (square_size < 1)
        square_size = 1;
    if (square_size > 32)
        square_size = 32;

    for (int y = 0; y < LCD_HEIGHT; y++)
    {
        for (int x = 0; x < LCD_WIDTH; x++)
        {
            // Calculate checkerboard pattern
            uint8_t x_block = x / square_size;
            uint8_t y_block = y / square_size;
            uint8_t pattern = (x_block + y_block) % 2;

            // Set pixel in framebuffer
            if (pattern)
            {
                g_framebuffer[y][x / 8] |= (1 << (7 - (x % 8))); // Set pixel white
            }
            else
            {
                g_framebuffer[y][x / 8] &= ~(1 << (7 - (x % 8))); // Set pixel black
            }
        }
    }
}

void lcd_fill(uint8_t fill_pattern)
{
    memset(g_framebuffer, fill_pattern, sizeof(g_framebuffer));
}

void lcd_clear_buffer(void)
{
    lcd_fill(0x00);
}

// Sends line data to LCD.
//
// Parameters
//     buf	Buffer with line data
//
// Buffer should be of LCD_LINE_BUF_SIZE.
//
// With following values:
//
//     [0] - reserved space for LCD write command
//     [1] - LCD line number
//     [2..51] - 50 bytes of LCD line data
//     [52..53] - padding required by LCD hw
//
// Only line number and line data have to by filled by user.
void LCD_write_line(uint8_t *buf)
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
    delay_us(12);
    HAL_SPI_Transmit(&hspi2, buf, LCD_LINE_BUF_SIZE, HAL_MAX_DELAY);
    delay_us(4);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
    delay_us(4);
}

void __lcd_init()
{
    SPI2_Init();
    TIM1_Init();
    HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 4095, RTC_WAKEUPCLOCK_RTCCLK_DIV8, 0);

    // Clear framebuffer to white (all pixels off)
    lcd_fill(0xff);
}
