#include "base_img.h"
#include <err.h>

SDL_Surface *load_img(const char *filename)
{
    SDL_Surface* img = IMG_Load(filename);
    if (!img)
        errx(EXIT_FAILURE, "%s\n", IMG_GetError());
    SDL_Surface *formatted = SDL_ConvertSurfaceFormat(img, SDL_PIXELFORMAT_RGB24, 0);
    SDL_FreeSurface(img);
    if (!formatted)
        errx(EXIT_FAILURE, "Unable to optimize image %s, %s\n", filename, SDL_GetError());
    return formatted;
}

void save_img_bmp(SDL_Surface *img, const char *filename)
{
    if (!img)
        errx(EXIT_FAILURE, "SDL_Surface NULL\n");
    if (SDL_SaveBMP(img, filename) != 0)
        errx(EXIT_FAILURE, "SDL_SaveBMP to %s : %s\n", filename, SDL_GetError());
}
