#pragma once

#include "base_img.h"

typedef struct {
    int x, y, w, h;
    size_t cx, cy; // center
    size_t nb_pixels;
} Box;

typedef struct {
    Box box;
    size_t letter_idx;
    size_t word_id;
} WordLetter;

typedef struct {
    Box box;
    size_t row, col;
} GridLetter;

size_t detect_connected_components(SDL_Surface *img, Box *boxes, size_t max_boxes, int **out_segments);

void save_boxes(SDL_Surface *img, Box *boxes, size_t nb, char *parent_name);

void save_grid_letters(SDL_Surface *img, GridLetter *gl, size_t nb, const char *parent_name);
void save_word_letters(SDL_Surface *img, WordLetter *wl, size_t nb, const char *parent_name);

size_t split_large_boxes(SDL_Surface *img, Box *boxes, size_t n, size_t max_boxes);

// cluster rows by cy, then cols, assign row/col
size_t group_letters_into_grid(SDL_Surface *img, Box *boxes, size_t n, GridLetter *out_letters, size_t max_letters, size_t *out_row_count, size_t *out_col_count);

// group by cy within tol_y, then sort by cx
size_t group_letters_into_words(Box *boxes, size_t n, WordLetter *out_letters, size_t max_letters, size_t *out_words_count);

void save_with_marks(SDL_Surface *img, Box *boxes, size_t *indices, size_t nx, size_t ny, const char *parent_name);
