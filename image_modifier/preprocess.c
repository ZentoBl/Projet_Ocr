#include <err.h>
#include <stdlib.h>
#include "preprocess.h"

#define UCHAR_MAX  255

Uint8 grayscaleRGB(Uint8 r, Uint8 g, Uint8 b)
{
    return 0.33 * r + 0.34 * g + 0.33 * b;
}

void gray_scale(SDL_Surface *img)
{
    if (!img)
        errx(EXIT_FAILURE, "SDL_Surface NULL\n");
    if (SDL_MUSTLOCK(img))
        SDL_LockSurface(img);
    Uint8 *pixels = img->pixels;
    size_t BytesPerPixel = img->format->BytesPerPixel;
    size_t count = img->w * img->h * BytesPerPixel;
    Uint8 new_color;
    for (size_t i = 0; i < count; i += BytesPerPixel)
    {
        new_color = grayscaleRGB(pixels[i], pixels[i+1], pixels[i+2]);
        pixels[i  ] = new_color;
        pixels[i+1] = new_color;
        pixels[i+2] = new_color;
    }
    if (SDL_MUSTLOCK(img))
        SDL_UnlockSurface(img);
}

int gt(const void* a, const void* b) {
    return *(const Uint8*)a - *(const Uint8*)b;
}
int lt(const void* a, const void* b) {
    return gt(b, a);
}
// dist = 1 for 3*3, 2 for 5*5, n for (2 * n + 1) ** 2
void denoise_average(SDL_Surface *img, int dist)
{
    if (!img)
        errx(EXIT_FAILURE, "SDL_Surface NULL\n");
    if (SDL_MUSTLOCK(img))
        SDL_LockSurface(img);
    Uint8 *pixels = img->pixels;
    Uint8 *copy = malloc(img->h * img->pitch);
    memcpy(copy, pixels, img->h * img->pitch);
    for (int y = 0; y < img->h; y++)
        for (int x = 0; x < img->w; x++)
        {            
            int x1 = x-dist < 0 ? 0 : x-dist;
            int x2 = img->w-1 < x+dist ? img->w-1 : x+dist;
            int y1 = y-dist < 0 ? 0 : y-dist;
            int y2 = img->h-1 < y+dist ? img->h-1 : y+dist;
            Uint8 r = 0, g = 0, b = 0;
            int k = 0;
            for (int dx = x1; dx <= x2; dx++)
                for (int dy = y1; dy <= y2; dy++)
                {
                    r += copy[dy * img->pitch + dx * img->format->BytesPerPixel];
                    g += copy[dy * img->pitch + dx * img->format->BytesPerPixel + 1];
                    b += copy[dy * img->pitch + dx * img->format->BytesPerPixel + 2];
                    k++;
                }
            pixels[y * img->pitch + x * img->format->BytesPerPixel    ] = r / k;
            pixels[y * img->pitch + x * img->format->BytesPerPixel + 1] = g / k;
            pixels[y * img->pitch + x * img->format->BytesPerPixel + 2] = b / k;
        }
    if (SDL_MUSTLOCK(img))
        SDL_UnlockSurface(img);
    free(copy);
}

