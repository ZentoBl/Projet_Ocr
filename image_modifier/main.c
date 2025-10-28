#include <err.h>
#include "preprocess.h"
#include "get_letters.h"

int main(int argc, char **argv)
{
    if (argc > 2)
        errx(EXIT_FAILURE, "argc = %i, used $ %s <inputfile> <outputfile>", argc, argv[0]);
    char *name_file;
    if (argc == 2)
        name_file = argv[1];
    else
        name_file = "level_1_image_1.png";

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        errx(EXIT_FAILURE, "SDL_Init failed");
    
    char input[64] = "";
    sprintf(input, "tests_images/%s", name_file);
    SDL_Surface *img = load_img(input);
    printf("image %s : %i x %i\n", name_file, img->w, img->h);

    process(img);

    char black_white_name[64] = "";
    sprintf(black_white_name, "black_and_white/%s_black_white.bmp", name_file);
    save_img_bmp(img, black_white_name);

    save_letters(img, name_file);

    SDL_FreeSurface(img);
    IMG_Quit();
    SDL_Quit();
    return EXIT_SUCCESS;
}