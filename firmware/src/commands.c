#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

#include "pico/stdio.h"
#include "pico/stdlib.h"

#include "config.h"
#include "savedata.h"
#include "cli.h"
#include "pedal.h"

#include "usb_descriptors.h"

#define SENSE_LIMIT_MAX 9
#define SENSE_LIMIT_MIN -9

static void disp_light()
{
    printf("[Light]\n");
    printf("  Level: %d.\n", rock_cfg->light.level);
}

static void disp_pedal()
{
    printf("[Pedal]\n");
    printf("  Input: %s.\n", rock_cfg->pedal.mode ? "Ring" : "Tip");
    printf("  Invert: %s.\n", rock_cfg->pedal.invert ? "Yes" : "No");
}

void handle_display(int argc, char *argv[])
{
    const char *usage = "Usage: display [light|pedal]\n";
    if (argc > 1) {
        printf(usage);
        return;
    }

    if (argc == 0) {
        disp_light();
        disp_pedal();
        return;
    }

    const char *choices[] = {"light", "pedal" };
    switch (cli_match_prefix(choices, count_of(choices), argv[0])) {
        case 0:
            disp_light();
            break;
        case 1:
            disp_pedal();
            break;
        default:
            printf(usage);
            break;
    }
}

static int fps[2];
void fps_count(int core)
{
    static uint32_t last[2] = {0};
    static int counter[2] = {0};

    counter[core]++;

    uint32_t now = time_us_32();
    if (now - last[core] < 1000000) {
        return;
    }
    last[core] = now;
    fps[core] = counter[core];
    counter[core] = 0;
}

static void handle_level(int argc, char *argv[])
{
    const char *usage = "Usage: level <0..255>\n";
    if (argc != 1) {
        printf(usage);
        return;
    }

    int level = cli_extract_non_neg_int(argv[0], 0);
    if ((level < 0) || (level > 255)) {
        printf(usage);
        return;
    }

    rock_cfg->light.level = level;
    config_changed();
    disp_light();
}

static void handle_pedal(int argc, char *argv[])
{
    const char *usage = "Usage: pedal <tip|ring> [invert]\n";
    if ((argc < 1) || (argc > 2)) {
        printf(usage);
        return;
    }

    const char *input_modes[] = {"tip", "ring"};
    int mode = cli_match_prefix(input_modes, count_of(input_modes), argv[0]);
    if (mode < 0) {
        printf(usage);
        return;
    }

    bool invert = false;
    if (argc == 2) {
        if (strncasecmp(argv[1], "invert", strlen(argv[1])) != 0) {
            printf(usage);
            return;
        }
        invert = true;
    }

    rock_cfg->pedal.mode = mode;
    rock_cfg->pedal.invert = invert;

    pedal_update_mode();
    config_changed();
    disp_pedal();
}

static void handle_save()
{
    savedata_save(true);
}

static void handle_factory_reset()
{
    config_factory_reset();
    printf("Factory reset done.\n");
}

void commands_init()
{
    cli_register("display", handle_display, "Display all config.");
    cli_register("level", handle_level, "Set LED brightness level.");
    cli_register("pedal", handle_pedal, "Set pedal input mode.");
    cli_register("save", handle_save, "Save config to flash.");
    cli_register("factory", handle_factory_reset, "Reset everything to default.");
}
