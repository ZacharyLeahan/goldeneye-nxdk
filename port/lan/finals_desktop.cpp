/* Windows bindings for the same fork controls compiled by the Xbox target. */
#include <SDL.h>
#include <cstdint>

#include "input.h"
#include "optionsoverlay.h"
#include "port/fast3d/gfx_api.h"
#include "port/fast3d/glad/glad.h"
#include "port/xbox/finals_controls.h"

// A key can go down and up between game ticks. Keep short direction taps
// visible to the simulation while held keys continue using SDL's live state.
static SDL_atomic_t directionPressed[SDL_NUM_SCANCODES];

static int SDLCALL rememberDirection(void *, SDL_Event *event)
{
    if (event->type == SDL_KEYDOWN && !event->key.repeat) {
        const SDL_Scancode key = event->key.keysym.scancode;
        switch (key) {
            case SDL_SCANCODE_A: case SDL_SCANCODE_D:
            case SDL_SCANCODE_W: case SDL_SCANCODE_S:
            case SDL_SCANCODE_LEFT: case SDL_SCANCODE_RIGHT:
            case SDL_SCANCODE_UP: case SDL_SCANCODE_DOWN:
                SDL_AtomicSet(&directionPressed[key], int(SDL_GetTicks()));
                break;
            default: break;
        }
    }
    return 0;
}

static bool directionHeld(const Uint8 *keys, SDL_Scancode key, Uint32 now)
{
    const Uint32 press = Uint32(SDL_AtomicGet(&directionPressed[key]));
    return keys[key] || (press && now - press < 100);
}

static int16_t keyboardAxis(int16_t stick, const Uint8 *keys, Uint32 now,
                            SDL_Scancode negativeKey, SDL_Scancode positiveKey)
{
    const bool negative = directionHeld(keys, negativeKey, now);
    const bool positive = directionHeld(keys, positiveKey, now);
    if (negative || positive) return 32767 * (int(positive) - int(negative));
    return stick;
}

