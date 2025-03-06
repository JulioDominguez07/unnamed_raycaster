#ifndef GRAPHIC_H
#define GRAPHIC_H

#include <stdint.h>
#include <SDL2/SDL.h>

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define FONT_CHAR_WIDTH 8
#define FONT_CHAR_HEIGHT 8

#define TEX_SIZE 64

typedef struct {
    Uint32* pixels;
    int width;
    int height;
} Texture;

extern uint32_t framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT];

void plot(uint16_t x, uint16_t y, uint32_t color);
void line(int x0, int y0, int x1, int y1, uint32_t color);

int load_texture(const char* path, Texture* tex);

void draw_char(uint16_t x, uint16_t y, char c, uint32_t color);
void draw_string(uint16_t x, uint16_t y, const char* str, uint32_t color);

#endif