#include "map.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

// Properly define the global variables
int** world_map = NULL;
int map_width = 0;
int map_height = 0;

float posX = 3.5f, posY = 3.5f;
float dirX = 1.0f, dirY = 0.0f;
float planeX = 0.0f, planeY = 0.66f;

void free_map() {
    if(world_map) {
        for(int i = 0; i < map_height; i++) {
            free(world_map[i]);
        }
        free(world_map);
        world_map = NULL;
    }
}

int load_map(const char* filename) {
    FILE* file = fopen(filename, "r");
    if(!file) return 0;

    char line[256];
    int section = 0;
    int row = 0;

    while(fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;

        if(strcmp(line, "[metadata]") == 0) {
            section = 1;
        }
        else if(strcmp(line, "[player]") == 0) {
            section = 2;
        }
        else if(strcmp(line, "[map]") == 0) {
            section = 3;
            // Allocate 2D array
            world_map = malloc(map_height * sizeof(int*));
            for(int i = 0; i < map_height; i++) {
                world_map[i] = malloc(map_width * sizeof(int));
            }
        }
        else if(section == 1) {
            if(sscanf(line, "width=%d", &map_width) == 1) continue;
            if(sscanf(line, "height=%d", &map_height) == 1) continue;
        }
        else if(section == 2) {
            float angle;
            if(sscanf(line, "posX=%f", &posX) == 1) continue;
            if(sscanf(line, "posY=%f", &posY) == 1) continue;
            if(sscanf(line, "angle=%f", &angle) == 1) {
                float rad = angle * (M_PI / 180.0f);
                dirX = cos(rad);
                dirY = sin(rad);
                planeX = -dirY * 0.66f;
                planeY = dirX * 0.66f;
                continue;
            }
        }
        else if(section == 3) {
            if(row >= map_height) continue;
            
            char* token = strtok(line, " ");
            for(int col = 0; col < map_width && token; col++) {
                world_map[row][col] = atoi(token);
                token = strtok(NULL, " ");
            }
            row++;
        }
    }

    fclose(file);
    return 1;
}