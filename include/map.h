#ifndef MAP_H
#define MAP_H

#include <stdio.h>

extern int** world_map;
extern int map_width;
extern int map_height;

extern float posX, posY;
extern float dirX, dirY;
extern float planeX, planeY;

void free_map();
int load_map(const char* filename);

#endif