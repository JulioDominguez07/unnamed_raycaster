#include "graphic.h"
#include "font.h"
#include <SDL2/SDL.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

uint32_t framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT];

void plot(uint16_t x, uint16_t y, uint32_t color) {
    if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) return;
    framebuffer[y * SCREEN_WIDTH + x] = color;
}

void line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = abs(x1 - x0);
    int dy = -abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (1) {
        plot(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

int load_texture(const char* path, Texture* tex) {
    SDL_Surface* surface = SDL_LoadBMP(path);
    if (!surface) return 0;

    SDL_Surface* converted = SDL_ConvertSurfaceFormat(
        surface, SDL_PIXELFORMAT_ARGB8888, 0
    );
    SDL_FreeSurface(surface);

    if (!converted) return 0;
    
    tex->width = converted->w;
    tex->height = converted->h;
    tex->pixels = malloc(tex->width * tex->height * sizeof(Uint32));
    
    SDL_LockSurface(converted);
    memcpy(tex->pixels, converted->pixels, 
           tex->width * tex->height * sizeof(Uint32));
    SDL_UnlockSurface(converted);
    SDL_FreeSurface(converted);
    
    return 1;
}

void draw_char(uint16_t x, uint16_t y, char c, uint32_t color) {
    // Use 0x20-0x7E range
    if(c < 0x20 || c > 0x7E) return; // Only render printable ASCII
    
    const uint8_t* glyph = font_bitmap[(unsigned char)c];
    for(int row = 0; row < 8; row++) {
        for(int col = 0; col < 8; col++) {
            if(glyph[row] & (1 << col)) {
                plot(x + col, y + row, color);
            }
        }
    }
}

void draw_string(uint16_t x, uint16_t y, const char* str, uint32_t color) {
    uint16_t current_x = x;
    while(*str) {
        draw_char(current_x, y, *str++, color);
        current_x += FONT_CHAR_WIDTH + 1;
    }
}