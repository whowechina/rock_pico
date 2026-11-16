/*
 * Controller Main
 * WHowe <github.com/whowechina>
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "bsp/board.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"

#include "hardware/clocks.h"
#include "hardware/flash.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"

#include "tusb.h"
#include "usb_descriptors.h"

#include "board_defs.h"

#include "savedata.h"
#include "config.h"
#include "cli.h"
#include "commands.h"

#include "light.h"
#include "button.h"
#include "pedal.h"


typedef struct {
    uint8_t h;
    uint8_t s;
    uint8_t v;
} hsv_t;

static struct {
    hsv_t on[7];
    hsv_t off[7];
} main_theme = {
    .on = {
        { HUE_RED, 240, 255 },
        { HUE_RED, 240, 255 },
        { HUE_BLUE, 240, 255 },
        { HUE_BLUE, 240, 255 },
        { HUE_RED, 240, 255 },
        { HUE_BLUE, 240, 255 },
        { HUE_BLUE, 240, 255 },
    },
    .off = {
        { HUE_RED, 0, 36 },
        { HUE_RED, 0, 36 },
        { HUE_BLUE, 0, 36 },
        { HUE_BLUE, 0, 36 },
        { HUE_RED, 0, 36 },
        { HUE_BLUE, 0, 36 },
        { HUE_BLUE, 0, 36 },
    }
};

static uint32_t hsv2rgb(hsv_t hsv)
{
    return rgb32_from_hsv(hsv.h, hsv.s, hsv.v);
}

static void light_render()
{
    uint16_t button = button_read();
    for (int i = 0; i < 7; i++) {
        uint32_t rgb = hsv2rgb((button & (1 << i)) ? main_theme.on[i]
                                                   : main_theme.off[i]);
        light_set(i, rgb, false);
    }
}

static void run_lights()
{
    static uint64_t next_frame = 0;
    uint64_t now = time_us_64();
    if (now < next_frame) {
        return;
    }
    next_frame = now + 1000000 / 120; // 120Hz

    light_render();
}

static void core1_init()
{
    flash_safe_execute_core_init();
}

static void core1_loop()
{
    core1_init();
    while (1) {
        run_lights();
        light_update();
        cli_fps_count(1);
        sleep_us(700);
    }
}

struct __attribute__((packed)) {
    uint16_t buttons;
} hid_joy, hid_joy_sent = {0};

struct __attribute__((packed)) {
    uint8_t modifier;
    uint8_t reserved;
    uint8_t keys[16]; // usage bitset for 0x00..0x7f
} hid_kbd, hid_kbd_sent = {0};

static void hid_update()
{
    static uint64_t last_report_key_time = 0;
    static uint64_t last_report_joy_time = 0;

    if (!tud_hid_ready()) {
        return;
    }

    uint64_t now = time_us_64();

    uint16_t buttons = button_read();
    buttons = (buttons & 0x7f) | (pedal_is_active(buttons) ? 0x80 : 0);

    if (rock_cfg->hid.joy) {
        hid_joy.buttons = buttons;

        if ((memcmp(&hid_joy, &hid_joy_sent, sizeof(hid_joy)) != 0) ||
            (now - last_report_joy_time > 10000)) {
            if (tud_hid_report(REPORT_ID_JOYSTICK, &hid_joy, sizeof(hid_joy))) {
                hid_joy_sent = hid_joy;
                last_report_joy_time = now;
            }
        }
    }

    if (rock_cfg->hid.keyboard) {
        memset(&hid_kbd, 0, sizeof(hid_kbd));

        for (int i = 0; i < 8; i++) {
            if (!(buttons & (1 << i))) {
                continue;
            }

            uint8_t keycode = rock_cfg->hid.keycode[i];
            if (keycode == 0 || keycode > 0x7f) {
                continue;
            }

            hid_kbd.keys[keycode / 8] |= 1 << (keycode % 8);
        }

        if ((memcmp(&hid_kbd, &hid_kbd_sent, sizeof(hid_kbd)) != 0) ||
            (now - last_report_key_time > 10000)) {
            if (tud_hid_report(REPORT_ID_KEYBOARD, &hid_kbd, sizeof(hid_kbd))) {
                hid_kbd_sent = hid_kbd;
                last_report_key_time = now;
            }
        }
    }
}

static void core0_loop()
{
    uint64_t next_frame = time_us_64() + 1001;

    while(1) {
        tud_task();

        cli_run();

        savedata_loop();
        cli_fps_count(0);

        button_update();

        hid_update();

        sleep_until(next_frame);
        next_frame += 1001;
    }
}

static void update_check()
{
    uint16_t buttons = button_read();
    if ((buttons & 0x0f) == 0x0f) {
        sleep_ms(100);
        reset_usb_boot(0, 2);
        return;
    }
}

static void hid_mode_check()
{
    uint16_t buttons = button_read();
    if (!(buttons & 0x01)) {
        return;
    }

    bool l1 = buttons & 0x02;
    bool l2 = buttons & 0x04;
    bool l3 = buttons & 0x08;

    rock_cfg->hid.joy = l1 || l2;
    rock_cfg->hid.keyboard = l1 || l3;
    config_changed();
}

void init()
{
    sleep_ms(50);
    set_sys_clock_khz(150000, true);

    board_init();

    button_init();
    update_check();

    tusb_init();
    stdio_init_all();

    config_init();
    savedata_init(0xca4ccabb);

    light_init();
    pedal_init();

    hid_mode_check();

    cli_init("rock_pico>", "\n   << Rock Pico Controller >>\n"
                            " https://github.com/whowechina\n\n");
    
    commands_init();
}

int main(void)
{
    init();
    multicore_launch_core1(core1_loop);
    core0_loop();
    return 0;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen)
{
    printf("Get from USB %d-%d\n", report_id, report_type);
    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize)
{
    if (report_id == REPORT_ID_LIGHTS) {
        if (bufsize < 12) {
            return;
        }
        for (int i = 0; i < 9; i++) {
            if (buffer[i]) {
                light_set(i, hsv2rgb(main_theme.on[i]), true);
            } else {
                light_set(i, hsv2rgb(main_theme.off[i]), true);
            }
        }
    }
}