// dist = 1 for 3*3, 2 for 5*5, n for (2 * n + 1) ** 2
// percent : near 0           for erosion : pixels near dark  pixel become dark
// percent : near UCHAR_MAX   for dilation: pixels near light pixel become light
// percent : UCHAR_MAX / 2    for median
// In a ordered list, for index remainder != 0
// priority < 0 : chosen previous l[k]
// priority = 0 : chosen average  (l[k]+l[k+1])/2
// priority > 0 : chosen next     l[k+1]
void denoise_percent(SDL_Surface *img, int dist, unsigned char percent, char priority)
{
    if (!img)
        errx(EXIT_FAILURE, "SDL_Surface NULL\n");
    if (SDL_MUSTLOCK(img))
        SDL_LockSurface(img);
    Uint8 *pixels = img->pixels;
    Uint8 *copy = malloc(img->h * img->pitch);
    memcpy(copy, pixels, img->h * img->pitch);
    Uint8 *l_r = malloc((2 * dist + 1) * (2 * dist + 1) * sizeof(Uint8));
    Uint8 *l_g = malloc((2 * dist + 1) * (2 * dist + 1) * sizeof(Uint8));
    Uint8 *l_b = malloc((2 * dist + 1) * (2 * dist + 1) * sizeof(Uint8));
    for (int y = 0; y < img->h; y++)
        for (int x = 0; x < img->w; x++)
        {
            int x1 = x-dist < 0 ? 0 : x-dist;
            int x2 = img->w-1 < x+dist ? img->w-1 : x+dist;
            int y1 = y-dist < 0 ? 0 : y-dist;
            int y2 = img->h-1 < y+dist ? img->h-1 : y+dist;
            size_t k = 0;
            for (int dx = x1; dx <= x2; dx++)
                for (int dy = y1; dy <= y2; dy++)
                {
                    l_r[k] = copy[dy * img->pitch + dx * img->format->BytesPerPixel];
                    l_g[k] = copy[dy * img->pitch + dx * img->format->BytesPerPixel + 1];
                    l_b[k++] = copy[dy * img->pitch + dx * img->format->BytesPerPixel + 2];
                }
            qsort(l_r, k, sizeof(Uint8), gt);
            qsort(l_g, k, sizeof(Uint8), gt);
            qsort(l_b, k, sizeof(Uint8), gt);
            size_t i = ((k - 1) * percent) / UCHAR_MAX;
            if (((k - 1) * percent) % UCHAR_MAX)
            {
                if (priority == 0)
                {
                    pixels[y * img->pitch + x * img->format->BytesPerPixel    ] = (l_r[i] + l_r[i + 1]) / 2;
                    pixels[y * img->pitch + x * img->format->BytesPerPixel + 1] = (l_g[i] + l_g[i + 1]) / 2;
                    pixels[y * img->pitch + x * img->format->BytesPerPixel + 2] = (l_b[i] + l_b[i + 1]) / 2;
                    continue;
                }
                if (priority > 0)
                    i++;
            }
            pixels[y * img->pitch + x * img->format->BytesPerPixel    ] = l_r[i];
            pixels[y * img->pitch + x * img->format->BytesPerPixel + 1] = l_g[i];
            pixels[y * img->pitch + x * img->format->BytesPerPixel + 2] = l_b[i];
        }
    free(l_r);
    free(l_g);
    free(l_b);
    if (SDL_MUSTLOCK(img))
        SDL_UnlockSurface(img);
    free(copy);
}
// dist = 1 for 3*3, 2 for 5*5, n for (2 * n + 1) ** 2
// In a ordered list, for index remainder != 0
// priority < 0 : chosen previous l[k]
// priority = 0 : chosen average  (l[k]+l[k+1])/2
// priority > 0 : chosen next     l[k+1]
void denoise_median(SDL_Surface *img, int dist, char priority)
{
    denoise_percent(img, dist, UCHAR_MAX / 2, priority);
}
// dist = 1 for 3*3, 2 for 5*5, n for (2 * n + 1) ** 2
// percent near 0   for opening : erosion then dilation of light pixel -> without light pixels alone
// percent near 255 for closing : dilation then erosion of light pixel -> without dark  pixels alone
void denoise_morphology(SDL_Surface *img, int dist, unsigned char percent)
{
    denoise_percent(img, dist, percent, percent - UCHAR_MAX / 2);
    denoise_percent(img, dist, UCHAR_MAX - percent, UCHAR_MAX / 2 - percent);
}

