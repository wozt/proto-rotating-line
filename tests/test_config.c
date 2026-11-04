/* SPDX-License-Identifier: MIT */
#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    const char *path = "proto_curve_test.cfg";
    ProtoConfig input, loaded;
    char error[256];
    proto_config_defaults(&input);
    input.section_count = 8;
    input.rotation_speed = -15.0f;
    input.sections[7].formula = PROTO_FRACTAL;
    assert(proto_config_save(path, &input, error, sizeof error));
    proto_config_defaults(&loaded);
    assert(proto_config_load(path, &loaded, error, sizeof error));
    assert(loaded.section_count == 8);
    assert(loaded.rotation_speed == -15.0f);
    assert(loaded.sections[7].formula == PROTO_FRACTAL);
    FILE *f = fopen(path, "w");
    assert(f);
    fputs("sections = 3\n", f);
    fclose(f);
    assert(!proto_config_load(path, &loaded, error, sizeof error));
    assert(loaded.section_count == 8); /* Failed loads do not mutate settings. */
    remove(path);
    puts("config_roundtrip: ok");
    return 0;
}
