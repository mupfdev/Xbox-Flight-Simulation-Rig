/*
 * display.h
 *
 *  Created on: Jul 23, 2023
 *      Author: Michael Fitzmayer
 */

#ifndef INC_DISPLAY_H_
#define INC_DISPLAY_H_

#include <stdint.h>

#define SEG_ALT 0x00783877
#define SEG_CLR 0x00000000
#define SEG_HDG 0x003d5e76
#define SEG_NAV 0x003e7737
#define SEG_OFF 0x00000000
#define SEG_PFT 0x00787173
#define SEG_ROL 0x00383f33
#define SEG_VS  0x00006d3e

#define DIGIT_0 0x3f
#define DIGIT_1 0x06
#define DIGIT_2 0x5b
#define DIGIT_3 0x4f
#define DIGIT_4 0x66
#define DIGIT_5 0x6d
#define DIGIT_6 0x7d
#define DIGIT_7 0x07
#define DIGIT_8 0x7f
#define DIGIT_9 0x6f

void clear_display(void);
void fill_display(void);
void set_display(uint32_t state, int32_t altitude);

#endif /* INC_DISPLAY_H_ */
