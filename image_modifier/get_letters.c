#include <err.h>
#include <math.h>
#include <limits.h>
#include "get_letters.h"

#define TOLERANCE_X 3
#define TOLERANCE_Y 3

size_t detect_connected_components(SDL_Surface *img, Box *boxes, size_t max_boxes, int **out_segments)
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
                if (bw >= 2 && bh >= 5 && bw * 5 < (size_t)img->w && bh * 5 < (size_t)img->h)
                {
                    if (max_boxes <= count)
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
    return count;
}

void save_boxes(SDL_Surface *img, Box *boxes, size_t nb, char *parent_name)
{
    for (size_t i = 0; i < nb; i++)
    {
        SDL_Rect r = {boxes[i].x, boxes[i].y, boxes[i].w, boxes[i].h};
        SDL_Surface *sub = SDL_CreateRGBSurface(0, r.w, r.h, img->format->BitsPerPixel, img->format->Rmask, img->format->Gmask, img->format->Bmask, img->format->Amask);
        if (!sub)
            errx(EXIT_FAILURE, "SDL_CreateRGBSurface failed : %s", SDL_GetError());
        if (SDL_BlitSurface(img, &r, sub, NULL))
            errx(EXIT_FAILURE, "SDL_BlitSurface failed");
        char file_name[128] = "";
        if (boxes[i].w * 2 > img->w && boxes[i].h * 2 > img->h)
            sprintf(file_name,"images_giant/boxe_%s_%li_%ix%i_%ix%i.bmp", parent_name, i, boxes[i].x, boxes[i].y, boxes[i].w, boxes[i].h);
        else if (boxes[i].h * 2 > img->h || boxes[i].w * 2 > img->w)
            sprintf(file_name,"images_big/boxe_%s_%li_%ix%i_%ix%i.bmp", parent_name, i, boxes[i].x, boxes[i].y, boxes[i].w, boxes[i].h);
        else if (boxes[i].w > boxes[i].h * 2)
            sprintf(file_name,"images_bin/boxe_%s_%li_%ix%i_%ix%i.bmp", parent_name, i, boxes[i].x, boxes[i].y, boxes[i].w, boxes[i].h);
        else
            sprintf(file_name,"images_letter/boxe_%s_%li_%ix%i_%ix%i.bmp", parent_name, i, boxes[i].x, boxes[i].y, boxes[i].w, boxes[i].h);
        save_img_bmp(sub, file_name);
        SDL_FreeSurface(sub);
    }
}

void save_grid_letters(SDL_Surface *img, GridLetter *gl, size_t nb, const char *parent_name)
{
    for (size_t i = 0; i < nb; i++)
    {
        SDL_Rect r = {gl[i].box.x, gl[i].box.y, gl[i].box.w, gl[i].box.h};
        SDL_Surface *sub = SDL_CreateRGBSurface(0, r.w, r.h, img->format->BitsPerPixel, img->format->Rmask, img->format->Gmask, img->format->Bmask, img->format->Amask);
        if (!sub)
            errx(EXIT_FAILURE, "SDL_CreateRGBSurface failed : %s", SDL_GetError());
        if (SDL_BlitSurface(img, &r, sub, NULL))
            errx(EXIT_FAILURE, "SDL_BlitSurface failed");
        char file_name[128] = "";
        sprintf(file_name,"images_grid_letters/grid_letter_%s_%li_%lix%li_(%ix%i_%ix%i).bmp", parent_name, i, gl[i].row, gl[i].col, gl[i].box.x, gl[i].box.y, gl[i].box.w, gl[i].box.h);
        save_img_bmp(sub, file_name);
        SDL_FreeSurface(sub);
    }
}

void save_word_letters(SDL_Surface *img, WordLetter *wl, size_t nb, const char *parent_name)
{
    for (size_t i = 0; i < nb; i++)
    {
        SDL_Rect r = {wl[i].box.x, wl[i].box.y, wl[i].box.w, wl[i].box.h};
        SDL_Surface *sub = SDL_CreateRGBSurface(0, r.w, r.h, img->format->BitsPerPixel, img->format->Rmask, img->format->Gmask, img->format->Bmask, img->format->Amask);
        if (!sub)
            errx(EXIT_FAILURE, "SDL_CreateRGBSurface failed : %s", SDL_GetError());
        if (SDL_BlitSurface(img, &r, sub, NULL))
            errx(EXIT_FAILURE, "SDL_BlitSurface failed");
        char file_name[128] = "";
        sprintf(file_name,"images_word_letters/word_letter_%s_%li_%lix%li_(%ix%i_%ix%i).bmp", parent_name, i, wl[i].word_id, wl[i].letter_idx, wl[i].box.x, wl[i].box.y, wl[i].box.w, wl[i].box.h);
        save_img_bmp(sub, file_name);
        SDL_FreeSurface(sub);
    }
}

