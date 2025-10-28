#pragma once

#include "base_img.h"

typedef struct {
    int x, y, w, h;
} BoundingBox;

typedef struct {
    int row, col;
    BoundingBox box;
} GridLetter;


void fill(SDL_Surface *img, int x, int y, char *visited, int *x_min, int *x_max, int *y_min, int *y_max);

size_t detect_connected(SDL_Surface *img, BoundingBox *boxes, size_t max_boxes);

void save_letters(SDL_Surface *img, char *parent_name);
