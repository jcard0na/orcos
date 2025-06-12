// Header file for the ORCOS library
// This is a subset of functions from dmcp.h
// https://github.com/swissmicros/DMCP5_SDK/blob/master/dmcp/dmcp.h
//
// That header file is distributed under BSD-3

int get_vbat(void);

void LCD_power_on(void);
void LCD_power_off(int clear);

// Put calculator to sleep
// off: if non-zero, only wake on ON key
// off: if zero, wake on any key
void sys_sleep(int off);

// Main library init function
void orcos_init();
