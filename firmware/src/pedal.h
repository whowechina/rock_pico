/*
 * Pedal handling
 * WHowe <github.com/whowechina>
 *
 * Provide pedal read and mode update according to
 * rock_cfg->pedal.mode and rock_cfg->pedal.invert
 */

#ifndef PEDAL_H
#define PEDAL_H

#include <stdbool.h>
#include <stdint.h>

void pedal_init();

bool pedal_read();
bool pedal_is_active(uint16_t buttons);
void pedal_update_mode();

#endif
