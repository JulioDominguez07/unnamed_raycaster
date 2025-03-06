#include "render.h"
#include <SDL2/SDL.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

enum TEXTURE_IDS { TEX_WALL, TEX_ENTITY, TEX_WEAPON };

Texture textures[MAX_TEXTURES];
Entity entities[MAX_ENTITIES];
int entity_count = 0;

WeaponState weapon_state = WEAPON_IDLE;
int weapon_frame = 0;

float fast_inv_sqrt(float x) {
    union { float f; uint32_t i; } conv = {x};
    conv.i = 0x5f3759df - (conv.i >> 1);
    conv.f *= 1.5f - (x * 0.5f * conv.f * conv.f);
    return conv.f;
}

void init_renderer(SDL_Window* window, SDL_Renderer** renderer, SDL_Texture** texture) {
    *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    *texture = SDL_CreateTexture(*renderer, 
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 
        SCREEN_WIDTH, SCREEN_HEIGHT);
}

void render_walls() {
    for(int x = 0; x < SCREEN_WIDTH; x++) {
        // Raycasting calculations
        float cameraX = 2 * x / (float)SCREEN_WIDTH - 1;
        float rayDirX = dirX + planeX * cameraX;
        float rayDirY = dirY + planeY * cameraX;

        // Normalize ray direction
        float len_sq = rayDirX*rayDirX + rayDirY*rayDirY;
        float inv_len = fast_inv_sqrt(len_sq);
        rayDirX *= inv_len;
        rayDirY *= inv_len;

        // DDA algorithm
        int mapX = (int)posX;
        int mapY = (int)posY;
        float deltaDistX = fabsf(1 / rayDirX);
        float deltaDistY = fabsf(1 / rayDirY);
        
        float sideDistX, sideDistY;
        int stepX, stepY;
        int hit = 0, side;

        if(rayDirX < 0) {
            stepX = -1;
            sideDistX = (posX - mapX) * deltaDistX;
        } else {
            stepX = 1;
            sideDistX = (mapX + 1.0f - posX) * deltaDistX;
        }
        if(rayDirY < 0) {
            stepY = -1;
            sideDistY = (posY - mapY) * deltaDistY;
        } else {
            stepY = 1;
            sideDistY = (mapY + 1.0f - posY) * deltaDistY;
        }

        while(!hit) {
            if(sideDistX < sideDistY) {
                sideDistX += deltaDistX;
                mapX += stepX;
                side = 0;
            } else {
                sideDistY += deltaDistY;
                mapY += stepY;
                side = 1;
            }
            if(world_map[mapX][mapY] > 0) hit = 1;
        }
        
        // Wall rendering code
        // Calculate distance and wall position
        float perpWallDist = side ? 
        (mapY - posY + (1 - stepY)/2.0f) / rayDirY :
        (mapX - posX + (1 - stepX)/2.0f) / rayDirX;
    
        int lineHeight = (int)(SCREEN_HEIGHT / perpWallDist);
        int drawStart = -lineHeight / 2 + SCREEN_HEIGHT / 2;
        int drawEnd = lineHeight / 2 + SCREEN_HEIGHT / 2;

        // Texture calculations
        float wallX;
        if(side == 0) wallX = posY + perpWallDist * rayDirY;
        else wallX = posX + perpWallDist * rayDirX;
        wallX -= floor(wallX);

        int texX = (int)(wallX * TEX_SIZE);
        if((side == 0 && rayDirX > 0) || (side == 1 && rayDirY < 0))
            texX = TEX_SIZE - texX - 1;

        float step = 1.0f * TEX_SIZE / lineHeight;
        float texPos = (drawStart - SCREEN_HEIGHT/2 + lineHeight/2) * step;
        
        // Texture mapping
        for(int y = drawStart; y < drawEnd; y++) {
            int texY = (int)texPos & (TEX_SIZE - 1);
            texPos += step;
            Uint32 color = textures[TEX_WALL].pixels[TEX_SIZE * texY + texX];
            plot(x, y, color);
        }
    }
}