void save_with_marks(SDL_Surface *img, Box *boxes, size_t *indices, size_t nx,  size_t ny, const char *parent_name)
{
    char file_name[128] = "";
    sprintf(file_name,"new_%s.bmp", parent_name);

    SDL_Surface *sub = SDL_CreateRGBSurface(0, img->w, img->h, img->format->BitsPerPixel, img->format->Rmask, img->format->Gmask, img->format->Bmask, img->format->Amask);
    if (!sub)
        errx(EXIT_FAILURE, "SDL_CreateRGBSurface failed : %s", SDL_GetError());
    if (SDL_BlitSurface(img, &img->clip_rect, sub, NULL))
        errx(EXIT_FAILURE, "SDL_BlitSurface failed");
    SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(sub);
    if (SDL_SetRenderDrawColor(renderer, 136, 8, 8, 255))
        errx(EXIT_FAILURE, "SDL_RenderDrawRect failed");

    size_t n_color = 3*(nx+ny);
    Uint8 *color = malloc(n_color);
    for (size_t i = 0; i < n_color; i++)
    {
        color[i] = rand() % 256;
    }

    for (size_t i = 0; i < nx * ny; i++)
    {
        size_t j = indices[i];
        SDL_Rect r = {boxes[j].x, boxes[j].y, boxes[j].w, boxes[j].h};
        if (SDL_RenderDrawRect(renderer, &r))
            errx(EXIT_FAILURE, "SDL_RenderDrawRect failed");
        for (int delta = 1; delta < 4; delta++)
        {
            Uint8 *p = sub->pixels + r.y * sub->pitch + r.x * sub->format->BytesPerPixel;
            // top
            if (r.y-(1+delta) >= 0)
            {
                for (int k=-(1+delta)*sub->pitch; k<-(1+delta)*sub->pitch+r.w*sub->format->BytesPerPixel; k+=sub->format->BytesPerPixel) {
                    p[k+0] = color[3*(i/nx)  ];
                    p[k+1] = color[3*(i/nx)+1];
                    p[k+2] = color[3*(i/nx)+2];
                }
            }
            // bottom
            if (r.y+r.h+delta < sub->h)
            {
                for (int k=(r.h+delta)*sub->pitch; k<(r.h+delta)*sub->pitch+r.w*sub->format->BytesPerPixel; k+=sub->format->BytesPerPixel) {
                    p[k+0] = color[3*(i/nx)  ];
                    p[k+1] = color[3*(i/nx)+1];
                    p[k+2] = color[3*(i/nx)+2];
                }
            }
            // left
            if (r.x-(1+delta) >= 0)
            {
                for (int k=-(1+delta)*sub->format->BytesPerPixel; k<r.h*sub->pitch-(1+delta)*sub->format->BytesPerPixel; k+=sub->pitch) {
                    p[k+0] = color[3*(i%nx)  ];
                    p[k+1] = color[3*(i%nx)+1];
                    p[k+2] = color[3*(i%nx)+2];
                }
            }
            // right
            if (r.x+r.w+delta < sub->w)
            {
                for (int k=(r.w+delta)*sub->format->BytesPerPixel; k<r.h*sub->pitch+(r.w+delta)*sub->format->BytesPerPixel; k+=sub->pitch) {
                    p[k+0] = color[3*(i%nx)  ];
                    p[k+1] = color[3*(i%nx)+1];
                    p[k+2] = color[3*(i%nx)+2];
                }
            }
        }
    }
    save_img_bmp(sub, file_name);
    SDL_FreeSurface(sub);
}


