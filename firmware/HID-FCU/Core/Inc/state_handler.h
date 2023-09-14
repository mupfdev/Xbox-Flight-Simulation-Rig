/*
 * state_handler.h
 *
 *  Created on: Sep 12, 2023
 *      Author: Michael Fitzmayer
 */

#ifndef STATE_HANDLER_H_
#define STATE_HANDLER_H_

typedef enum
{
  STATE_OFF,
  STATE_AVIONICS_BUS2_BOOTUP,
  STATE_READY

} fcu_state;

fcu_state get_state(void);
void      set_state(fcu_state state);

#endif /* STATE_HANDLER_H_ */
