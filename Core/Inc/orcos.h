// Header file for the ORCOS library
// This is a subset of functions from dmcp.h
// https://github.com/swissmicros/DMCP5_SDK/blob/master/dmcp/dmcp.h
//
// That header file is distributed under BSD-3

int get_vbat(void);

void LCD_power_on();
void LCD_power_off(int clear);


// Main library init function
void orcos_init();
