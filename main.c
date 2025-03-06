#include "include/graphic.h"
#include "include/map.h"
#include "include/render.h"
#include <SDL2/SDL.h>
#include <math.h>
#include <stdlib.h>

enum TEXTURE_IDS { TEX_WALL, TEX_ENTITY, TEX_WEAPON, TEX_AMMO };
UIState ui = {100, 30, 0, 3};

void randomize_entity_direction(Entity* e) {
    float angle = (rand() % 360) * (M_PI / 180.0f);
    float speed = 0.02f; // Adjust movement speed
    e->dx = cos(angle) * speed;
    e->dy = sin(angle) * speed;
    e->move_timer = (rand() % 100) / 20.0f + 1.0f; // 1-6 seconds
}

void move_entity(Entity* e) {
    float new_x = e->x + e->dx;
    float new_y = e->y + e->dy;
    
    // Check X movement
    if(world_map[(int)new_x][(int)e->y] == 0) {
        e->x = new_x;
    } else {
        e->dx *= -1; // Bounce off wall
    }
    
    // Check Y movement
    if(world_map[(int)e->x][(int)new_y] == 0) {
        e->y = new_y;
    } else {
        e->dy *= -1; // Bounce off wall
    }
}

void chase_player(Entity* e) {
    float chase_speed = 0.03f;
    float dx = posX - e->x;
    float dy = posY - e->y;
    float dist = sqrtf(dx*dx + dy*dy);
    
    if(dist > 1.5f) { // Stop when close
        // Normalize direction
        float inv_dist = 1.0f / dist;
        e->dx = dx * inv_dist * chase_speed;
        e->dy = dy * inv_dist * chase_speed;
        
        // Move with collision check
        float new_x = e->x + e->dx;
        float new_y = e->y + e->dy;
        
        if(world_map[(int)new_x][(int)e->y] == 0) e->x = new_x;
        if(world_map[(int)e->x][(int)new_y] == 0) e->y = new_y;
    }
}

void init_entities() {
    // Regular entities
    for(int i = 0; i < 4; i++) {
        entities[entity_count] = (Entity){
            .x = (rand() % (map_height-2)) + 1.5f,
            .y = (rand() % (map_width-2)) + 1.5f,
            .texture_id = TEX_ENTITY,
            .visible = 1
        };
        randomize_entity_direction(&entities[entity_count]);
        entity_count++;
    }

    for(int i = 0; i < rand() % map_height; i++) {
        entities[entity_count] = (Entity){
            .x = (rand() % (map_height-2)) + 1.5f,
            .y = (rand() % (map_width-2)) + 1.5f,
            .dx = 0.0f,
            .dy = 0.0f,
            .texture_id = TEX_AMMO,
            .visible = 1
        };
        entity_count++;
    }
    
    // Chaser entity
    entities[entity_count] = (Entity){
        .x = 5.5f,
        .y = 5.5f,
        .texture_id = TEX_ENTITY,
        .visible = 1,
        .is_chaser = 1  // Mark as chaser
    };
    entity_count++;
}

void player_take_damage(int damage) {
    ui.health -= damage;
    if(ui.health < 0) ui.health = 0;
}