void denoise_min_or_max(SDL_Surface *img, int dist, __compar_fn_t f)
{
    // dist = 1 for 3*3, 2 for 5*5, n for (2 * n + 1) ** 2
    if (!img)
        errx(EXIT_FAILURE, "SDL_Surface NULL\n");
    if (SDL_MUSTLOCK(img))
        SDL_LockSurface(img);
    Uint8 *pixels = img->pixels;
    Uint8 *copy = malloc(img->h * img->pitch);
    memcpy(copy, pixels, img->h * img->pitch);
    for (int y = 0; y < img->h; y++)
        for (int x = 0; x < img->w; x++)
        {            
            int x1 = x-dist < 0 ? 0 : x-dist;
            int x2 = img->w-1 < x+dist ? img->w-1 : x+dist;
            int y1 = y-dist < 0 ? 0 : y-dist;
            int y2 = img->h-1 < y+dist ? img->h-1 : y+dist;
            Uint8 r = pixels[y * img->pitch + x * img->format->BytesPerPixel];
            Uint8 g = pixels[y * img->pitch + x * img->format->BytesPerPixel + 1];
            Uint8 b = pixels[y * img->pitch + x * img->format->BytesPerPixel + 2];
            for (int dx = x1; dx <= x2; dx++)
                for (int dy = y1; dy <= y2; dy++)
                {
                    if (f(&r, &copy[dy * img->pitch + dx * img->format->BytesPerPixel]) > 0)
                        r = copy[dy * img->pitch + dx * img->format->BytesPerPixel];
                    if (f(&g, &copy[dy * img->pitch + dx * img->format->BytesPerPixel + 1]) > 0)
                        g = copy[dy * img->pitch + dx * img->format->BytesPerPixel + 1];
                    if (f(&b, &copy[dy * img->pitch + dx * img->format->BytesPerPixel + 2]) > 0)
                        b = copy[dy * img->pitch + dx * img->format->BytesPerPixel + 2];
                }
            pixels[y * img->pitch + x * img->format->BytesPerPixel] = r;
            pixels[y * img->pitch + x * img->format->BytesPerPixel + 1] = g;
            pixels[y * img->pitch + x * img->format->BytesPerPixel + 2] = b;
        }
    if (SDL_MUSTLOCK(img))
        SDL_UnlockSurface(img);
    free(copy);
}

// without light pixels alone
// dist = 1 for 3*3, 2 for 5*5, n for (2 * n + 1) ** 2
void denoise_opening(SDL_Surface *img, int dist)
{
    denoise_min_or_max(img, dist, gt); // erosion : pixels near dark  pixel become dark
    denoise_min_or_max(img, dist, lt); // dilation: pixels near light pixel become light
}
// without dark pixels alone
// dist = 1 for 3*3, 2 for 5*5, n for (2 * n + 1) ** 2
void denoise_closing(SDL_Surface *img, int dist)
{
    denoise_min_or_max(img, dist, lt); // dilation: pixels near light pixel become light
    denoise_min_or_max(img, dist, gt); // erosion : pixels near dark  pixel become dark
}

Uint8 get_threshold(SDL_Surface *img)
{
    unsigned long hist[256] = {0};
    Uint8 *pixels = img->pixels;
    size_t BytesPerPixel = img->format->BytesPerPixel;
    size_t count = img->w * img->h * BytesPerPixel;
    for (size_t i = 0; i < count; i += BytesPerPixel)
        hist[grayscaleRGB(pixels[i], pixels[i+1], pixels[i+2])]++;
    unsigned long total = img->w * img->h;
    double sum = 0.0;
    for (size_t t = 0; t < 256; ++t)
        sum += t * hist[t];
    double sumB = 0.0;
    int wB = 0;
    int wF = 0;
    double var_max = 0.0;
    Uint8 threshold = 0;
    for (size_t t = 0; t < 256; ++t)
    {
        wB += hist[t];
        if (wB == 0)
            continue;
        wF = total - wB;
        if (wF == 0)
            break;
        sumB += (double)(t * hist[t]);
        double mB = sumB / wB;
        double mF = (sum - sumB) / wF;
        double var_between = (double)wB * (double)wF * (mB - mF) * (mB - mF);
        if (var_between > var_max)
        {
            var_max = var_between;
            threshold = t;
        }
    }
    return threshold;
}

