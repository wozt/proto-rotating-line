/* SPDX-License-Identifier: MIT */
/* SDL handles presentation only; all divider geometry lives in curve.c. */
#include "config.h"
#include "curve.h"

#include <SDL3/SDL.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *program) {
    fprintf(stderr, "Usage: %s [--config FILE.json] [--sections 1|2|4|8|16] "
                    "[--speed DEG_PER_SEC]\n", program);
}

static bool parse_float(const char *arg, float *out) {
    char *end = NULL;
    float value = strtof(arg, &end);
    if (end == arg || *end || !isfinite(value)) return false;
    *out = value;
    return true;
}

static bool parse_options(int argc, char **argv, ProtoConfig *config, const char **path) {
    /* Load JSON first regardless of CLI option order; flags override JSON. */
    *path = "config/prototype.json";
    bool explicit_config = false;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--config") && i + 1 < argc) {
            *path = argv[++i];
            explicit_config = true;
        }
    }
    char error[256];
    FILE *probe = fopen(*path, "r");
    if (probe) {
        fclose(probe);
        if (!proto_config_load(*path, config, error, sizeof error)) {
            fprintf(stderr, "%s\n", error);
            return false;
        }
    } else if (explicit_config || errno != ENOENT) {
        fprintf(stderr, "Cannot read %s: %s\n", *path, strerror(errno));
        return false;
    }
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--config") && i + 1 < argc) ++i;
        else if (!strcmp(argv[i], "--sections") && i + 1 < argc) {
            float value;
            if (!parse_float(argv[++i], &value) || !proto_valid_section_count((int)value) ||
                value != (float)(int)value) return false;
            config->section_count = (int)value;
        } else if (!strcmp(argv[i], "--speed") && i + 1 < argc) {
            if (!parse_float(argv[++i], &config->rotation_speed) ||
                fabsf(config->rotation_speed) > 360.0f) return false;
        } else return false;
    }
    return true;
}

static void update_title(SDL_Window *window, const ProtoConfig *config,
                         const ProtoAnimation *animation, int selected) {
    char title[320];
    snprintf(title, sizeof title,
             "Rotating Line | %d sections | %.1f deg/s | section %d: %s | "
             "Tab select, F formula, 1/2/4/8/0 count, L line, R reload JSON, "
             "+/- speed, Up/Down amplitude, Space pause, S save",
             config->section_count, config->rotation_speed, selected + 1,
             proto_formula_name(config->sections[selected].formula));
    if (animation->paused) strncat(title, " | PAUSED", sizeof title - strlen(title) - 1);
    SDL_SetWindowTitle(window, title);
}

static void draw(const ProtoConfig *config, const ProtoFrame *frame, unsigned char *pixels) {
    for (int y = 0; y < PROTO_HEIGHT; ++y) {
        for (int x = 0; x < PROTO_WIDTH; ++x) {
            float signed_distance = proto_signed_side(frame, x + 0.5f, y + 0.5f);
            const unsigned char *color = config->show_divider && config->line_width > 0.0f &&
                fabsf(signed_distance) <= config->line_width * 0.5f
                ? config->divider_color
                : signed_distance < 0.0f ? config->violet : config->cyan;
            memcpy(pixels + ((size_t)y * PROTO_WIDTH + x) * 4, color, 4);
        }
    }
}

