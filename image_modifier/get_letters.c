#include <err.h>
#include "base_img.h"

#define TOLERANCE_X 3
#define TOLERANCE_Y 3
#define NB_MIN_LETTERS 2
#define MAX_BOXES 2048

typedef struct {
    int x, y, w, h;
    int cx, cy; // center
} Box;

typedef struct {
    Box box;
    size_t word_id, letter_idx;
} WordLetter;

typedef struct {
    Box box;
    size_t row, col;
} GridLetter;

typedef enum {
    WORDS_CENTER,
    WORDS_LEFT,
    WORDS_RIGHT,
    WORDS_TOP,
    WORDS_BOTTOM,
} WordsDirection;

int cmp_box_cy(const void *a, const void *b)
{
    const Box *box_a = a; const Box *box_b = b;
    return box_a->cy - box_b->cy;
}
int cmp_box_cx(const void *a, const void *b)
{
    const Box *box_a = a; const Box *box_b = b;
    return box_a->cx - box_b->cx;
}
int cmp_box_tolerance_y(const void *a, const void *b)
{
    int c = cmp_box_cy(a, b);
    return (c != 0) ? c : cmp_box_cx(a, b);
}
int cmp_box_tolerance_x(const void *a, const void *b)
{
    int c = cmp_box_cx(a, b);
    return (c != 0) ? c : cmp_box_cy(a, b);
}
int cmp_size_t(const void* a, const void* b)
{
    return *(const size_t*)a - *(const size_t*)b;
}
int cmp_int(const void *a, const void *b)
{
    return *(const int*)a - *(const int*)b;
}
// the middle number of size_t array
size_t median_size_t(size_t *arr, size_t n)
{
    if (!arr || n == 0) errx(EXIT_FAILURE, "median_size_t arr NULL or n=0");
    size_t length = n * sizeof(*arr);
    size_t *tab = malloc(length);
    if (!tab) err(EXIT_FAILURE, "median_size_t malloc failed");
    memcpy(tab, arr, length);
    qsort(tab, n, sizeof(*tab), cmp_size_t);
    size_t res = tab[(n - 1) / 2];
    free(tab);
    return res;
}
// the middle number of int array
size_t median_int(int *arr, size_t n)
{
    if (!arr || n == 0) errx(EXIT_FAILURE, "median_size_t arr NULL or n=0");
    size_t length = n * sizeof(*arr);
    int *tab = malloc(length);
    if (!tab) err(EXIT_FAILURE, "median_size_t malloc failed");
    memcpy(tab, arr, length);
    qsort(tab, n, sizeof(*tab), cmp_size_t);
    int res = tab[(n - 1) / 2];
    free(tab);
    return res;
}
// the value that appears most often in arr
size_t mode_size_t(size_t *arr, size_t n)
{
    if (!arr || n == 0) errx(EXIT_FAILURE, "mode_size_t arr NULL or n=0");
    size_t length = n * sizeof(*arr);
    size_t *tab = malloc(length);
    if (!tab) err(EXIT_FAILURE, "mode_size_t malloc failed");
    memcpy(tab, arr, length);
    qsort(tab, n, sizeof(*tab), cmp_size_t);
    size_t count = 0;
    size_t value = 0;
    size_t max_count = 0;
    size_t max_value = 0;
    for (size_t i = 0; i < n; i++)
    {
        if (tab[i] == value)
        {
            count++;
            if (count > max_count)
            {
                max_value = value;
                max_count = count;
            }
        }
        else
        {
            count = 1;
            value = tab[i];
        }
    }
    free(tab);
    return max_value;
}

void save_boxes(SDL_Surface *img, Box *boxes, size_t nb, const char *name_file)
{
    for (size_t i = 0; i < nb; i++)
    {
        SDL_Rect r = {boxes[i].x, boxes[i].y, boxes[i].w, boxes[i].h};
        SDL_Surface *sub = SDL_CreateRGBSurface(0, r.w, r.h, img->format->BitsPerPixel, img->format->Rmask, img->format->Gmask, img->format->Bmask, img->format->Amask);
        if (!sub)
            err(EXIT_FAILURE, "SDL_CreateRGBSurface failed : %s", SDL_GetError());
        if (SDL_BlitSurface(img, &r, sub, NULL))
            err(EXIT_FAILURE, "SDL_BlitSurface failed");
        char file_name[128] = "";
        sprintf(file_name,"images_boxes/boxe_%s_%li_%ix%i_%ix%i.bmp",
            name_file, i, boxes[i].x, boxes[i].y, boxes[i].w, boxes[i].h);
        save_img_bmp(sub, file_name);
        SDL_FreeSurface(sub);
    }
}

