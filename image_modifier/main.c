#include <err.h>
#include <SDL2/SDL.h>
#include "preprocess.h"
#include "get_letters.h"

#define MAX_BOXES 2048

int main(int argc, char **argv)
{
    if (argc > 2)
        errx(EXIT_FAILURE, "argc = %i, used $ %s <inputfile> <outputfile>", argc, argv[0]);
    const char *name_file = (argc == 2) ? argv[1] : "level_1_image_1.png";

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

    Box *boxes = malloc(sizeof(Box) * MAX_BOXES);
    int *segments;
    size_t nb = detect_connected_components(img, boxes, MAX_BOXES, &segments);
    printf("%li boxes detected\n", nb);
    
    nb = split_large_boxes(img, boxes, nb, MAX_BOXES);
    printf("%li letters detected\n", nb);

    save_boxes(img, boxes, nb, input);
    
    GridLetter *gl = malloc(sizeof(GridLetter) * MAX_BOXES);
    size_t rows, cols;
    size_t ngl = group_letters_into_grid(img, boxes, nb, gl, MAX_BOXES, &rows, &cols);
    printf("Grid detected: %li letters, rows=%li cols=%li\n", ngl, rows, cols);
    save_grid_letters(img, gl, ngl, name_file);

/*
    WordLetter *wl = malloc(sizeof(WordLetter) * MAX_BOXES);
    size_t words_count;
    size_t nwl = group_letters_into_words(boxes, nb, wl, MAX_BOXES, &words_count);
    printf("Found %li word-letters in %li words \n", nwl, words_count);
    save_word_letters(img, wl, nb, name_file);
*/

    free(segments);
    free(boxes);
    free(gl);
    //free(wl);
    
    SDL_FreeSurface(img);
    SDL_Quit();
    return EXIT_SUCCESS;
}