void player_add_score(int points) {
    ui.score += points;
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Demo", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH*2, SCREEN_HEIGHT*2,0);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Texture* screen_texture = SDL_CreateTexture(renderer, 
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 
        SCREEN_WIDTH, SCREEN_HEIGHT);

    if (!load_map("demo.map")) {
        SDL_Log("Failed to load map!");
        return 1;
    }

    if (!load_texture("texture/wall.bmp", &textures[TEX_WALL]) || 
        !load_texture("texture/entity.bmp", &textures[TEX_ENTITY]) ||
        !load_texture("texture/weapon.bmp", &textures[TEX_WEAPON]) ||
        !load_texture("texture/ammo.bmp", &textures[TEX_AMMO])) {
        SDL_Log("Failed to load textures!");
        return 1;
    }
    init_entities();

    Uint32 last_time = SDL_GetTicks();
    int running = 1;
    
    while(running) {
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) {
                running = 0;
            }
            else if(event.type == SDL_KEYDOWN) {
                // Fullscreen toggle
                if(event.key.keysym.sym == SDLK_F1) {
                    Uint32 flags = SDL_GetWindowFlags(window);
                    if(flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
                        SDL_SetWindowFullscreen(window, 0);
                    } else {
                        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
                    }
                }

                if(event.key.keysym.sym == SDLK_LCTRL || event.key.keysym.sym == SDLK_RCTRL && weapon_state != WEAPON_FIRING) {
                    weapon_state = WEAPON_FIRING;
                    ui.ammo -= 1;
                    if(ui.ammo < 0) {
                        ui.ammo = 0;
                        weapon_state = WEAPON_IDLE;
                    }
                }
            }
        }

        // Handle input with collision detection
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        float delta_time = (SDL_GetTicks() - last_time) / 1000.0f;
        float moveSpeed = 0.05f;
        float rotSpeed = 0.03f;

        if(keys[SDL_SCANCODE_LSHIFT]) {
            moveSpeed += 0.05f;
        } else {
            moveSpeed = 0.05f;
        }
        
        // Movement with collision
        if(keys[SDL_SCANCODE_UP]) {
            float newPosX = posX + dirX * moveSpeed;
            float newPosY = posY + dirY * moveSpeed;
            if(newPosX >= 0 && newPosX < map_width && newPosY >= 0 && newPosY < map_height) {
                if(world_map[(int)newPosX][(int)posY] == 0) posX = newPosX;
                if(world_map[(int)posX][(int)newPosY] == 0) posY = newPosY;
            }
        }
        if(keys[SDL_SCANCODE_DOWN]) {
            float newPosX = posX - dirX * moveSpeed;
            float newPosY = posY - dirY * moveSpeed;
            if(newPosX >= 0 && newPosX < map_width && newPosY >= 0 && newPosY < map_height) {
                if(world_map[(int)newPosX][(int)posY] == 0) posX = newPosX;
                if(world_map[(int)posX][(int)newPosY] == 0) posY = newPosY;
            }
        }
        
        // Rotation
        if(keys[SDL_SCANCODE_RIGHT]) {
            float oldDirX = dirX;
            dirX = dirX * cos(rotSpeed) - dirY * sin(rotSpeed);
            dirY = oldDirX * sin(rotSpeed) + dirY * cos(rotSpeed);
            float oldPlaneX = planeX;
            planeX = planeX * cos(rotSpeed) - planeY * sin(rotSpeed);
            planeY = oldPlaneX * sin(rotSpeed) + planeY * cos(rotSpeed);
        }
        if(keys[SDL_SCANCODE_LEFT]) {
            float oldDirX = dirX;
            dirX = dirX * cos(-rotSpeed) - dirY * sin(-rotSpeed);
            dirY = oldDirX * sin(-rotSpeed) + dirY * cos(-rotSpeed);
            float oldPlaneX = planeX;
            planeX = planeX * cos(-rotSpeed) - planeY * sin(-rotSpeed);
            planeY = oldPlaneX * sin(-rotSpeed) + planeY * cos(-rotSpeed);
        }

        // Update entities
        for(int i = 0; i < entity_count; i++) {
            // Skip movement logic for ammo pickups
            if(entities[i].texture_id == TEX_AMMO) continue;

            if(entities[i].is_chaser) {
                chase_player(&entities[i]);
            } else {
                entities[i].move_timer -= delta_time;
                if(entities[i].move_timer <= 0) {
                    randomize_entity_direction(&entities[i]);
                }
                move_entity(&entities[i]);
            }

            // Keep within bounds (for moving entities only)
            entities[i].x = fmax(1.1f, fmin(map_height-1.1f, entities[i].x));
            entities[i].y = fmax(1.1f, fmin(map_width-1.1f, entities[i].y));
        }

        for(int i = 0; i < entity_count; i++) {
            if(entities[i].texture_id == TEX_AMMO && entities[i].visible) {
                float dx = entities[i].x - posX;
                float dy = entities[i].y - posY;
                float dist = sqrtf(dx*dx + dy*dy);
        
                if(dist < 0.7f) { // Pickup radius
                    ui.ammo += 15;
                    entities[i].visible = 0; // Remove pickup
                    player_add_score(50);

                    ui.pickup_flash_timer = 0.3f; // 0.3 seconds of flash
                }
            }
        }

        // Update SDL texture
        render_frame(renderer, screen_texture);

        // Frame rate control
        Uint32 current_time = SDL_GetTicks();
        if(current_time - last_time < 16) SDL_Delay(16 - (current_time - last_time));
        last_time = current_time;
    }

    for(int i = 0; i < MAX_TEXTURES; i++) {
        free(textures[i].pixels);
    }

    free_map();

    SDL_DestroyTexture(screen_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}