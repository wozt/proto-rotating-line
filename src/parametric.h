/* SPDX-License-Identifier: MIT */
#ifndef PROTO_PARAMETRIC_H
#define PROTO_PARAMETRIC_H
#include "curve.h"
void proto_spiral_point(float u, float section_start, float section_span,
                        float amplitude, float curl, float *x, float *y);
/* Thin centred curl used by the spatial-gradient mode. */
void proto_tentacle_gradient_point(float u, float start, float span,
                                  float amplitude, float *x, float *y);
/* Wolfram Alpha arc on t=0..0.5 with straight connectors. */
void proto_wolfram_halfturn_point(float p, float cell_start,
                                  float cell_span, float radius,
                                  float strength, float *x, float *y);
void proto_parametric_prepare(ProtoFrame *frame);
float proto_parametric_side(const ProtoFrame *frame, float tangent, float normal);
#endif
