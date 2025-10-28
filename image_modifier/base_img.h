#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

SDL_Surface *load_img(const char *filename);

void save_img_bmp(SDL_Surface *img, const char *filename);
