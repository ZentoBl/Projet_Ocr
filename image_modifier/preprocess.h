#pragma once

#include "base_img.h"

Uint8 grayscaleRGB(Uint8 r, Uint8 g, Uint8 b);

void gray_scale(SDL_Surface *img);

int cmp_Uint8(const void* a, const void* b);

void denoise_average(SDL_Surface *img, int dist);
void denoise_percent(SDL_Surface *img, int dist, unsigned char percent, char priority);
void denoise_median(SDL_Surface *img, int dist, char priority);
void denoise_morphology(SDL_Surface *img, int dist, unsigned char percent);
void denoise_opening(SDL_Surface *img, int dist);
void denoise_closing(SDL_Surface *img, int dist);

Uint8 get_threshold_Otsu(SDL_Surface *img);
void to_binary(SDL_Surface *img);

void denoise_by_count(SDL_Surface *img, int min_size);

void process(SDL_Surface *img);