// safe push into boxes array (returns new count or keeps old if overflow)
static size_t push_box(Box *boxes, size_t count, size_t max_boxes, Box b)
{
    if (count >= max_boxes)
        errx(EXIT_FAILURE, "max boxes count");
    boxes[count] = b;
    return count + 1;
}
// For each box whose width is "large" relative to its height (or absolute),
// compute vertical projection histogram inside the box and find valley columns where
// the column count <= valley_threshold. Use these columns as separators.
// Create sub-boxes between separators and append them.
size_t split_large_boxes(SDL_Surface *img, Box *boxes, size_t n, size_t max_boxes)
{
    if (!img || !boxes) return n;
    // We'll write new boxes into a temporary array then copy back
    Box *newboxes = malloc(sizeof(Box) * max_boxes);
    if (!newboxes) errx(EXIT_FAILURE, "malloc failed in split_large_boxes");
    size_t newcount = 0;
    int bpp = img->format->BytesPerPixel;
    Uint8 *pixels = (Uint8 *)img->pixels;

    for (size_t i = 0; i < n; ++i)
    {
        Box B = boxes[i];
        // Heuristique : if box is relatively narrow, keep it
        if (B.w <= (int)(1.6 * B.h) || B.w < 12)
        {
            newcount = push_box(newboxes, newcount, max_boxes, B);
            continue;
        }
        // Build vertical histogram inside B
        int bw = B.w;
        int bh = B.h;
        int *hist_y = calloc(bw, sizeof(int));
        if (!hist_y) errx(EXIT_FAILURE, "calloc failed in split_large_boxes");
        for (int yy = 0; yy < bh; ++yy)
        {
            int py = B.y + yy;
            for (int xx = 0; xx < bw; ++xx)
            {
                int px = B.x + xx;
                Uint8 *p = pixels + py * img->pitch + px * bpp;
                if (p[0] == 0) hist_y[xx]++; // black pixel
            }
        }
        // Determine threshold for valley: small fraction of box height
        int valley_thresh = (int)fmax(1.0, 0.08 * bh); // tweakable
        // Find valley positions: consecutive columns with hist_y <= valley_thresh
        int in_valley = 0;
        int valley_start = -1;
        int splits_capacity = 64;
        int *splits = malloc(sizeof(int) * splits_capacity);
        int split_count = 0;
        for (int x = 0; x < bw; ++x)
        {
            if (hist_y[x] <= valley_thresh)
            {
                if (!in_valley)
                {
                    in_valley = 1;
                    valley_start = x;
                }
            }
            else
            {
                if (in_valley)
                {
                    int valley_end = x - 1;
                    int valley_width = valley_end - valley_start + 1;
                    // Accept valley if wide enough (at least 1 or more)
                    if (valley_width >= 1 && valley_start > 1 && valley_end < bw - 2)
                    {
                        // choose split column the middle of valley
                        int split_col = (valley_start + valley_end) / 2;
                        if (split_count < splits_capacity)
                            splits[split_count++] = split_col;
                    }
                    in_valley = 0;
                }
            }
        }
        // tail valley
        if (in_valley)
        {
            int valley_end = bw - 1;
            int valley_width = valley_end - valley_start + 1;
            if (valley_width >= 1 && valley_start > 1 && valley_end < bw - 2)
            {
                int split_col = (valley_start + valley_end) / 2;
                if (split_count < splits_capacity)
                    splits[split_count++] = split_col;
            }
        }
        if (split_count == 0) // No split found: keep as is
            newcount = push_box(newboxes, newcount, max_boxes, B);
        else
        {
            // Build segments between splits
            int seg_x0 = 0;
            for (int s = 0; s <= split_count; ++s)
            {
                int seg_x1;
                if (s < split_count) seg_x1 = splits[s];
                else seg_x1 = bw - 1;
                int seg_w = seg_x1 - seg_x0 + 1;
                if (seg_w >= 3) // keep reasonable minimum width
                {
                    Box nb;
                    nb.x = B.x + seg_x0;
                    nb.y = B.y;
                    nb.w = seg_w;
                    nb.h = B.h;
                    nb.cx = nb.x + nb.w / 2;
                    nb.cy = nb.y + nb.h / 2;
                    nb.nb_pixels = 0; // optional to fill
                    newcount = push_box(newboxes, newcount, max_boxes, nb);
                }
                seg_x0 = seg_x1 + 1;
            }
        }
        free(splits);
        free(hist_y);
    }
    // Copy back into boxes[]
    size_t copy_n = (newcount < max_boxes) ? newcount : max_boxes;
    memcpy(boxes, newboxes, sizeof(Box) * copy_n);
    free(newboxes);
    return copy_n;
}


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