void render_entities() {
    for(int i = 0; i < entity_count; i++) {
        float dx = entities[i].x - posX;
        float dy = entities[i].y - posY;
        entities[i].distance = dx*dx + dy*dy;
    }

    // Bubble sort by distance
    for(int i = 0; i < entity_count-1; i++) {
        for(int j = 0; j < entity_count-i-1; j++) {
            if(entities[j].distance < entities[j+1].distance) {
                Entity temp = entities[j];
                entities[j] = entities[j+1];
                entities[j+1] = temp;
            }
        }
    }

    for(int i = 0; i < entity_count; i++) {
        if(!entities[i].visible) continue;
        
        Texture* tex = &textures[entities[i].texture_id];
        float spriteX = entities[i].x - posX;
        float spriteY = entities[i].y - posY;
        
        float invDet = 1.0f / (planeX * dirY - dirX * planeY);
        float transformX = invDet * (dirY * spriteX - dirX * spriteY);
        float transformY = invDet * (-planeY * spriteX + planeX * spriteY);

        int spriteScreenX = (int)((SCREEN_WIDTH / 2) * (1 + transformX / transformY));
        int spriteHeight = abs((int)(SCREEN_HEIGHT / transformY));
        int drawStartY = -spriteHeight / 2 + SCREEN_HEIGHT / 2;
        int drawEndY = spriteHeight / 2 + SCREEN_HEIGHT / 2;
        int drawStartX = -spriteHeight / 2 + spriteScreenX;
        int drawEndX = spriteHeight / 2 + spriteScreenX;

        for(int stripe = drawStartX; stripe < drawEndX; stripe++) {
            if(stripe >= 0 && stripe < SCREEN_WIDTH && transformY > 0) {
                int texX = (int)((stripe - drawStartX) * TEX_SIZE / (float)spriteHeight);
                for(int y = drawStartY; y < drawEndY; y++) {
                    if(y >= 0 && y < SCREEN_HEIGHT) {
                        int texY = (int)((y - drawStartY) * TEX_SIZE / (float)spriteHeight);
                        Uint32 color = tex->pixels[TEX_SIZE * texY + texX];
                        // Skip magenta (0xFF00FF) transparent pixels
                        if((color & 0xFFFFFF) != 0xFF00FF) {
                            plot(stripe, y, color);
                        }
                    }
                }
            }
        }
    }
}

void handle_window_resize(SDL_Renderer* renderer, SDL_Texture** texture) {
    SDL_DestroyTexture(*texture);
    *texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH, SCREEN_HEIGHT);
}

void render_ui() {
    char buffer[32];
    uint32_t text_color = 0xFFFFFF; // White
    int y_pos = SCREEN_HEIGHT - 30; // 10 pixels from bottom

    // Calculate column width (1/4 of screen)
    int col_width = SCREEN_WIDTH / 4;
    int padding = 15;  // Minimum space from screen edges

    // Draw what ever this is called
    for(int y = y_pos - 10; y < SCREEN_HEIGHT; y++) {
        for(int x = 0; x < SCREEN_WIDTH; x++) {
            plot(x, y, 0x000000);
        }
    }

    // Health percentage (left)
    snprintf(buffer, sizeof(buffer), "HP");
    draw_string(padding, y_pos, buffer, text_color);
    snprintf(buffer, sizeof(buffer), "%3d%%", ui.health);
    draw_string(padding, y_pos + 10, buffer, text_color);

    // Score (center)
    snprintf(buffer, sizeof(buffer), "SCORE");
    draw_string(col_width + padding, y_pos, buffer, text_color);
    snprintf(buffer, sizeof(buffer), "%06d", ui.score);
    draw_string(col_width + padding, y_pos + 10, buffer, text_color);

    // Ammo & Lives (right)
    snprintf(buffer, sizeof(buffer), "AMMO");
    draw_string(col_width * 2 + padding, y_pos, buffer, text_color);
    snprintf(buffer, sizeof(buffer), "%03d", ui.ammo);
    draw_string(col_width * 2 + padding, y_pos + 10, buffer, text_color);
    
    snprintf(buffer, sizeof(buffer), "LIVES");
    draw_string(col_width * 3 + padding, y_pos, buffer, text_color);
     snprintf(buffer, sizeof(buffer), "%02d", ui.lives);
    draw_string(col_width * 3 + padding, y_pos + 10, buffer, text_color);
}

void update_weapon_animation() {
    switch(weapon_state) {
        case WEAPON_IDLE:
            // Gentle sway
            break;
        case WEAPON_FIRING:
            // Recoil animation
            weapon_frame++;
            if(weapon_frame > 10) {
                weapon_state = WEAPON_IDLE;
                weapon_frame = 0;
            }
            break;
    }
}