void save_grid_letters(SDL_Surface *img, GridLetter *gl, size_t nb,
    const char *name_file)
{
    for (size_t i = 0; i < nb; i++)
    {
        SDL_Rect r = {gl[i].box.x, gl[i].box.y, gl[i].box.w, gl[i].box.h};
        SDL_Surface *sub = SDL_CreateRGBSurface(0, r.w, r.h, img->format->BitsPerPixel, img->format->Rmask, img->format->Gmask, img->format->Bmask, img->format->Amask);
        if (!sub)
            err(EXIT_FAILURE, "SDL_CreateRGBSurface failed : %s", SDL_GetError());
        if (SDL_BlitSurface(img, &r, sub, NULL))
            err(EXIT_FAILURE, "SDL_BlitSurface failed");
        char file_name[128] = "";
        sprintf(file_name,"images_grid_letters/grid_letter_%s_%li_%lix%li_(%ix%i_%ix%i).bmp",
            name_file, i, gl[i].col, gl[i].row, gl[i].box.x, gl[i].box.y, gl[i].box.w, gl[i].box.h);
        save_img_bmp(sub, file_name);
        SDL_FreeSurface(sub);
    }
}

void save_word_letters(SDL_Surface *img, WordLetter *wl, size_t nb,
    const char *name_file)
{
    for (size_t i = 0; i < nb; i++)
    {
        SDL_Rect r = {wl[i].box.x, wl[i].box.y, wl[i].box.w, wl[i].box.h};
        SDL_Surface *sub = SDL_CreateRGBSurface(0, r.w, r.h, img->format->BitsPerPixel, img->format->Rmask, img->format->Gmask, img->format->Bmask, img->format->Amask);
        if (!sub)
            err(EXIT_FAILURE, "SDL_CreateRGBSurface failed : %s", SDL_GetError());
        if (SDL_BlitSurface(img, &r, sub, NULL))
            err(EXIT_FAILURE, "SDL_BlitSurface failed");
        char file_name[128] = "";
        sprintf(file_name,"images_word_letters/word_letter_%s_%li_%lix%li_(%ix%i_%ix%i).bmp",
            name_file, i, wl[i].letter_idx, wl[i].word_id, wl[i].box.x, wl[i].box.y, wl[i].box.w, wl[i].box.h);
        save_img_bmp(sub, file_name);
        SDL_FreeSurface(sub);
    }
}

void save_with_marks(SDL_Surface *img, Box *boxes, size_t *indices,
    size_t nx, size_t ny, const char *name_file, const char *context)
{
    char file_name[128] = "";
    sprintf(file_name, "debug/%s_%s.bmp", name_file ? name_file : "solver", context ? context : "test");
    SDL_Surface *sub = SDL_CreateRGBSurface(0, img->w, img->h, img->format->BitsPerPixel, img->format->Rmask, img->format->Gmask, img->format->Bmask, img->format->Amask);
    if (!sub)
        err(EXIT_FAILURE, "SDL_CreateRGBSurface failed : %s", SDL_GetError());
    if (SDL_BlitSurface(img, &img->clip_rect, sub, NULL))
        err(EXIT_FAILURE, "SDL_BlitSurface failed");
    SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(sub);
    if (SDL_SetRenderDrawColor(renderer, 136, 8, 8, 255))
        err(EXIT_FAILURE, "SDL_RenderDrawRect failed");
    size_t n_color = 3 * (nx + ny);
    Uint8 *color = malloc(n_color);
    for (size_t i = 0; i < n_color; i++)
        color[i] = rand() % 256;
    for (size_t i = 0; i < nx * ny; i++)
    {
        size_t j = indices[i];
        SDL_Rect r = {boxes[j].x, boxes[j].y, boxes[j].w, boxes[j].h};
        if (SDL_RenderDrawRect(renderer, &r))
            err(EXIT_FAILURE, "SDL_RenderDrawRect failed");
        for (int delta = 0; delta < 1; delta++)
        {
            Uint8 *p = sub->pixels + r.y * sub->pitch + r.x * sub->format->BytesPerPixel;
            if (r.y-(1+delta) >= 0) // top
            {
                for (int k=-(1+delta)*sub->pitch; k<-(1+delta)*sub->pitch+r.w*sub->format->BytesPerPixel; k+=sub->format->BytesPerPixel)
                {
                    p[k  ] = color[3*(i/nx)];
                    p[k+1] = color[3*(i/nx)+1];
                    p[k+2] = color[3*(i/nx)+2];
                }
            }
            if (r.y+r.h+delta < sub->h) // bottom
            {
                for (int k=(r.h+delta)*sub->pitch; k<(r.h+delta)*sub->pitch+r.w*sub->format->BytesPerPixel; k+=sub->format->BytesPerPixel)
                {
                    p[k  ] = color[3*(i/nx)];
                    p[k+1] = color[3*(i/nx)+1];
                    p[k+2] = color[3*(i/nx)+2];
                }
            }
            if (r.x-(1+delta) >= 0) // left
            {
                for (int k=-(1+delta)*sub->format->BytesPerPixel; k<r.h*sub->pitch-(1+delta)*sub->format->BytesPerPixel; k+=sub->pitch)
                {
                    p[k  ] = color[3*(i%nx)];
                    p[k+1] = color[3*(i%nx)+1];
                    p[k+2] = color[3*(i%nx)+2];
                }
            }
            if (r.x+r.w+delta < sub->w) // right
            {
                for (int k=(r.w+delta)*sub->format->BytesPerPixel; k<r.h*sub->pitch+(r.w+delta)*sub->format->BytesPerPixel; k+=sub->pitch)
                {
                    p[k  ] = color[3*(i%nx)];
                    p[k+1] = color[3*(i%nx)+1];
                    p[k+2] = color[3*(i%nx)+2];
                }
            }
        }
    }
    save_img_bmp(sub, file_name);
    SDL_FreeSurface(sub);
}

