#ifndef LETTER_PARSER_H
#define LETTER_PARSER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

typedef struct {
    int col, row;
    int x, y, w, h;
} LetterInfo;

int parse_letter_filename(const char *filename, LetterInfo *out);

LetterInfo *load_letters_from_folder(const char *folder, int *nb_out);

LetterInfo *find_letter(LetterInfo *arr, int nb, int col, int row);
LetterInfo *get_first_letter(LetterInfo *arr, int nb, int word_id);
LetterInfo *get_last_letter(LetterInfo *arr, int nb, int word_id);

#endif
