/* SPDX-License-Identifier: MIT */
#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void overwrite(const char *path, const char *text) {
    FILE *f = fopen(path, "wb");
    assert(f);
    assert(fputs(text, f) >= 0);
    assert(fclose(f) == 0);
}

int main(void) {
    const char *path = "proto_curve_test.json";
    ProtoConfig input, loaded;
    char error[256];
    proto_config_defaults(&input);
    assert(proto_config_load(PROTO_SAMPLE_CFG, &input, error, sizeof error));
    assert(input.section_count == 4);
    assert(input.sections[4].formula == PROTO_CUSTOM);
    input.section_count = 16;
    input.rotation_speed = -15.0f;
    input.show_divider = false;
    input.violet[0] = 0x12;
    input.violet[3] = 0xBB;
    input.cyan[1] = 0x34;
    input.sections[15].formula = PROTO_CUSTOM;
    strcpy(input.sections[15].expression, "sin(2*pi*(cycles*u+speed*t))");
    assert(proto_config_save(path, &input, error, sizeof error));
    proto_config_defaults(&loaded);
    assert(proto_config_load(path, &loaded, error, sizeof error));
    assert(loaded.section_count == 16);
    assert(loaded.rotation_speed == -15.0f);
    assert(!loaded.show_divider);
    assert(loaded.violet[0] == 0x12 && loaded.violet[3] == 0xBB && loaded.cyan[1] == 0x34);
    assert(loaded.sections[15].formula == PROTO_CUSTOM);
    assert(!strcmp(loaded.sections[15].expression, input.sections[15].expression));
    overwrite(path, "{\"section_count\":3}");
    assert(!proto_config_load(path, &loaded, error, sizeof error));
    assert(loaded.section_count == 16); /* No partial mutation. */
    overwrite(path, "{\"section_count\":1,\"sections\":[{\"formula\":\"tri(cycles*u)\"}]}");
    assert(proto_config_load(path, &loaded, error, sizeof error));
    assert(loaded.section_count == 1 && loaded.sections[0].formula == PROTO_CUSTOM);
    overwrite(path, "{\"section_count\":4,\"sections\":[{\"formula\":\"straight\"}]}");
    assert(!proto_config_load(path, &loaded, error, sizeof error));
    overwrite(path, "{\"section_count\":2,\"section_count\":4}");
    assert(!proto_config_load(path, &loaded, error, sizeof error));
    overwrite(path, "{\"colors\": {\"violet\":\"#XXYYZZ\"}}");
    assert(!proto_config_load(path, &loaded, error, sizeof error));
    overwrite(path, "{\"sections\":[{\"formula\":\"sin(bad*u)\"}]}");
    assert(!proto_config_load(path, &loaded, error, sizeof error));
    overwrite(path, "{\"show_divider\":\"false\"}");
    assert(!proto_config_load(path, &loaded, error, sizeof error));
    overwrite(path, "{\"extra\":123}");
    assert(!proto_config_load(path, &loaded, error, sizeof error));
    overwrite(path, "{\"section_count\":1,}");
    assert(!proto_config_load(path, &loaded, error, sizeof error));
    remove(path);
    puts("config_roundtrip: ok");
    return 0;
}