void to_binary(SDL_Surface *img)
{
    if (!img)
        errx(EXIT_FAILURE, "SDL_Surface NULL\n");
    if (SDL_MUSTLOCK(img))
        SDL_LockSurface(img);
    Uint8 threshold = get_threshold(img);
    Uint8 *pixels = img->pixels;
    size_t BytesPerPixel = img->format->BytesPerPixel;
    size_t count = img->w * img->h * BytesPerPixel;
    Uint8 new_color;
    for (size_t i = 0; i < count; i += BytesPerPixel)
    {
        new_color = grayscaleRGB(pixels[i], pixels[i+1], pixels[i+2]) > threshold ? 255 : 0;
        pixels[i  ] = new_color;
        pixels[i+1] = new_color;
        pixels[i+2] = new_color;
    }
    if (SDL_MUSTLOCK(img))
        SDL_UnlockSurface(img);
}

// Clear black pixels in binary image if they are in groups of less than min_size
void denoise_by_count(SDL_Surface *img, int min_size)
{
    if (!img)
        errx(EXIT_FAILURE, "SDL_Surface NULL\n");
    if (SDL_MUSTLOCK(img))
        SDL_LockSurface(img);
    Uint8 *pixels = (Uint8 *)img->pixels;
    Uint8 *visited = calloc(img->w * img->h, sizeof(Uint8));
    int stack_max = img->w * img->h / 10;
    int *stack = malloc(sizeof(int) * stack_max * 2);
    for (int y = 0; y < img->h; y++)
    {
        for (int x = 0; x < img->w; x++)
        {
            if (visited[y * img->w + x])
                continue;
            Uint8 *p = pixels + y * img->pitch + x * img->format->BytesPerPixel;
            if (p[0] != 0) // white pixel
                continue;
            // new component
            int stack_size = 0;
            int count = 0;
            stack[stack_size++] = x;
            stack[stack_size++] = y;
            while (stack_size > 0)
            {
                int cy = stack[--stack_size];
                int cx = stack[--stack_size];
                if (cx < 0 || cy < 0 || cx >= img->w || cy >= img->h)
                    continue;
                if (visited[cy * img->w + cx])
                    continue;
                Uint8 *cp = pixels + cy * img->pitch + cx * img->format->BytesPerPixel;
                if (cp[0] != 0)
                    continue;
                visited[cy * img->w + cx] = 1;
                count++;
                stack[stack_size++] = cx + 1; stack[stack_size++] = cy;
                stack[stack_size++] = cx - 1; stack[stack_size++] = cy;
                stack[stack_size++] = cx;     stack[stack_size++] = cy + 1;
                stack[stack_size++] = cx;     stack[stack_size++] = cy - 1;
            }
            if (count < min_size) // too small -> clear pixels
            {
                stack_size = 0;
                stack[stack_size++] = x;
                stack[stack_size++] = y;
                while (stack_size > 0)
                {
                    int cy = stack[--stack_size];
                    int cx = stack[--stack_size];
                    if (cx < 0 || cy < 0 || cx >= img->w || cy >= img->h)
                        continue;
                    if (visited[cy * img->w + cx] != 1)
                        continue;
                    Uint8 *cp = pixels + cy * img->pitch + cx * img->format->BytesPerPixel;
                    if (cp[0] == 0)
                    {
                        cp[0] = 255;
                        cp[1] = 255;
                        cp[2] = 255;
                    }
                    visited[cy * img->w + cx] = 2; // clear
                    stack[stack_size++] = cx + 1; stack[stack_size++] = cy;
                    stack[stack_size++] = cx - 1; stack[stack_size++] = cy;
                    stack[stack_size++] = cx;     stack[stack_size++] = cy + 1;
                    stack[stack_size++] = cx;     stack[stack_size++] = cy - 1;
                }
            }
        }
    }
    free(stack);
    free(visited);
    if (SDL_MUSTLOCK(img))
        SDL_UnlockSurface(img);
}


void process(SDL_Surface *img)
{
    gray_scale(img);
    to_binary(img);
    denoise_by_count(img, 20);
}
