
#include <freewpc.h>
#include <mach/switch.h>
#include <mach/coil.h>

void sw_rocket_handler (void)
{
}

DECLARE_SWITCH_DRIVER (sw_rocket)
{
	.fn = sw_rocket_handler,
	.devno = SW_DEVICE_DECL(3),
};

void rocket_enter (device_t *dev)
{
	score_add_current_const(0x10000);
}

__taskentry__ void rocket_kick_sound (void)
{
	sound_send (SND_ROCKET_KICK_DONE);
	flasher_pulse (FLASH_UR_FLIPPER);
	task_exit ();
}

void rocket_kick_attempt (device_t *dev)
{
	if (in_live_game)
	{
		db_puts ("Sending rocket kick sound\n");
		leff_start (LEFF_NO_GI);
		sound_send (SND_ROCKET_KICK_REVVING);
		task_sleep (TIME_100MS * 8);
		task_create_gid (0, rocket_kick_sound);
	}
}



device_ops_t rocket_ops = {
	.enter = rocket_enter,
	.kick_attempt = rocket_kick_attempt,
};

device_properties_t rocket_props = {
	.ops = &rocket_ops,
	.name = "ROCKET KICKER",
	.sol = SOL_ROCKET,
	.sw_count = 1,
	.init_max_count = 0,
	.sw = { SW_ROCKET },
};


CALLSET_ENTRY (rocket, init)
{
	device_register (3, &rocket_props);
}

