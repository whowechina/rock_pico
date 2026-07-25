/*
 * Controller Config
 * WHowe <github.com/whowechina>
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

typedef struct __attribute__((packed)) {
    struct {
        uint8_t level;
    } light;
    struct {
        uint8_t mode : 1;
        uint8_t invert : 1;
        uint8_t reserved : 6;
    } pedal;
    struct {
        bool keyboard;
        bool joy;
        uint8_t keycode[8];
    } hid;
    uint8_t theme;
} rock_cfg_t;

typedef struct {
    uint16_t fps[2];
    uint8_t chain_id;
    uint64_t sync_expire;
} rock_runtime_t;

extern rock_cfg_t *rock_cfg;
extern rock_runtime_t rock_runtime;

void config_init();
void config_changed(); // Notify the config has changed
void config_factory_reset(); // Reset the config to factory default

#endif
