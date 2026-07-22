/*
 * Pedal handling
 * WHowe <github.com/whowechina>
 */

#include "pedal.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "hardware/gpio.h"

#include "board_defs.h"

#include "button.h"
#include "config.h"

static const uint8_t gpios[] = BUTTON_DEF;

#define PEDAL_BIT_TIP (count_of(gpios) - 2)
#define PEDAL_BIT_RING (count_of(gpios) - 1)

#define PEDAL_OUT_BIT PEDAL_BIT_TIP

void pedal_init()
{
    gpio_init(gpios[PEDAL_BIT_TIP]);
    gpio_set_function(gpios[PEDAL_BIT_TIP], GPIO_FUNC_SIO);
    gpio_set_dir(gpios[PEDAL_BIT_TIP], GPIO_IN);
    gpio_pull_up(gpios[PEDAL_BIT_TIP]);
    gpio_init(gpios[PEDAL_BIT_RING]);
    gpio_set_function(gpios[PEDAL_BIT_RING], GPIO_FUNC_SIO);
    gpio_set_dir(gpios[PEDAL_BIT_RING], GPIO_IN);
    gpio_pull_up(gpios[PEDAL_BIT_RING]);
}

bool pedal_is_active(uint16_t buttons)
{
    int bit = rock_cfg->pedal.mode ? PEDAL_BIT_RING : PEDAL_BIT_TIP;
    bool active = buttons & (1 << bit);
    return rock_cfg->pedal.invert ? !active : active;
}

void pedal_update_mode()
{
    // nothing to do for now, just a placeholder
}
