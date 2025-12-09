#include "letter_parser.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

int parse_letter_filename(const char *filename, LetterInfo *out)
{
    // "_<id>_<col>x<row>_(<x>x<y>_<w>x<h>).bmp"
    const char *p = filename + strlen(filename);
    while (p > filename && *p != '_') p--;
    p--;
    while (p > filename && *p != '_') p--;
    p--;
    while (p > filename && *p != '_') p--;
    p--;
    while (p > filename && *p != '_') p--;
    int id, col, row,  x, y, w, h;
    if(sscanf(p, "_%d_%dx%d_(%dx%d_%dx%d)", &id, &col, &row, &x, &y, &w, &h) != 7) 
    {
        fprintf(stderr, "failed to parce : (%s) -> (%s)", filename, p);
        return EXIT_FAILURE;
    }
    out->col = col;
    out->row = row;
    out->x = x;
    out->y = y;
    out->w = w;
    out->h = h;
    return EXIT_SUCCESS;
}

LetterInfo *load_letters_from_folder(const char *folder, int *nb_out)
{
    DIR *d = opendir(folder);
    if (!d)
    {
        perror("opendir");
        *nb_out = 0;
        return NULL;
    }
    LetterInfo *arr = NULL;
    int count = 0;
    struct dirent *ent;
    while ((ent = readdir(d)))
    {
        if (ent->d_name[0] == '.') continue;
        LetterInfo info;
        if (parse_letter_filename(ent->d_name, &info))
            continue;
        arr = realloc(arr, sizeof(LetterInfo) * (count + 1));
        arr[count++] = info;
    }
    closedir(d);
    *nb_out = count;
    return arr;
}

LetterInfo *find_letter(LetterInfo *arr, int nb, int col, int row)
{
    for (int i = 0; i < nb; i++)
    {
        if (arr[i].col == col && arr[i].row == row)
            return &arr[i];
    }
    return NULL;
}

LetterInfo *get_first_letter(LetterInfo *arr, int nb, int word_id)
{
    for (int i = 0; i < nb; i++)
    {
        if (arr[i].row == word_id && arr[i].col == 0)
            return &arr[i];
    }
    return NULL;
}

LetterInfo *get_last_letter(LetterInfo *arr, int nb, int word_id)
{
    int max_col = -1;
    int best = -1;
    for (int i = 0; i < nb; i++)
    {
        if (arr[i].row == word_id && arr[i].col > max_col)
        {
            max_col = arr[i].col;
            best = i;
        }
    }
    if (best >= 0)
        return &arr[best];
    return NULL;
}