int cmp_size_t(const void* a, const void* b) {
    return *(const size_t*)a - *(const size_t*)b;
}
// the middle number
size_t median(size_t *arr, size_t n)
{
    if (!arr || n == 0) errx(EXIT_FAILURE, "median arr NULL or n=0");
    size_t length = n * sizeof(*arr);
    size_t *tab = malloc(length);
    if (!arr || n == 0) errx(EXIT_FAILURE, "median malloc failed");
    memcpy(tab, arr, length);
    qsort(tab, n, sizeof(*tab), cmp_size_t);
    size_t res = tab[(n - 1) / 2];
    free(tab);
    return res;
}
// the value that appears most often in arr
size_t mode(size_t *arr, size_t n)
{
    if (!arr || n == 0) errx(EXIT_FAILURE, "mode arr NULL or n=0");
    size_t length = n * sizeof(*arr);
    size_t *tab = malloc(length);
    if (!tab) errx(EXIT_FAILURE, "mode malloc failed");
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
        //printf("%li %li %li %li\n", value, count, max_value, max_count);
    }
    free(tab);
    return max_value;
}
size_t get_count(size_t *hist, size_t max, size_t tolerance, size_t *packet_size, size_t *packet_count, size_t *out_indices)
{
    size_t *all_packet = calloc(max / 2, sizeof(size_t));
    if (!all_packet) errx(EXIT_FAILURE, "get_count calloc failed");
    size_t count_zero = tolerance + 1;
    size_t sum = 0;
    size_t nb_packet = 0;
    for (size_t i = 0; i <= max; i++)
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
    
    *packet_size = mode(all_packet, nb_packet);
    size_t nb_indices = 0;
    size_t index = 0;
    for (size_t i = 0; i < nb_packet; i++)
    {
        // printf("%li\n", all_packet[i]);
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

size_t group_letters_into_grid(SDL_Surface *img, Box *boxes, size_t n, GridLetter *out_letters, size_t max_letters, size_t *out_row_count, size_t *out_col_count)
{
    if (n == 0)
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
        size_t max_x = 0;
        for (size_t i = 0; i < n; i++)
            if (boxes[i].cx > max_x)
                max_x = boxes[i].cx;
        size_t *hist_x = calloc(max_x + 1, sizeof(size_t));
        for (size_t i = 0; i < n; i++)
            hist_x[boxes[i].cx]++;
        nb_indices_x = get_count(hist_x, max_x, TOLERANCE_X, &nb_row, &col_count_x, indices_x);
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
            all_gap_y[x] = median(gap_y_i, nb_row);
        }
        free(gap_y_i);
        size_t gap_y = median(all_gap_y, col_count_x);
        free(all_gap_y);

        printf("gap_y=%li\n", gap_y);
        save_with_marks(img, boxes, indices_x, col_count_x, nb_row, "test_detection_x");

        free(indices_x);
        printf("grid x = %lix%li\n", nb_row, col_count_x);
    }
    //////////////////////////////////////////////////////////
    {
        qsort(boxes, n, sizeof(*boxes), cmp_box_tolerance_y);
        size_t max_y = 0;
        for (size_t i = 0; i < n; i++)
            if (boxes[i].cy > max_y)
                max_y = boxes[i].cy;
        size_t *hist_y = calloc(max_y + 1, sizeof(size_t));
        for (size_t i = 0; i < n; i++)
            hist_y[boxes[i].cy]++;
        nb_indices_y = get_count(hist_y, max_y, TOLERANCE_Y, &nb_col, &row_count_y, indices_y);
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
            all_gap_x[y] = median(gap_x_i, nb_col);
        }
        free(gap_x_i);
        size_t gap_x = median(all_gap_x, row_count_y);
        free(all_gap_x);

        printf("gap_x=%li\n", gap_x);
        save_with_marks(img, boxes, indices_y, nb_col, row_count_y, "test_detection_y");

        printf("grid y = %lix%li\n", row_count_y, nb_col);
    }
return 0;
/*
    if (nb_indices_x > nb_indices_y)
    {
        qsort(boxes, n, sizeof(*boxes), cmp_box_tolerance_x);
        if (out_row_count) *out_row_count = nb_row;
        if (out_col_count) *out_col_count = col_count_x;


        printf("%li %li\n", *out_row_count, *out_col_count);
        for (size_t i = 0; i < nb_indices_x; i++)
        {
            if (i >= max_letters)
                errx(EXIT_FAILURE, "max_letters = %li", max_letters);
            out_letters[i] = (GridLetter){boxes[indices_x[i]], i / col_count_x, i % col_count_x};
        }
        free(indices_x);
        free(indices_y);
        return nb_indices_x;
    }
    else
    {
        if (out_row_count) *out_row_count = row_count_y;
        if (out_col_count) *out_col_count = nb_col;

        printf("%li %li\n", *out_row_count,  *out_col_count);
        for (size_t i = 0; i < nb_indices_y; i++)
        {
            if (i >= max_letters)
                errx(EXIT_FAILURE, "max_letters = %li", max_letters);
            out_letters[i] = (GridLetter){boxes[indices_y[i]], i % nb_col, i / nb_col};
        }
        free(indices_x);
        free(indices_y);
        return nb_indices_y;
    }
    */
}

size_t group_letters_into_words(Box *boxes, size_t n, WordLetter *out_letters, size_t max_letters, size_t *out_words_count)
{
    return 0;
}