void render_weapon() {
    Texture* tex = &textures[TEX_WEAPON];
    int screen_bottom = SCREEN_HEIGHT - 10;
    
    // Weapon dimensions
    int frame_width = 64; // Each frame is 64x64
    int frame_height = 64;
    int weapon_height = SCREEN_HEIGHT / 2;
    int weapon_width = (weapon_height * frame_width) / frame_height * 1.2;
    int x_pos = (SCREEN_WIDTH - weapon_width) / 2;
    int y_pos = screen_bottom - weapon_height - 30;

    // Animation state
    static int current_frame = 0;
    static int animation_timer = 0;
    const int frames_per_row = 5; // 320px / 64px = 5 frames
    const int animation_speed = 5; // Frames per animation step

    // Update animation
    if(weapon_state == WEAPON_FIRING) {
        animation_timer++;
        if(animation_timer >= animation_speed) {
            current_frame++;
            animation_timer = 0;
            
            if(current_frame >= frames_per_row) {
                current_frame = 0;
                weapon_state = WEAPON_IDLE;
            }
        }
    }

    // Calculate frame position
    int frame_x = current_frame * frame_width;

    // Render current frame
    float scale_x = (float)weapon_width / frame_width;
    float scale_y = (float)weapon_height / frame_height;
    
    for(int y = 0; y < weapon_height; y++) {
        for(int x = 0; x < weapon_width; x++) {
            int tex_x = frame_x + (int)(x / scale_x);
            int tex_y = (int)(y / scale_y);
            uint32_t color = tex->pixels[tex_y * tex->width + tex_x];
            
            if((color & 0xFFFFFF) != 0xFF00FF) {
                plot(x_pos + x, y_pos + y, color);
            }
        }
    }
}

void render_frame(SDL_Renderer* renderer, SDL_Texture* screen_texture) {
    static Uint32 last_frame_time = 0;
    float delta_time = (SDL_GetTicks() - last_frame_time) / 1000.0f;
    last_frame_time = SDL_GetTicks();

    // Clear framebuffer
    memset(framebuffer, 0, sizeof(framebuffer));
    
    static int last_w = 0, last_h = 0;
    int w, h;
    SDL_GetWindowSize(SDL_GetWindowFromID(SDL_GetWindowID(renderer)), &w, &h);
    if(w != last_w || h != last_h) {
        last_w = w;
        last_h = h;
        handle_window_resize(renderer, &screen_texture);
    }

    uint32_t dark_gray = 0x202020;  // Dark gray (RGB: 32,32,32)
    uint32_t light_gray = 0x404040; // Light gray (RGB: 64,64,64)
    int split_point = SCREEN_HEIGHT / 2; // Split screen in half

    // Draw top half (dark gray)
    for(int y = 0; y < split_point; y++) {
        for(int x = 0; x < SCREEN_WIDTH; x++) {
            plot(x, y, dark_gray);
        }
    }

    // Draw bottom half (light gray)
    for(int y = split_point; y < SCREEN_HEIGHT; y++) {
        for(int x = 0; x < SCREEN_WIDTH; x++) {
            plot(x, y, light_gray);
        }
    }

    render_walls();
    render_entities();
    render_ui();
    render_weapon();

    if(ui.pickup_flash_timer > 0) {
        uint32_t flash_color = 0xFFED29; // Green with 50% alpha
        for(int y = 0; y < SCREEN_HEIGHT; y++) {
            for(int x = 0; x < SCREEN_WIDTH; x++) {
                // Blend with existing pixel
                uint32_t bg = framebuffer[y * SCREEN_WIDTH + x];
                framebuffer[y * SCREEN_WIDTH + x] = 
                    ((bg & 0xFEFEFE) >> 1) + ((flash_color & 0xFEFEFE) >> 1);
            }
        }
        ui.pickup_flash_timer -= delta_time;
    }

    if(ui.pickup_flash_timer > 0) ui.pickup_flash_timer -= delta_time;

    // Update SDL texture
    void* pixels;
    int pitch;
    SDL_LockTexture(screen_texture, NULL, &pixels, &pitch);
    memcpy(pixels, framebuffer, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    SDL_UnlockTexture(screen_texture);
    SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void cleanup_renderer(SDL_Renderer* renderer, SDL_Texture* texture) {
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
}