static void key_down(const SDL_KeyboardEvent *key, ProtoConfig *config,
                     ProtoAnimation *animation, int *selected, const char *config_path) {
    ProtoSection *section = &config->sections[*selected];
    switch (key->key) {
    case SDLK_TAB:
        *selected = (*selected + 1) % config->section_count;
        break;
    case SDLK_1: case SDLK_2: case SDLK_4: case SDLK_8: case SDLK_0:
        config->section_count = key->key == SDLK_1 ? 1 : key->key == SDLK_2 ? 2 :
                                key->key == SDLK_4 ? 4 : key->key == SDLK_8 ? 8 : 16;
        if (*selected >= config->section_count) *selected = config->section_count - 1;
        break;
    case SDLK_F:
        proto_animation_change_formula(animation, config, *selected,
            (ProtoFormula)((section->formula + 1) % PROTO_CUSTOM));
        break;
    case SDLK_L:
        config->show_divider = !config->show_divider;
        break;
    case SDLK_UP:
        if (section->amplitude < 48.0f) section->amplitude += 1.0f;
        break;
    case SDLK_DOWN:
        if (section->amplitude > 0.0f) section->amplitude -= 1.0f;
        break;
    case SDLK_EQUALS: case SDLK_KP_PLUS:
        if (config->rotation_speed < 360.0f) config->rotation_speed += 1.0f;
        break;
    case SDLK_MINUS: case SDLK_KP_MINUS:
        if (config->rotation_speed > -360.0f) config->rotation_speed -= 1.0f;
        break;
    case SDLK_SPACE:
        animation->paused = !animation->paused;
        break;
    case SDLK_R: {
        char error[256];
        ProtoConfig reloaded = *config;
        if (proto_config_load(config_path, &reloaded, error, sizeof error)) {
            float time = animation->time, rotation = animation->rotation_degrees;
            bool paused = animation->paused;
            *config = reloaded;
            proto_animation_init(animation, config);
            animation->time = time;
            animation->rotation_degrees = rotation;
            animation->paused = paused;
            if (*selected >= config->section_count) *selected = config->section_count - 1;
            fprintf(stdout, "Reloaded %s\n", config_path);
        } else fprintf(stderr, "%s\n", error);
        break;
    }
    case SDLK_S: {
        char error[256];
        if (proto_config_save(config_path, config, error, sizeof error))
            fprintf(stdout, "Saved %s\n", config_path);
        else fprintf(stderr, "%s: %s\n", config_path, error);
        break;
    }
    default: break;
    }
}

int main(int argc, char **argv) {
    ProtoConfig config;
    proto_config_defaults(&config);
    const char *config_path;
    if (!parse_options(argc, argv, &config, &config_path)) {
        usage(argv[0]);
        return 2;
    }
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window *window = SDL_CreateWindow("Rotating Line", PROTO_WIDTH * 4,
                                          PROTO_HEIGHT * 4, SDL_WINDOW_RESIZABLE);
    SDL_Renderer *renderer = window ? SDL_CreateRenderer(window, NULL) : NULL;
    SDL_Texture *texture = renderer ? SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                         SDL_TEXTUREACCESS_STREAMING, PROTO_WIDTH, PROTO_HEIGHT) : NULL;
    if (!texture || !SDL_SetRenderLogicalPresentation(renderer, PROTO_WIDTH, PROTO_HEIGHT,
                                                     SDL_LOGICAL_PRESENTATION_LETTERBOX)) {
        fprintf(stderr, "SDL video: %s\n", SDL_GetError());
        if (texture) SDL_DestroyTexture(texture);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    SDL_SetRenderVSync(renderer, 1);
    unsigned char pixels[PROTO_WIDTH * PROTO_HEIGHT * 4];
    ProtoFrame frame;
    ProtoAnimation animation;
    proto_animation_init(&animation, &config);
    int selected = 0;
    update_title(window, &config, &animation, selected);
    bool running = true;
    uint64_t last = SDL_GetTicksNS();
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
                if (event.key.key == SDLK_ESCAPE) running = false;
                else key_down(&event.key, &config, &animation, &selected, config_path);
                update_title(window, &config, &animation, selected);
            }
        }
        uint64_t now = SDL_GetTicksNS();
        float dt = (float)(now - last) / 1000000000.0f;
        last = now;
        proto_animation_tick(&animation, &config, dt);
        proto_make_frame(&config, &animation, &frame);
        draw(&config, &frame, pixels);
        SDL_UpdateTexture(texture, NULL, pixels, PROTO_WIDTH * 4);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
