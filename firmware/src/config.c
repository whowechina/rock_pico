/*
 * Controller Config and Runtime Data
 * WHowe <github.com/whowechina>
 * 
 * Config is a global data structure that stores all the configuration
 * Runtime is something to share between files.
 */

#include "config.h"
#include "savedata.h"

rock_cfg_t *rock_cfg;

static rock_cfg_t default_cfg = {
    .light = {
        .level = 128,
    },
    .pedal = {
        .mode = 0,
        .invert = 0,
    },
    .hid = {
        .keyboard = true,
        .joy = true,
        //       START L1 L2 L3 R1 R2 R3 Pedal
        // Default: 1,  D  S  F  K  J  L Spacebar
        .keycode = {30, 7, 22, 9, 14, 13, 15, 44},
    },
};

rock_runtime_t rock_runtime;

static void config_loaded()
{
    if (rock_cfg->pedal.mode > 1) {
        rock_cfg->pedal.mode = 0;
    }
    rock_cfg->pedal.invert = rock_cfg->pedal.invert ? 1 : 0;
    rock_cfg->pedal.reserved = 0;

    for (int i = 0; i < 8; i++) {
        uint8_t key = rock_cfg->hid.keycode[i];
        if (key > 0x7f) {
            rock_cfg->hid.keycode[i] = default_cfg.hid.keycode[i];
        }
    }
    config_changed();
}

void config_changed()
{
    savedata_save(false);
}

void config_factory_reset()
{
    *rock_cfg = default_cfg;
    savedata_save_clean();
}

void config_init()
{
    rock_cfg = (rock_cfg_t *)savedata_alloc(sizeof(*rock_cfg), &default_cfg, config_loaded);
}
