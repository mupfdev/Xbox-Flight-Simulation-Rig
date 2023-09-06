/*
 * switch-panel.h
 *
 *  Created on: Aug 16, 2023
 *      Author: Michael Fitzmayer
 */

#ifndef INC_SWITCH_PANEL_H_
#define INC_SWITCH_PANEL_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
  SW_BATTERY = 0,
  SW_GENERATOR,
  SW_GEN_RESET,
  SW_AUX_F_PUMP,
  SW_AVIONIC_BUS1,
  SW_AVIONIC_BUS2,
  SW_IGNITION,
  SW_STARTER,
  SW_NAV_LT,
  SW_BEACON,
  SW_LANDING_LIGHT,
  SW_LANDING_LIGHT_POS,
  SW_INSTR_LIGHT,
  SW_CABIN_LIGHT,
  SW_ANTI_ICE,
  SW_PROP_DEICE,
  SW_RESERVED_1,
  SW_RESERVED_2,
  SW_RESERVED_3,
  SW_RESERVED_4,
  SW_RESERVED_5,
  SW_RESERVED_6,
  SW_RESERVED_7,
  SW_RESERVED_8,
  SW_RESERVED_9,
  SW_RESERVED_10,
  SW_RESERVED_11,
  SW_RESERVED_12,
  SW_RESERVED_13,
  SW_RESERVED_14,
  SW_RESERVED_15,
  SW_RESERVED_16

} switch_id;

void handle_switch_panel(void);

#endif /* INC_SWITCH_PANEL_H_ */
