/* SPDX-License-Identifier: MIT */
#ifndef PROTO_CONFIG_H
#define PROTO_CONFIG_H

#include "curve.h"
#include <stddef.h>

/* Text format: sections, rotation_speed, line_width, section0..7.
 * Section values: formula, amplitude, cycles, phase_speed. */
bool proto_config_load(const char *path, ProtoConfig *config, char *error, size_t size);
bool proto_config_save(const char *path, const ProtoConfig *config, char *error, size_t size);

#endif
