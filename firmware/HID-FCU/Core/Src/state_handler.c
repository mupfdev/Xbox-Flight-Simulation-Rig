/*
 * state_handler.c
 *
 *  Created on: Sep 12, 2023
 *      Author: Michael Fitzmayer
 */

#include "state_handler.h"

static fcu_state main_state = STATE_OFF;

fcu_state get_state(void)
{
  return main_state;
}

void set_state(fcu_state state)
{
  main_state = state;
}