size_t detect_connected_components(SDL_Surface *img, Box *boxes,
    int **out_segments, const char *name_file)
{
    if (!img)
        errx(EXIT_FAILURE, "SDL_Surface NULL\n");
    if (SDL_MUSTLOCK(img))
        SDL_LockSurface(img);
    const int dx[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    const int dy[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    *out_segments = calloc(img->w * img->h, sizeof(int));
    int *stack_x = calloc(img->w * img->h, sizeof(int));
    int *stack_y = calloc(img->w * img->h, sizeof(int));
    size_t count = 0;
    for (int y = 0; y < img->h; y++)
    {
        for (int x = 0; x < img->w; x++)
        {
            if ((*out_segments)[y * img->w + x])
                continue;
            Uint8 *p = img->pixels + y * img->pitch + x * img->format->BytesPerPixel;
            if (*p == 0) // black
            {
                int x_min = x, x_max = x, y_min = y, y_max = y;
                int top = 0;
                stack_x[top] = x;
                stack_y[top] = y;
                (*out_segments)[y * img->w + x] = 1;
                while (top >= 0)
                {
                    int cx = stack_x[top];
                    int cy = stack_y[top--];
                    if (cx < x_min) x_min = cx;
                    if (cx > x_max) x_max = cx;
                    if (cy < y_min) y_min = cy;
                    if (cy > y_max) y_max = cy;

                    for (unsigned char k = 0; k < 8; ++k)
                    {
                        int nx = cx + dx[k];
                        int ny = cy + dy[k];
                        if (nx < 0 || nx >= img->w || ny < 0 || ny >= img->h)
                            continue;
                        if ((*out_segments)[ny * img->w + nx])
                            continue;
                        Uint8 *p = img->pixels + ny * img->pitch + nx * img->format->BytesPerPixel;
                        if (*p == 0) // black
                        {
                            (*out_segments)[ny * img->w +nx] = 1;
                            stack_x[++top] = nx;
                            stack_y[top] = ny;
                        }
                    }
                }
                size_t bw = x_max - x_min + 1;
                size_t bh = y_max - y_min + 1;
                if (bw >= 2 && bh >= 8 && bw * 5 < (size_t)img->w && bh * 5 < (size_t)img->h)
                {
                    if (MAX_BOXES <= count)
                        errx(EXIT_FAILURE, "boxe full");
                    boxes[count].x = x_min;
                    boxes[count].y = y_min;
                    boxes[count].w = bw;
                    boxes[count].h = bh;
                    boxes[count].cx = boxes[count].x + boxes[count].w / 2;
                    boxes[count].cy = boxes[count].y + boxes[count].h / 2;
                    count++;
                }
            }
        }
    }
    free(stack_x);
    free(stack_y);
    if (SDL_MUSTLOCK(img))
        SDL_UnlockSurface(img);
    size_t *identity = malloc(count * sizeof(size_t));
    for(size_t i = 0; i < count; i++)
        identity[i] = i;
    save_with_marks(img, boxes, identity, count, 1, name_file, "connected_components");
    free(identity);
    return count;
}

size_t get_grid_size(size_t *hist, int max, size_t tolerance,
    size_t *packet_size, size_t *packet_count, size_t *out_indices)
{
    size_t *all_packet = calloc(max + 1, sizeof(size_t));
    if (!all_packet) err(EXIT_FAILURE, "get_grid_size calloc failed");
    size_t count_zero = tolerance + 1;
    size_t sum = 0;
    size_t nb_packet = 0;
    for (int i = 0; i <= max; i++)
    {
        if (hist[i])
        {
            sum += hist[i];
            count_zero = 0;
        }
        else
        {
            count_zero++;
            if (count_zero == tolerance)
            {
                if(sum) 
                    all_packet[nb_packet++] = sum;
                sum = 0;
            }
        }
    }
    if (sum)
        all_packet[nb_packet++] = sum;
    *packet_count = 0;
    if (nb_packet == 0)
    {
        free(all_packet);
        *packet_size = 0;
        return 0;
    }
    *packet_size = mode_size_t(all_packet, nb_packet);
    size_t nb_indices = 0;
    size_t index = 0;
    for (size_t i = 0; i < nb_packet; i++)
    {
        if (all_packet[i] != *packet_size)
        {
            index += all_packet[i];
            continue;
        }
        (*packet_count)++;
        for (size_t j = 0; j < *packet_size; j++)
            out_indices[nb_indices++] = index++;
    }
    free(all_packet);
    return nb_indices;
}

size_t get_number_of_lines(size_t *hist, int max, size_t *out_line_id)
{
    size_t *all_packet = calloc(max + 1, sizeof(size_t));
    if (!all_packet) err(EXIT_FAILURE, "get_number_of_lines calloc failed");
    size_t count_zero = TOLERANCE_Y + 1;
    size_t sum = 0;
    size_t nb_packet = 0;
    for (int i = 0; i <= max; i++)
    {
        if (hist[i])
        {
            sum += hist[i];
            count_zero = 0;
        }
        else
        {
            count_zero++;
            if (count_zero == TOLERANCE_Y)
            {
                if(sum)
                    all_packet[nb_packet++] = sum;
                sum = 0;
            }
        }
    }
    if (sum)
        all_packet[nb_packet++] = sum;
    if (nb_packet == 0)
    {
        free(all_packet);
        return 0;
    }
    size_t start = 0;
    for (size_t i = 0; i < nb_packet; i++)
    {
        out_line_id[i] = start;
        start += all_packet[i];
    }
    out_line_id[nb_packet] = start;

    free(all_packet);
    return nb_packet;
}

size_t group_letters_into_grid(Box *boxes, size_t n,
    GridLetter *out_letters, size_t *out_row_count, size_t *out_col_count,
    SDL_Surface *img, const char *name_file)
{
    if (!boxes || n == 0)
    {
        if (out_row_count) *out_row_count = 0; 
        if (out_col_count) *out_col_count = 0;
        return 0;
    }
    size_t nb_indices_x = 0;
    size_t nb_row = 0;
    size_t col_count_x = 0;
    size_t *indices_x = malloc(n * sizeof(size_t));
    size_t nb_indices_y = 0;
    size_t nb_col = 0;
    size_t row_count_y = 0;
    size_t *indices_y = malloc(n * sizeof(size_t));
    {
        qsort(boxes, n, sizeof(*boxes), cmp_box_tolerance_x);
        int max_x = 0;
        for (size_t i = 0; i < n; i++)
            if (boxes[i].cx > max_x)
                max_x = boxes[i].cx;
        size_t *hist_x = calloc(max_x + 1, sizeof(size_t));
        for (size_t i = 0; i < n; i++)
            hist_x[boxes[i].cx]++;
        nb_indices_x = get_grid_size(hist_x, max_x, TOLERANCE_X, &nb_row, &col_count_x, indices_x);
        free(hist_x);
        size_t *gap_y_i = malloc(nb_row * sizeof(size_t));
        size_t *all_gap_y = malloc(col_count_x * sizeof(size_t));
        for (size_t x = 0; x < col_count_x; x++)
        {
            // sort the range boxes[indices_x[x*col_count_x : (x+1)*col_count_x]] by increasing y
            for (size_t i = 0; i < nb_row; i++)
            {
                int by = boxes[indices_x[x*nb_row+i]].y;
                int by_min = by;
                int by_ind = i;
                for (size_t j = i+1; j < nb_row; j++)
                {
                    int byj = boxes[indices_x[x*nb_row+j]].y;
                    if(byj < by_min) {
                        by_min = byj;
                        by_ind = j;
                    }
                }
                size_t temp = indices_x[x*nb_row+by_ind];
                indices_x[x*nb_row+by_ind] = indices_x[x*nb_row+i];
                indices_x[x*nb_row+i] = temp;
            }
            int last_cy = 0;
            size_t i = 0;
            for (size_t y = 0; y < nb_row; y++)
            {
                gap_y_i[y] = boxes[indices_x[i]].cy - last_cy;
                last_cy = boxes[indices_x[i++]].cy;
            }
            all_gap_y[x] = median_size_t(gap_y_i, nb_row);
        }
        free(gap_y_i);
        size_t gap_y = median_size_t(all_gap_y, col_count_x);
        free(all_gap_y);

        printf("gap_y=%li\n", gap_y);
        printf("grid_x = %lix%li\n", col_count_x, nb_row);
        save_with_marks(img, boxes, indices_x, col_count_x, nb_row, name_file, "grid_x");
    }
    //////////////////////////////////////////////////////////
    {
        qsort(boxes, n, sizeof(*boxes), cmp_box_tolerance_y);
        int max_y = 0;
        for (size_t i = 0; i < n; i++)
            if (boxes[i].cy > max_y)
                max_y = boxes[i].cy;
        size_t *hist_y = calloc(max_y + 1, sizeof(size_t));
        for (size_t i = 0; i < n; i++)
            hist_y[boxes[i].cy]++;
        nb_indices_y = get_grid_size(hist_y, max_y, TOLERANCE_Y, &nb_col, &row_count_y, indices_y);
        free(hist_y);
        size_t *gap_x_i = malloc(nb_col * sizeof(size_t));
        size_t *all_gap_x = malloc(row_count_y * sizeof(size_t));
        for (size_t y = 0; y < row_count_y; y++)
        {
            // sort the range boxes[indices_y[y*row_count_y : (y+1)*row_count_y]] by increasing x
            for (size_t i = 0; i < nb_col; i++)
            {
                int bx = boxes[indices_y[y*nb_col+i]].x;
                int bx_min = bx;
                int bx_ind = i;
                for (size_t j = i+1; j < nb_col; j++)
                {
                    int bxj = boxes[indices_y[y*nb_col+j]].x;
                    if(bxj < bx_min) {
                        bx_min = bxj;
                        bx_ind = j;
                    }
                }
                size_t temp = indices_y[y*nb_col+bx_ind];
                indices_y[y*nb_col+bx_ind] = indices_y[y*nb_col+i];
                indices_y[y*nb_col+i] = temp;
            }
            int last_cx = 0;
            size_t i = 0;
            for (size_t x = 0; x < nb_col; x++)
            {
                gap_x_i[x] = boxes[indices_y[i]].cx - last_cx;
                last_cx = boxes[indices_y[i++]].cx;
            }
            all_gap_x[y] = median_size_t(gap_x_i, nb_col);
        }
        free(gap_x_i);
        size_t gap_x = median_size_t(all_gap_x, row_count_y);
        free(all_gap_x);

        printf("gap_x=%li\n", gap_x);
        printf("grid_y = %lix%li\n", nb_col, row_count_y);
        save_with_marks(img, boxes, indices_y, nb_col, row_count_y, name_file, "grid_y");
    }
    if (nb_indices_x > nb_indices_y)
    {
        qsort(boxes, n, sizeof(*boxes), cmp_box_tolerance_x);
        if (out_row_count) *out_row_count = nb_row;
        if (out_col_count) *out_col_count = col_count_x;
        for (size_t i = 0; i < nb_indices_x; i++)
        {
            if (i >= MAX_BOXES)
                errx(EXIT_FAILURE, "group_letters_into_grid max_letters = %i", MAX_BOXES);
            out_letters[i] = (GridLetter){boxes[indices_x[i]], i % col_count_x, i / col_count_x};
        }
        free(indices_x);
        free(indices_y);
        return nb_indices_x;
    }
    else
    {
        if (out_row_count) *out_row_count = row_count_y;
        if (out_col_count) *out_col_count = nb_col;
        for (size_t i = 0; i < nb_indices_y; i++)
        {
            if (i >= MAX_BOXES)
                errx(EXIT_FAILURE, "group_letters_into_grid max_letters = %i", MAX_BOXES);
            out_letters[i] = (GridLetter){boxes[indices_y[i]], i / nb_col, i % nb_col};
        }
        free(indices_x);
        free(indices_y);
        return nb_indices_y;
    }
}

size_t group_letters_into_words(Box *boxes, size_t n,
    WordLetter *out_letters, size_t *out_words_count,
    SDL_Surface *img, const char *name_file)
{
    if (!boxes || n == 0)
    {
        if (out_words_count) *out_words_count = 0;
        return 0;
    }
    size_t nb_lines = 0;
    qsort(boxes, n, sizeof(*boxes), cmp_box_tolerance_y);
    int max_y = 0;
    for (size_t i = 0; i < n; i++)
        if (boxes[i].cy > max_y)
            max_y = boxes[i].cy;
    size_t *hist_y = calloc(max_y + 1, sizeof(size_t));
    for (size_t i = 0; i < n; i++)
        hist_y[boxes[i].cy]++;
    size_t *line_id = calloc(max_y + 1, sizeof(size_t));
    nb_lines = get_number_of_lines(hist_y, max_y, line_id);
    free(hist_y);
    size_t *indices = calloc(n, sizeof(size_t));
    for(size_t i = 0; i < n; ++i)
        indices[i] = i;
    for (size_t y = 0; y < nb_lines; y++)
    {
        size_t start = line_id[y];
        size_t end = line_id[y+1];
        // sort the range boxes[start : end] by increasing x
        for (size_t i = start; i < end; i++)
        {
            int bx = boxes[indices[i]].x;
            int bx_min = bx;
            int bx_ind = i;
            for (size_t j = i+1; j < end; j++)
            {
                int bxj = boxes[indices[j]].x;
                if(bxj < bx_min)
                {
                    bx_min = bxj;
                    bx_ind = j;
                }
            }
            size_t temp = indices[bx_ind];
            indices[bx_ind] = indices[i];
            indices[i] = temp;
        }
    }
    size_t total_letters = 0;
    size_t num_words = 0;
    for (size_t y = 0; y < nb_lines; y++)
    {
        size_t start = line_id[y];
        size_t end = line_id[y+1];
        int last = boxes[indices[start]].x;
        for (size_t i = start; i < end; i++)
        {
            if (boxes[indices[i]].x - last >= boxes[indices[i]].w + 2 * TOLERANCE_X)
            {
                start = i;
                num_words++;
            }
            out_letters[total_letters].box = boxes[indices[i]];
            out_letters[total_letters].word_id = num_words;
            out_letters[total_letters++].letter_idx = i - start;
            last = boxes[indices[i]].x + boxes[indices[i]].w;
        }
        num_words++;
    }
    // Letters
    save_with_marks(img, boxes, indices, n, 1, name_file, "letters_individual");
    { // Words
        size_t word_id = -1;
        Box *words = malloc(num_words * sizeof(Box));
        size_t *identity = malloc(num_words * sizeof(size_t));
        for(size_t i = 0; i < total_letters; i++)
        {
            if(out_letters[i].word_id == word_id)
            {
                words[word_id].x = (int)fmin(words[word_id].x, out_letters[i].box.x);
                words[word_id].y = (int)fmin(words[word_id].y, out_letters[i].box.y);
                words[word_id].w = (int)fmax(words[word_id].x + words[word_id].w, out_letters[i].box.x+out_letters[i].box.w) - words[word_id].x;
                words[word_id].h = (int)fmax(words[word_id].y + words[word_id].h, out_letters[i].box.y+out_letters[i].box.h) - words[word_id].y;
            }
            else
            {
                word_id = out_letters[i].word_id;
                words[word_id].x = out_letters[i].box.x;
                words[word_id].y = out_letters[i].box.y;
                words[word_id].w = out_letters[i].box.w;
                words[word_id].h = out_letters[i].box.h;
            }
        }
        for(size_t i = 0; i < num_words; i++)
            identity[i] = i;
        save_with_marks(img, words, identity, num_words, 1, name_file, "words_group");
        free(words);
        free(identity);
    }
    free(line_id);
    free(indices);
    if (out_words_count)
        *out_words_count = num_words;
    return total_letters;
}

size_t remove_small_words(WordLetter *wl, size_t count, size_t *word_count,
                          SDL_Surface *img, const char *name_file)
{
    if (count == 0) return 0;
    size_t orig_words = *word_count;
    size_t *sizes = calloc(orig_words, sizeof(size_t));
    for (size_t i = 0; i < count; i++)
        sizes[wl[i].word_id]++;
    int *remap = malloc(orig_words * sizeof(int));
    int next_id = 0;
    for (size_t w = 0; w < orig_words; w++)
        remap[w] = (sizes[w] >= NB_MIN_LETTERS) ? next_id++ : -1;
    size_t out = 0;
    for (size_t i = 0; i < count; i++)
    {
        int new_id = remap[wl[i].word_id];
        if (new_id == -1) continue;
        wl[i].word_id = new_id;
        wl[i].letter_idx = 0;
        wl[out++] = wl[i];
    }
    free(sizes);
    free(remap);
    size_t *positions = calloc(next_id + 1, sizeof(size_t));
    for (size_t i = 0; i < out; i++)
        positions[wl[i].word_id]++;
    size_t cum = 0;
    for (int i = 0; i <= next_id; i++)
    {
        size_t tmp = positions[i];
        positions[i] = cum;
        cum += tmp;
    }
    // Suppr first letter if spaced away from the others
    for (int w = 0; w < next_id; w++)
    {
        size_t start = positions[w];
        size_t end = positions[w+1];
        size_t count_letters = end - start;
        if (count_letters <= NB_MIN_LETTERS + 1)
            continue;
        double total_gap = 0;
        for (size_t i = start + 1; i < end-1; i++)
            total_gap += fmax(1.0, wl[i+1].box.x - (wl[i].box.x + wl[i].box.w));
        double avg_gap = total_gap / (count_letters - 2);
        double first_gap = wl[start+1].box.x - (wl[start].box.x + wl[start].box.w);
        if (first_gap > avg_gap + TOLERANCE_X)
        {
            for (size_t i = start; i < out-1; i++)
                wl[i] = wl[i+1];
            out--;
            for (int i = w+1; i <= next_id; i++)
                positions[i]--;
        }
    }
    size_t *counters = calloc(next_id, sizeof(size_t));
    for (size_t i = 0; i < out; i++)
    {
        size_t w = wl[i].word_id;
        wl[i].letter_idx = counters[w]++;
    }
    free(counters);
    { // Letters
        Box *letter_boxes = malloc(out * sizeof(Box));
        size_t *identity = malloc(out * sizeof(size_t));
        for (size_t i = 0; i < out; i++)
        {
            letter_boxes[i] = wl[i].box;
            identity[i] = i;
        }
        save_with_marks(img, letter_boxes, identity, out, 1, name_file, "z_letters_individual");
        free(letter_boxes);
        free(identity);
    }
    { // Words
        Box *words = malloc(next_id * sizeof(Box));
        size_t *identity = malloc(next_id * sizeof(size_t));
        for (int w = 0; w < next_id; w++)
        {
            size_t start = positions[w];
            size_t end = positions[w+1];
            if (start >= end) continue;
            words[w] = wl[start].box; // initialiser avec la première lettre
            for (size_t i = start+1; i < end; ++i)
            {
                words[w].x = (int)fmin(words[w].x, wl[i].box.x);
                words[w].y = (int)fmin(words[w].y, wl[i].box.y);
                words[w].w = (int)fmax(words[w].x + words[w].w, wl[i].box.x + wl[i].box.w) - words[w].x;
                words[w].h = (int)fmax(words[w].y + words[w].h, wl[i].box.y + wl[i].box.h) - words[w].y;
            }
            identity[w] = w;
        }
        save_with_marks(img, words, identity, next_id, 1, name_file, "z_words_grouped");
        free(words);
        free(identity);
    }
    *word_count = next_id;
    free(positions);
    return out;
}

size_t difference_update(Box *boxes, size_t nb, const GridLetter *gl, size_t ngl)
{
    size_t new_i = 0;
    for (size_t i = 0; i < nb; ++i)
    {
        char absent = 1;
        for (size_t j = 0; j < ngl; ++j)
        {
            Box b = gl[j].box;
            if (boxes[i].x == b.x && boxes[i].y == b.y &&
                boxes[i].w == b.w && boxes[i].h == b.h)
            {
                absent = 0;
                break;
            }
        }
        if (absent) boxes[new_i++] = boxes[i];
    }
    return new_i;
}

Box compute_grid_bbox(GridLetter *gl, size_t n)
{
    Box b;
    b.w = 0;
    b.h = 0;
    if (n == 0)
    {
        b.x = 0;
        b.y = 0;
        return b;
    }
    b.x = gl->box.x;
    b.y = gl->box.y;
    for (size_t i = 0; i < n; i++)
    {
        Box *bgl = &gl[i].box;
        b.x = (int)fmin(b.x, bgl->x);
        b.y = (int)fmin(b.y, bgl->y);
        b.w = (int)fmax(b.x + b.w, bgl->x + bgl->w) - b.x;
        b.h = (int)fmax(b.y + b.h, bgl->y + bgl->h) - b.y;
    }
    b.cx = b.x + b.w / 2;
    b.cy = b.y + b.h / 2;
    return b;
}

Box compute_words_barycenter(Box *boxes, size_t n)
{
    Box barycenter;
    double sx = 0, sy = 0;
    for (size_t i = 0; i < n; i++)
    {
        Box *b = &boxes[i];
        double cx = b->x + b->w / 2.0;
        double cy = b->y + b->h / 2.0;
        sx += cx;
        sy += cy;
    }
    barycenter.x = sx / n;
    barycenter.y = sy / n;
    barycenter.w = 0;
    barycenter.h = 0;
    barycenter.cx = barycenter.x;
    barycenter.cy = barycenter.y;
    return barycenter;
}

WordsDirection detect_words_side(Box a, Box b)
{
    // distances signées
    double dist_left   = a.x - b.x;
    double dist_right  = b.x + b.w - a.x;
    double dist_top    = a.y - b.y;
    double dist_bottom = b.y + b.h - a.y;
    double best = 1e12;
    WordsDirection side = WORDS_CENTER;
    if (a.y < b.y && fabs(dist_top) < best)
    {
        best = fabs(dist_top);
        side = WORDS_TOP;
    }
    if (a.y > b.y + b.h && fabs(dist_bottom) < best)
    {
        best = fabs(dist_bottom);
        side = WORDS_BOTTOM;
    }
    if (a.x < b.x && fabs(dist_left) < best)
    {
        best = fabs(dist_left);
        side = WORDS_LEFT;
    }
    if (a.x > b.x + b.w && fabs(dist_right) < best)
    {
        best = fabs(dist_right);
        side = WORDS_RIGHT;
    }
    return side;
}

size_t filter_by_direction(Box *boxes, size_t n, Box grid, WordsDirection side)
{
    size_t out = 0;
    for (size_t i = 0; i < n; i++)
    {
        Box *b = &boxes[i];
        int keep = 0;
        if (side == WORDS_CENTER)
            keep = 1;
        if (side == WORDS_TOP)
            keep = ((int)b->cy + b->h < grid.y);
        else if (side == WORDS_BOTTOM)
            keep = ((int)b->cy - b->h > grid.y + grid.h + 5);
        else if (side == WORDS_LEFT)
            keep = ((int)b->cx + b->h < grid.x);
        else if (side == WORDS_RIGHT)
            keep = ((int)b->cx - b->h > grid.x + grid.w);
        if (keep)
            boxes[out++] = boxes[i];
    }
    return out;
}

size_t filter_by_height(Box *boxes, size_t n)
{
    size_t out = 0;
    int *heights = malloc(sizeof(int) * n);
    for (size_t i = 0; i < n; i++)
        heights[i] = boxes[i].h;
    int medianheight = median_int(heights, n);
    free(heights);
    for (size_t i = 0; i < n; i++)
        if (boxes[i].h > medianheight - TOLERANCE_Y && boxes[i].h < medianheight + 2*TOLERANCE_Y)
            boxes[out++] = boxes[i];
    return out;
}

int main(int argc, char **argv)
{
    if (argc > 2)
        errx(EXIT_FAILURE, "argc=%i, used $ %s <input> <output>", argc, argv[0]);
    const char *name_file = (argc == 2) ? argv[1] : "level_1_image_1.png_bw.bmp";

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        err(EXIT_FAILURE, "SDL_Init failed");

    char black_white_name[64] = "";
    sprintf(black_white_name, "black_and_white/%s", name_file);
    SDL_Surface *img = load_img(black_white_name);
    printf("image %s : %i x %i\n", black_white_name, img->w, img->h);

    Box *boxes = malloc(sizeof(Box) * MAX_BOXES);
    int *segments;
    size_t nb = detect_connected_components(img, boxes, &segments, name_file);
    printf("%li boxes detected\n", nb);

    save_boxes(img, boxes, nb, name_file);

    GridLetter *gl = malloc(sizeof(GridLetter) * MAX_BOXES);
    size_t rows, cols;
    size_t ngl = group_letters_into_grid(boxes, nb, gl, &rows, &cols, img, name_file);
    printf("Grid detected: %li letters, rows=%li cols=%li\n", ngl, rows, cols);
    save_grid_letters(img, gl, ngl, name_file);
    nb = difference_update(boxes, nb, gl, ngl);
    printf("lenboxes-lengrid = %li\n", nb);

    Box grid_bb = compute_grid_bbox(gl, ngl);
    printf("box grid: %ix%i\n", grid_bb.cx, grid_bb.cy);
    Box words_center = compute_words_barycenter(boxes, nb);
    printf("barycenter exept grid : %ix%i\n", words_center.cx, words_center.cy);
    WordsDirection side = detect_words_side(words_center, grid_bb);
    nb = filter_by_direction(boxes, nb, grid_bb, side);
    printf("filtered by WordsDirection (%i) letters : %li\n", side,  nb);

    nb = filter_by_height(boxes, nb);
    printf("filtered by height letters : %li\n", nb);

    WordLetter *wl = malloc(sizeof(WordLetter) * MAX_BOXES);
    size_t words_count;
    size_t nwl = group_letters_into_words(boxes, nb, wl, &words_count, img, name_file);
    printf("Detected %li letters in %li words \n", nwl, words_count);
    //save_word_letters(img, wl, nwl, name_file);
    nwl = remove_small_words(wl, nwl, &words_count, img, name_file);
    printf("Detected %li letters in %li real words \n", nwl, words_count);
    save_word_letters(img, wl, nwl, name_file);

    free(segments);
    free(boxes);
    free(gl);
    free(wl);

    SDL_FreeSurface(img);
    SDL_Quit();
    return EXIT_SUCCESS;
}
