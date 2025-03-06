#ifndef RENDER_H
#define RENDER_H

#include "map.h"
#include "graphic.h"

#define MAX_ENTITIES 100
#define MAX_TEXTURES 2

typedef struct {
    float x, y;
    float dx, dy;
    float move_timer;
    float distance;
    int texture_id;
    int visible;
    int is_chaser;
} Entity;

typedef struct {
    int health;
    int ammo;
    int score;
    int lives;
    float pickup_flash_timer;
} UIState;

typedef enum {
    WEAPON_IDLE,
    WEAPON_FIRING
} WeaponState;

extern WeaponState weapon_state;

extern UIState ui;

extern Texture textures[MAX_TEXTURES];
extern Entity entities[MAX_ENTITIES];
extern int entity_count;

void init_renderer(SDL_Window* window, SDL_Renderer** renderer, SDL_Texture** texture);
void render_frame(SDL_Renderer* renderer, SDL_Texture* screen_texture);
void cleanup_renderer(SDL_Renderer* renderer, SDL_Texture* texture);

#endif