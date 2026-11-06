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
/* Two strands on the same Wolfram spiral, joined at the animated tip. */
void proto_wolfram_tentacle_point(float p, float cell_start, float cell_span,
                                  float amplitude, float t_limit,
                                  float strand_offset, float *x, float *y);
/* Rasterize closed, tapered Wolfram ribbons into the colour mask. */
void proto_wolfram_ribbon_prepare(ProtoFrame *frame,
                                   const ProtoConfig *config,
                                   const ProtoAnimation *animation);
/* Two-arm Fermat spiral, one continuous divider. */
void proto_fermat_spiral_point(float p, float cell_start,
                               float cell_span, float amplitude,
                               float progress, float *x, float *y);
/* Piecewise cubic position; parabolic velocity per half-cycle. */
float proto_fermat_parabolic_progress(float time, float phase_speed);
/* Faster 5s two-arm Fermat with independent winding depth. */
float proto_fermat_twist_progress(float time, float phase_speed);
float proto_fermat_twist_direction(float time, float phase_speed);
void proto_fermat_twist_point(float p, float cell_start,
                              float cell_span, float amplitude,
                              float progress, float winding,
                              float direction, float *x, float *y);
/* Peak-radius-weighted positions for eight Fermat twists. */
void proto_fermat_packed_cell(float start, float span, int count, int index,
                              float *cell_start, float *cell_span);
float proto_metroid_lightning_value(float x);
float proto_fermat_stagger_progress(float time, float speed,
                                    int index, float delay);
float proto_fermat_bothways_progress(float time, float speed,
                                    int from_centre, float delay);
float proto_fermat_tall_reference_peak(int from_centre);
void proto_parametric_prepare(ProtoFrame *frame);
float proto_parametric_side(const ProtoFrame *frame, float tangent, float normal);
#endif
