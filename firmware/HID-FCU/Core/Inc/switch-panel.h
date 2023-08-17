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
  SW_BATTERY,
  SW_GENERATOR,
  SW_GEN_RESET,
  SW_IGNITION,
  SW_AUX_F_PUMP,
  SW_AVIONIC_BUS1,
  SW_AVIONIC_BUS2,
  SW_STARTER,
  SW_NAV_LT,
  SW_BEACON,
  SW_PROP_DEICE,
  SW_LH_LIGHT,
  SW_LANDING_LIGHT,
  SW_INSTR_LIGHT,
  SW_CABIN_LIGHT,
  SW_ANTI_ICE

} switch_id;

void handle_switch_panel(void);

#endif /* INC_SWITCH_PANEL_H_ */
