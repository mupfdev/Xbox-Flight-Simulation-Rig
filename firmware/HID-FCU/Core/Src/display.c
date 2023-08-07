/*
 * display.c
 *
 *  Created on: Jul 23, 2023
 *      Author: Michael Fitzmayer
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "display.h"
#include "main.h"
#include "tm1637.h"

static tm1637_t hdisp_1;
static tm1637_t hdisp_2;

void clear_display(void)
{
  tm1637_fill(&hdisp_1, false);
  tm1637_fill(&hdisp_2, false);
}

void fill_display(void)
{
  tm1637_fill(&hdisp_1, true);
  tm1637_fill(&hdisp_2, true);
}

void set_display(uint32_t state, int32_t altitude)
{
  static bool is_initialised = false;
  uint8_t seg[4]     = { 0 };
  uint8_t seg_alt[4] = { 0 };
	uint8_t pos;
	bool overwrite_alt = false;

	if (false == is_initialised)
	{
		tm1637_init(&hdisp_1, DISP_1_CLK_GPIO_Port, DISP_1_CLK_Pin, DISP_1_DIO_GPIO_Port, DISP_1_DIO_Pin);
		tm1637_init(&hdisp_2, DISP_2_CLK_GPIO_Port, DISP_2_CLK_Pin, DISP_2_DIO_GPIO_Port, DISP_2_DIO_Pin);
		is_initialised = true;
	}

	tm1637_fill(&hdisp_1, false);
	tm1637_fill(&hdisp_2, false);

	seg[0] = state & 0xff;
	seg[1] = (state >> 8) & 0xff;
	seg[2] = (state >> 16) & 0xff;

	/* Set altitude boundaries. */
	if (altitude > 99999)
	{
		altitude = 99999;
	}
	else if (altitude < -9999)
	{
		altitude = -9999;
	}

	if (altitude >= 1000)
	{
	  tm1637_show_zero(&hdisp_2, true);
	}
	else
	{
    tm1637_show_zero(&hdisp_2, false);
	}

	/* Format output. */
	if ((altitude) >= 0 && (altitude < 10))
	{
	  pos = 3;
	}
	else if ((altitude >= 10) && (altitude < 100))
	{
	  pos = 2;
	}
	else if ((altitude >= 100) && (altitude < 1000))
	{
	  pos = 1;
	}
	else
	{
	 pos = 0;
	}

	if (altitude <= -1000)
	{
		altitude = -altitude;
		seg[3]   = 0x40;
	}
    else if (altitude >= 10000)
	{
    	extern const uint8_t _tm1637_digit[];
    	char str[2];

    	int32_t tmp = ((altitude / 10000) * 10000);
    	altitude    = altitude - tmp;

    	/* Hack, but it works for the time being. */
    	if (altitude < 1000)
    	{
    	  overwrite_alt = true;
        seg_alt[0] = DIGIT_0;
        seg_alt[2] = DIGIT_0;
        seg_alt[3] = DIGIT_0;
    	}
    	switch (altitude)
    	{
    	  case 0:
          seg_alt[1] = DIGIT_0;
          break;
    	  case 100:
          seg_alt[1] = DIGIT_1;
          break;
        case 200:
          seg_alt[1] = DIGIT_2;
          break;
        case 300:
          seg_alt[1] = DIGIT_3;
          break;
        case 400:
          seg_alt[1] = DIGIT_4;
          break;
        case 500:
          seg_alt[1] = DIGIT_5;
          break;
        case 600:
          seg_alt[1] = DIGIT_6;
          break;
        case 700:
          seg_alt[1] = DIGIT_7;
          break;
        case 800:
          seg_alt[1] = DIGIT_8;
          break;
        case 900:
          seg_alt[1] = DIGIT_9;
          break;
    	}

    	snprintf(str, 2 , "%ld", tmp);

    	seg[3] = _tm1637_digit[str[0] - 48];
	}
	else
	{
		/* All good. */
	}

	tm1637_write_segment(&hdisp_1, seg, 4, 0);
	if (true == overwrite_alt)
	{
	  tm1637_write_segment(&hdisp_2, seg_alt, 4, 0);
	}
	else
	{
	  tm1637_write_int(&hdisp_2, altitude, pos);
	}
}
