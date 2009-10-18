/*
 * Copyright 2006, 2007, 2008, 2009 by Brian Dominy <brian@oddchange.com>
 *
 * This file is part of FreeWPC.
 *
 * FreeWPC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * FreeWPC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FreeWPC; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * \file
 * \brief Common flipper logic, for switches and solenoids
 */

#include <freewpc.h>
#include <test.h>


/** Says whether or not the flipper coils are enabled */
__fastram__ U8 flippers_enabled;

__fastram__ U8 fliptronic_powered_coil_outputs;

volatile __fastram__ U8 flipper_inputs;

volatile __fastram__ U8 flipper_outputs;


/** Software controlled flipper inputs for Fliptronic games. */
#if (MACHINE_FLIPTRONIC == 1)
__fastram__ U8 flipper_overrides;
#endif


void flipper_enable (void)
{
	flipper_triac_enable ();
	flippers_enabled = TRUE;
}


void flipper_disable (void)
{
	flipper_triac_disable ();
	flippers_enabled = FALSE;
}


#if (MACHINE_FLIPTRONIC == 1)

void flipper_override_on (U8 switches)
{
	flipper_overrides |= switches;
}


void flipper_override_off (U8 switches)
{
	flipper_overrides &= ~switches;
}


void flipper_override_pulse (U8 switches)
{
	flipper_override_on (switches);
	task_sleep (TIME_166MS);
	flipper_override_off (switches);
	task_sleep (TIME_66MS);
}


/** Perform real-time processing for a single flipper.
 * This is executed once per flipper every 2ms.
 */
static inline void flipper_service (
	const U8 sw_button, const U8 sw_eos,
	const U8 sol_power, const U8 sol_hold )
{
	/* The logic is as follows:
	 * If the button is held and the EOS is active, enable holding power.
	 * If the button is held and no EOS is seen, enable full power.
	 * If the button is not held, then the coil is off by default.
	 *
	 * Future enhancements:
	 * - Duty cycling: pulsing the coil at less than 100%
	 * - Handling EOS errors: If the EOS is broken, there will always be
	 * full power.
	 * - Handle button errors: If a button is not working, its
	 * peer button (upper & lower as part of a set) should compensate.
	 * - Switch debouncing on the EOS
	 * - Minimum off time between presses, to prevent constant full
	 * power because EOS never has a chance to kick in.
	 * - Detect system crashes where the entire memory becomes FF.
	 * This causes the flippers to go permanently (at least for a while).
	 */

	if (unlikely (flipper_inputs & sw_button))
	{
		if (likely (flipper_inputs & sw_eos))
		{
			flipper_outputs |= sol_hold;
		}
		else
		{
			flipper_outputs |= sol_power | sol_hold;
		}
	}
}


/** Real-time function that services all of the flipper switches and coils.
 * On non-Fliptronic games, the CPU has no visibility to the flippers so
 * this isn't necessary. */
void fliptronic_rtt (void)
{
	if (likely (flippers_enabled))
	{
		register U8 inputs = ~wpc_read_flippers () | flipper_overrides;
		if (unlikely (inputs))
		{
			flipper_inputs = inputs;
			flipper_outputs = fliptronic_powered_coil_outputs;

			flipper_service (WPC_LL_FLIP_SW, WPC_LL_FLIP_EOS, WPC_LL_FLIP_POWER, WPC_LL_FLIP_HOLD);
			flipper_service (WPC_LR_FLIP_SW, WPC_LR_FLIP_EOS, WPC_LR_FLIP_POWER, WPC_LR_FLIP_HOLD);

			/* Some machines use the upper flipper coils for other uses.
			 * Those can already be handled by the regular solenoid module. */
#ifdef MACHINE_HAS_UPPER_LEFT_FLIPPER
			flipper_service (WPC_UL_FLIP_SW, WPC_UL_FLIP_EOS, WPC_UL_FLIP_POWER, WPC_UL_FLIP_HOLD);
#endif

#ifdef MACHINE_HAS_UPPER_RIGHT_FLIPPER
			flipper_service (WPC_UR_FLIP_SW, WPC_UR_FLIP_EOS, WPC_UR_FLIP_POWER, WPC_UR_FLIP_HOLD);
#endif
			wpc_write_flippers (flipper_outputs);
			return;
		}
	}

	wpc_write_flippers (fliptronic_powered_coil_outputs);
}

#endif /* MACHINE_FLIPTRONIC */


CALLSET_ENTRY (fliptronic, ball_search)
{
#if (MACHINE_FLIPTRONIC == 1)
	flipper_override_pulse (WPC_LL_FLIP_SW);
	flipper_override_pulse (WPC_LR_FLIP_SW);
#ifdef MACHINE_HAS_UPPER_LEFT_FLIPPER
	flipper_override_pulse (WPC_UL_FLIP_SW);
#endif
#ifdef MACHINE_HAS_UPPER_RIGHT_FLIPPER
	flipper_override_pulse (WPC_UR_FLIP_SW);
#endif
#endif /* MACHINE_FLIPTRONIC */
}

void flipper_init (void)
{
	flipper_disable ();
	fliptronic_powered_coil_outputs = 0;
}


