/*
 * ads1115.h
 *
 *  Created on: Sep 3, 2023
 *      Author: Michael Fitzmayer
 */

#ifndef ADS1115_H
#define ADS1115_H

#include <stdint.h>

void ads1115_init(void);
void ads1115_read(int16_t* raw, int16_t lower_limit, int16_t upper_limit);
void ads1115_set_channel(uint8_t channel);

#endif /* ADS1115_H */
