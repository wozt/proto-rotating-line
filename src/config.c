/* SPDX-License-Identifier: MIT */
#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void set_error(char *error, size_t size, const char *message) {
    if (error && size) snprintf(error, size, "%s", message);
}

static char *trim(char *text) {
    while (isspace((unsigned char)*text)) ++text;
    size_t length = strlen(text);
    while (length && isspace((unsigned char)text[length - 1])) text[--length] = '\0';
    return text;
}

static bool number(const char *text, float *out) {
    char *end = NULL;
    errno = 0;
    float value = strtof(text, &end);
    if (end == text || errno == ERANGE || !isfinite(value)) return false;
    while (isspace((unsigned char)*end)) ++end;
    if (*end) return false;
    *out = value;
    return true;
}

static bool parse_section(char *text, ProtoSection *section) {
    char *values[4];
    for (int i = 0; i < 4; ++i) {
        values[i] = text;
        if (i < 3) {
            char *comma = strchr(text, ',');
            if (!comma) return false;
            *comma = '\0';
            text = comma + 1;
        } else if (strchr(text, ',')) return false;
    }
    ProtoFormula formula;
    float amp, cycles, phase;
    if (!proto_parse_formula(trim(values[0]), &formula) ||
        !number(trim(values[1]), &amp) ||
        !number(trim(values[2]), &cycles) ||
        !number(trim(values[3]), &phase) ||
        amp < 0.0f || amp > 48.0f || cycles < 0.25f || cycles > 12.0f ||
        phase < -4.0f || phase > 4.0f) return false;
    *section = (ProtoSection){formula, amp, cycles, phase};
    return true;
}

bool proto_config_load(const char *path, ProtoConfig *config, char *error, size_t size) {
    FILE *file = fopen(path, "r");
    if (!file) {
        char message[256];
        snprintf(message, sizeof message, "Cannot open %s: %s", path, strerror(errno));
        set_error(error, size, message);
        return false;
    }
    ProtoConfig candidate = *config;
    char line[256];
    int line_no = 0;
    bool valid = true;
    while (fgets(line, sizeof line, file)) {
        ++line_no;
        if (!strchr(line, '\n') && !feof(file)) { valid = false; break; }
        char *item = trim(line);
        if (!*item || *item == '#' || *item == ';') continue;
        char *equals = strchr(item, '=');
        if (!equals) { valid = false; break; }
        *equals = '\0';
        char *key = trim(item), *value = trim(equals + 1);
        float scalar;
        if (strcmp(key, "sections") == 0) {
            if (!number(value, &scalar) ||
                !(scalar == 2.0f || scalar == 4.0f || scalar == 8.0f)) valid = false;
            else candidate.section_count = (int)scalar;
        } else if (strcmp(key, "rotation_speed") == 0) {
            if (!number(value, &scalar) || scalar < -360.0f || scalar > 360.0f) valid = false;
            else candidate.rotation_speed = scalar;
        } else if (strcmp(key, "line_width") == 0) {
            if (!number(value, &scalar) || scalar < 0.0f || scalar > 4.0f) valid = false;
            else candidate.line_width = scalar;
        } else if (strncmp(key, "section", 7) == 0 &&
                   key[7] >= '0' && key[7] <= '7' && key[8] == '\0') {
            valid = parse_section(value, &candidate.sections[key[7] - '0']);
        } else valid = false;
        if (!valid) break;
    }
    if (ferror(file)) valid = false;
    fclose(file);
    if (!valid) {
        char message[256];
        snprintf(message, sizeof message, "Invalid configuration: %s line %d", path, line_no);
        set_error(error, size, message);
        return false;
    }
    *config = candidate; /* A malformed file never partly changes the settings. */
    set_error(error, size, "");
    return true;
}

bool proto_config_save(const char *path, const ProtoConfig *config, char *error, size_t size) {
    FILE *file = fopen(path, "w");
    if (!file) {
        set_error(error, size, "Cannot write the configuration file");
        return false;
    }
    int result = fprintf(file,
                         "# Purely mathematical SDL3 rotating split prototype.\n"
                         "# sectionN = formula, amplitude_px, cycles, phase_cycles_per_second\n"
                         "sections = %d\nrotation_speed = %.3f\nline_width = %.3f\n",
                         config->section_count, config->rotation_speed, config->line_width);
    for (int i = 0; i < PROTO_MAX_SECTIONS && result >= 0; ++i) {
        const ProtoSection *s = &config->sections[i];
        result = fprintf(file, "section%d = %s, %.3f, %.3f, %.3f\n", i,
                         proto_formula_name(s->formula), s->amplitude, s->cycles,
                         s->phase_speed);
    }
    if (fclose(file) != 0 || result < 0) {
        set_error(error, size, "Error saving configuration");
        return false;
    }
    set_error(error, size, "");
    return true;
}
