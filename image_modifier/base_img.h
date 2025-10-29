#pragma once

#include <SDL2/SDL.h>

SDL_Surface *load_img(const char *filename);

void save_img_name_bmp(SDL_Surface *img, const char *filename);

void create_parent_dirs(const char *path);

void save_img_bmp(SDL_Surface *img, const char *filepath);

