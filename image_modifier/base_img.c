#include <err.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "base_img.h"

#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(path) _mkdir(path)
    #define PATH_SEP '\\'
#else
    #include <sys/stat.h>
    #define MKDIR(path) mkdir(path, 0755)
    #define PATH_SEP '/'
#endif

SDL_Surface *load_img(const char *filename)
{
    if (!filename)
        errx(EXIT_FAILURE, "filename NULL\n");
    SDL_Surface* img = IMG_Load(filename);
    if (!img)
        errx(EXIT_FAILURE, "%s\n", SDL_GetError());
    SDL_Surface *formatted = SDL_ConvertSurfaceFormat(img, SDL_PIXELFORMAT_RGB24, 0);
    SDL_FreeSurface(img);
    if (!formatted)
        errx(EXIT_FAILURE, "Unable to optimize image %s, %s\n", filename, SDL_GetError());
    return formatted;
}

void save_img_name_bmp(SDL_Surface *img, const char *filename)
{
    if (!img || !filename)
        errx(EXIT_FAILURE, "save_img_name_bmp surface=%p, filename==%p", img, filename);
    if (SDL_SaveBMP(img, filename) != 0)
        errx(EXIT_FAILURE, "SDL_SaveBMP to %s : %s\n", filename, SDL_GetError());
}

void create_parent_dirs(const char *path)
{
    char tmp[512];
    strncpy(tmp, path, sizeof(tmp));
    for (char *p = tmp + 1; *p; p++)
    {
        if (*p == PATH_SEP)
        {
            *p = '\0';
            if (MKDIR(tmp) != 0 && errno != EEXIST)
                errx(EXIT_FAILURE, "failed to create folder %s", tmp);
            *p = PATH_SEP;
        }
    }
}

void save_img_bmp(SDL_Surface *img, const char *filepath)
{
    if (!img || !filepath)
        errx(EXIT_FAILURE, "save_img_bmp surface=%p, filepath==%p", img, filepath);
    create_parent_dirs(filepath);
    save_img_name_bmp(img, filepath);
}
