/* SPDX-License-Identifier: MIT */
/* SDL handles presentation only; all divider geometry lives in curve.c. */
#include "config.h"
#include "curve.h"
#include "pulse_audio.h"

#include <SDL3/SDL.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

/* Patch 0029: keyboard X/Y and Nintendo-layout gamepad north/west.
 * Long hold only launches if the corresponding environment command is set. */
typedef struct {
    bool held;
    bool launched;
    float elapsed;
    float pulse_elapsed;
} ProtoHeldButton;

static void proto_request_launch(bool aria) {
    const char *env_key = aria ? "PROTO_ARIA_LAUNCH" : "PROTO_METROID_LAUNCH";
    const char *command = getenv(env_key);
    if (!command || !*command) {
        fprintf(stderr, "%s is not configured; long press will not launch a game.\n", env_key);
        return;
    }
    const pid_t child = fork();
    if (child == 0) {
        execl("/bin/sh", "sh", "-c", command, (char *)NULL);
        _exit(127);
    }
    if (child < 0) fprintf(stderr, "Failed to start %s: %s\n", env_key, strerror(errno));
    else fprintf(stdout, "Launch requested: %s\n", env_key);
}

static void proto_trigger_pulse(ProtoAnimation *animation, SDL_AudioStream *sound,
                                bool aria) {
    if (aria) animation->aria_pulse_age = 0.0f;
    else animation->metroid_pulse_age = 0.0f;
    proto_pulse_audio_emit(sound, aria);
}

static void proto_press(ProtoHeldButton *button, ProtoAnimation *animation,
                        SDL_AudioStream *sound, bool aria) {
    if (button->held) return;
    button->held = true;
    button->launched = false;
    button->elapsed = 0.0f;
    button->pulse_elapsed = 0.0f;
    proto_trigger_pulse(animation, sound, aria);
}

static void proto_release(ProtoHeldButton *button) {
    button->held = false;
    button->launched = false;
    button->elapsed = 0.0f;
    button->pulse_elapsed = 0.0f;
}

static void proto_hold_tick(ProtoHeldButton *button, float dt,
                            ProtoAnimation *animation, SDL_AudioStream *sound, bool aria) {
    if (!button->held) return;
    button->elapsed += dt;
    button->pulse_elapsed += dt;
    if (!button->launched && button->elapsed >= 1.0f) {
        button->launched = true;
        proto_request_launch(aria);
    }
    if (button->pulse_elapsed >= 0.5f) {
        button->pulse_elapsed -= 0.5f;
        proto_trigger_pulse(animation, sound, aria);
    }
}

static void proto_pulse_age_tick(ProtoAnimation *animation, float dt) {
    float *ages[] = { &animation->aria_pulse_age, &animation->metroid_pulse_age };
    for (int i = 0; i < 2; ++i) {
        if (*ages[i] >= 0.0f) {
            *ages[i] += dt;
            if (*ages[i] >= 0.24f) *ages[i] = -1.0f;
        }
    }
}

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
             "+/- speed, Up/Down amplitude, Space pause, S save, "
             "X Aria | Y Metroid (hold 1s to launch)",
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
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
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
    SDL_InitSubSystem(SDL_INIT_AUDIO); /* Silent fallback if unavailable. */
    SDL_AudioStream *sound = proto_pulse_audio_open();
    if (!sound) fprintf(stderr, "Audio unavailable: %s\n", SDL_GetError());
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    SDL_SetRenderVSync(renderer, 1);
    unsigned char pixels[PROTO_WIDTH * PROTO_HEIGHT * 4];
    ProtoFrame frame;
    ProtoAnimation animation;
    proto_animation_init(&animation, &config);
    ProtoHeldButton aria_hold = {0}, metroid_hold = {0};
    int selected = 0;
    update_title(window, &config, &animation, selected);
    bool running = true;
    uint64_t last = SDL_GetTicksNS();
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
                if (event.key.key == SDLK_X)
                    proto_press(&aria_hold, &animation, sound, true);
                else if (event.key.key == SDLK_Y)
                    proto_press(&metroid_hold, &animation, sound, false);
                else if (event.key.key == SDLK_ESCAPE) running = false;
                else key_down(&event.key, &config, &animation, &selected, config_path);
                update_title(window, &config, &animation, selected);
            } else if (event.type == SDL_EVENT_KEY_UP) {
                if (event.key.key == SDLK_X) proto_release(&aria_hold);
                if (event.key.key == SDLK_Y) proto_release(&metroid_hold);
            } else if (event.type == SDL_EVENT_GAMEPAD_ADDED) {
                if (!SDL_GetGamepadFromID(event.gdevice.which))
                    (void)SDL_OpenGamepad(event.gdevice.which);
            } else if (event.type == SDL_EVENT_GAMEPAD_REMOVED) {
                SDL_Gamepad *removed = SDL_GetGamepadFromID(event.gdevice.which);
                if (removed) SDL_CloseGamepad(removed);
                proto_release(&aria_hold);
                proto_release(&metroid_hold);
            } else if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
                if (event.gbutton.button == SDL_GAMEPAD_BUTTON_NORTH)
                    proto_press(&aria_hold, &animation, sound, true);
                else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_WEST)
                    proto_press(&metroid_hold, &animation, sound, false);
            } else if (event.type == SDL_EVENT_GAMEPAD_BUTTON_UP) {
                if (event.gbutton.button == SDL_GAMEPAD_BUTTON_NORTH)
                    proto_release(&aria_hold);
                else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_WEST)
                    proto_release(&metroid_hold);
            }
        }
        uint64_t now = SDL_GetTicksNS();
        float dt = (float)(now - last) / 1000000000.0f;
        last = now;
        if (dt > 0.10f) dt = 0.10f;
        proto_pulse_age_tick(&animation, dt);
        proto_hold_tick(&aria_hold, dt, &animation, sound, true);
        proto_hold_tick(&metroid_hold, dt, &animation, sound, false);
        proto_animation_tick(&animation, &config, dt);
        proto_make_frame(&config, &animation, &frame);
        draw(&config, &frame, pixels);
        SDL_UpdateTexture(texture, NULL, pixels, PROTO_WIDTH * 4);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }
    if (sound) SDL_DestroyAudioStream(sound);
    SDL_JoystickID *ids = SDL_GetGamepads(NULL);
    if (ids) {
        for (int j = 0; ids[j]; ++j) {
            SDL_Gamepad *p = SDL_GetGamepadFromID(ids[j]);
            if (p) SDL_CloseGamepad(p);
        }
        SDL_free(ids);
    }
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