extern "C" void xboxReadFinalsPad(int slot, FinalsPad *pad)
{
    *pad = {};
    if (optionsOverlayIsOpen()) return;
    pad->lx = inputPadAxis(slot, SDL_CONTROLLER_AXIS_LEFTX);
    pad->ly = inputPadAxis(slot, SDL_CONTROLLER_AXIS_LEFTY);
    pad->rx = inputPadAxis(slot, SDL_CONTROLLER_AXIS_RIGHTX);
    pad->ry = inputPadAxis(slot, SDL_CONTROLLER_AXIS_RIGHTY);
    if (inputPadButton(slot, SDL_CONTROLLER_BUTTON_A)) pad->buttons |= FIN_A;
    if (inputPadButton(slot, SDL_CONTROLLER_BUTTON_B)) pad->buttons |= FIN_B;
    if (inputPadButton(slot, SDL_CONTROLLER_BUTTON_X)) pad->buttons |= FIN_X;
    if (inputPadButton(slot, SDL_CONTROLLER_BUTTON_Y)) pad->buttons |= FIN_Y;
    if (inputPadButton(slot, SDL_CONTROLLER_BUTTON_LEFTSHOULDER)) pad->buttons |= FIN_WHEEL;
    if (inputPadButton(slot, SDL_CONTROLLER_BUTTON_LEFTSTICK)) pad->buttons |= FIN_SPRINT;
    if (inputPadButton(slot, SDL_CONTROLLER_BUTTON_RIGHTSTICK)) pad->buttons |= FIN_SLAP;
    if (inputPadAxis(slot, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > 4000) pad->buttons |= FIN_AIM;
    if (inputPadAxis(slot, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > 4000) pad->buttons |= FIN_FIRE;
    if (slot != 0) return;

    static bool watchingKeys;
    if (!watchingKeys) {
        SDL_AddEventWatch(rememberDirection, nullptr);
        watchingKeys = true;
    }
    if (SDL_GetKeyboardFocus()) {
        const Uint32 now = SDL_GetTicks();
        const Uint8 *keys = SDL_GetKeyboardState(nullptr);
        if ((SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_LMASK) || keys[SDL_SCANCODE_LCTRL])
            pad->buttons |= FIN_FIRE;
        if (keys[SDL_SCANCODE_R]) pad->buttons |= FIN_X;
        if (keys[SDL_SCANCODE_SPACE]) pad->buttons |= FIN_A;
        pad->lx = keyboardAxis(pad->lx, keys, now, SDL_SCANCODE_A, SDL_SCANCODE_D);
        pad->ly = keyboardAxis(pad->ly, keys, now, SDL_SCANCODE_W, SDL_SCANCODE_S);
        pad->rx = keyboardAxis(pad->rx, keys, now, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT);
        if (keys[SDL_SCANCODE_TAB]) pad->buttons |= FIN_WHEEL;
        if (pad->buttons & FIN_WHEEL) {
            pad->ry = keyboardAxis(pad->ry, keys, now, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN);
        } else {
            pad->ly = keyboardAxis(pad->ly, keys, now, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN);
        }
    }
}

extern "C" bool xboxDrawWheel(int slot, int left, int top, int width, int height,
                                const uint32_t *replacement)
{
    struct Surface { GLuint texture = 0; int width = 0, height = 0; };
    static Surface surfaces[4];
    static GLuint program, vao, vbo;
    if (!program) {
        const char *vertexSource =
            "#version 130\n"
            "in vec2 position; in vec2 texcoord; out vec2 uv;\n"
            "void main(){uv=texcoord;gl_Position=vec4(position,0,1);}";
        const char *fragmentSource =
            "#version 130\n"
            "uniform sampler2D image; in vec2 uv; out vec4 color;\n"
            "void main(){color=texture(image,uv);}";
        GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
        GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(vertex, 1, &vertexSource, NULL);
        glShaderSource(fragment, 1, &fragmentSource, NULL);
        glCompileShader(vertex);
        glCompileShader(fragment);
        GLint okVertex = 0, okFragment = 0;
        glGetShaderiv(vertex, GL_COMPILE_STATUS, &okVertex);
        glGetShaderiv(fragment, GL_COMPILE_STATUS, &okFragment);
        if (!okVertex || !okFragment) return false;
        program = glCreateProgram();
        glAttachShader(program, vertex);
        glAttachShader(program, fragment);
        glBindAttribLocation(program, 0, "position");
        glBindAttribLocation(program, 1, "texcoord");
        glLinkProgram(program);
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        GLint linked = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) { glDeleteProgram(program); program = 0; return false; }
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
    }
    if ((unsigned)slot >= 4 || width <= 0 || height <= 0) return false;
    Surface &surface = surfaces[slot];
    if (!replacement && (!surface.width || !surface.height)) return false;
    if (!surface.texture) glGenTextures(1, &surface.texture);
    GLint oldActive = 0, oldTexture = 0, oldProgram = 0, oldVao = 0, oldBuffer = 0;
    GLint oldBlendSrcRgb = 0, oldBlendDstRgb = 0, oldBlendSrcAlpha = 0, oldBlendDstAlpha = 0;
    GLint oldBlendEqRgb = 0, oldBlendEqAlpha = 0, oldUnpack = 0;
    GLboolean oldBlend = glIsEnabled(GL_BLEND), oldDepth = glIsEnabled(GL_DEPTH_TEST);
    GLboolean oldCull = glIsEnabled(GL_CULL_FACE), oldScissor = glIsEnabled(GL_SCISSOR_TEST);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &oldActive);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexture);
    glGetIntegerv(GL_CURRENT_PROGRAM, &oldProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &oldVao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &oldBuffer);
    glGetIntegerv(GL_BLEND_SRC_RGB, &oldBlendSrcRgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &oldBlendDstRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &oldBlendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &oldBlendDstAlpha);
    glGetIntegerv(GL_BLEND_EQUATION_RGB, &oldBlendEqRgb);
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &oldBlendEqAlpha);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldUnpack);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, surface.texture);
    if (replacement) {
        surface.width = width;
        surface.height = height;
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        /* Painter stores AARRGGBB words; little-endian memory is BGRA bytes. */
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                     GL_BGRA, GL_UNSIGNED_BYTE, replacement);
    }
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glViewport(left, int(gfx_current_dimensions.height) - top - height, width, height);
    const float vertices[] = {
        -1,1,0,0, 1,1,1,0, 1,-1,1,1,
        -1,1,0,0, 1,-1,1,1, -1,-1,0,1,
    };
    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "image"), 0);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    GLenum error = glGetError();
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

    if (oldBlend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (oldDepth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (oldCull) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (oldScissor) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    glBlendFuncSeparate(oldBlendSrcRgb, oldBlendDstRgb, oldBlendSrcAlpha, oldBlendDstAlpha);
    glBlendEquationSeparate(oldBlendEqRgb, oldBlendEqAlpha);
    glPixelStorei(GL_UNPACK_ALIGNMENT, oldUnpack);
    glBindTexture(GL_TEXTURE_2D, oldTexture);
    glActiveTexture(oldActive);
    glBindVertexArray(oldVao);
    glBindBuffer(GL_ARRAY_BUFFER, oldBuffer);
    glUseProgram(oldProgram);
    return error == GL_NO_ERROR;
}
