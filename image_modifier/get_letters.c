#include <err.h>
#include <math.h>
#include "get_letters.h"

void fill(SDL_Surface *img, int x, int y, char *visited, int *x_min, int *x_max, int *y_min, int *y_max)
{
    const int dx[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    const int dy[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    size_t stack_size = img->w * img->h;
    int *stack_x = calloc(stack_size, sizeof(int));
    int *stack_y = calloc(stack_size, sizeof(int));
    int top = 0;
    stack_x[top] = x;
    stack_y[top] = y;
    while (top >= 0)
    {
        int cx = stack_x[top];
        int cy = stack_y[top--];
        if (cx < *x_min) *x_min = cx;
        if (cx > *x_max) *x_max = cx;
        if (cy < *y_min) *y_min = cy;
        if (cy > *y_max) *y_max = cy;

        for (unsigned char k = 0; k < 8; ++k)
        {
            int nx = cx + dx[k];
            int ny = cy + dy[k];
            if (nx < 0 || nx >= img->w || ny < 0 || ny >= img->h)
                continue;
            if (visited[ny * img->w + nx])
                continue;
            Uint8 *p = img->pixels + ny * img->pitch + nx * img->format->BytesPerPixel;
            if (*p == 0) // black
            {
                visited[ny * img->w +nx] = 1;
                stack_x[++top] = nx;
                stack_y[top] = ny;
            }
        }
    }
    free(stack_x);
    free(stack_y);
}
size_t detect_connected(SDL_Surface *img, BoundingBox *boxes, size_t max_boxes)
{
    if (!img)
        errx(EXIT_FAILURE, "SDL_Surface NULL\n");
    if (SDL_MUSTLOCK(img))
        SDL_LockSurface(img);
    char *visited = calloc(img->w * img->h, sizeof(char));
    size_t count = 0;
    for (int y = 0; y < img->h; y++)
    {
        for (int x = 0; x < img->w; x++)
        {
            if (visited[y * img->w + x])
                continue;
            Uint8 *p = img->pixels + y * img->pitch + x * img->format->BytesPerPixel;
            if (*p == 0) // black
            {
                int x_min = x, x_max = x, y_min = y, y_max = y;
                fill(img, x, y, visited, &x_min, &x_max, &y_min, &y_max);
                int bw = x_max - x_min + 1;
                int bh = y_max - y_min + 1;
                if (bw > 4 && bh > 4)
                {
                    if (max_boxes <= count)
                        errx(EXIT_FAILURE, "boxe full");
                    boxes[count].x = x_min;
                    boxes[count].y = y_min;
                    boxes[count].w = bw;
                    boxes[count].h = bh;
                    count++;
                }
            }
        }
    }
    free(visited);
    if (SDL_MUSTLOCK(img))
        SDL_UnlockSurface(img);
    return count;
}

void save_letters(SDL_Surface *img, char *parent_name)
{
    BoundingBox boxes[1024];
    size_t nb = detect_connected(img, boxes, 1024);
    printf("%li boxes detected\n", nb);
    for (size_t i = 0; i < nb; i++)
    {
        SDL_Rect r = {boxes[i].x, boxes[i].y, boxes[i].w, boxes[i].h};
        SDL_Surface *sub = SDL_CreateRGBSurface(0, r.w, r.h, img->format->BitsPerPixel, img->format->Rmask, img->format->Gmask, img->format->Bmask, img->format->Amask);
        if (!sub)
            errx(EXIT_FAILURE, "SDL_CreateRGBSurface failed : %s", SDL_GetError());
        if (SDL_BlitSurface(img, &r, sub, NULL))
            errx(EXIT_FAILURE, "SDL_BlitSurface failed");
        char file_name[64] = "";
        if (boxes[i].w * 2 > img->w && boxes[i].h * 2 > img->h)
            sprintf(file_name,"images_giant/boxe_%s_%li_%ix%i_%ix%i.bmp", parent_name, i, boxes[i].x, boxes[i].y, boxes[i].w, boxes[i].h);
        else if (boxes[i].h * 2 > img->h)
            sprintf(file_name,"images_big/boxe_%s_%li_%ix%i_%ix%i.bmp", parent_name, i, boxes[i].x, boxes[i].y, boxes[i].w, boxes[i].h);
        else if (boxes[i].w * 2 > img->w)
            sprintf(file_name,"images_large/boxe_%s_%li_%ix%i_%ix%i.bmp", parent_name, i, boxes[i].x, boxes[i].y, boxes[i].w, boxes[i].h);
        else if (boxes[i].w > boxes[i].h * 2)
            sprintf(file_name,"images_bin/boxe_%s_%li_%ix%i_%ix%i.bmp", parent_name, i, boxes[i].x, boxes[i].y, boxes[i].w, boxes[i].h);
        else
            sprintf(file_name,"images_letter/boxe_%s_%li_%ix%i_%ix%i.bmp", parent_name, i, boxes[i].x, boxes[i].y, boxes[i].w, boxes[i].h);
        save_img_bmp(sub, file_name);
        SDL_FreeSurface(sub);
    }
}

