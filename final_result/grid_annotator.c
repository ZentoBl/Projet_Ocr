#include "letter_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <math.h>

#define TOLERANCE 20 // (between 2 and 30 for good detection)

#define BORDER 3
#define ALPHA_BORDER 128
#define ALPHA_FILL 90
#define POWER 0.6 // (0 for [], 0.4 for (), 0.5 for O, 1 for <>, 2 for {}, 4 for -[]-)

#define COLOR_NOT_FOUND 32, 16, 16, 180 // r, g, b, a (Uint8 : 0 to 225)

static void pick_random_color(int index, Uint8 *r, Uint8 *g, Uint8 *b)
{
    // deterministic-ish per-index colors
    Uint32 seed = (Uint32)(index * 1664525u + 1013904223u);
    seed ^= 0x9e3779b9u;
    *r = (Uint8)((seed >> 16) & 0xFF);
    *g = (Uint8)((seed >> 8) & 0xFF);
    *b = (Uint8)(seed & 0xFF);
    if (*r + *g + *b < 60) // ensure not too dark
    {
        *r ^= 0x55;
        *g ^= 0x33;
        *b ^= 0x77;
    }
}
static void put_pixel_alpha(SDL_Surface *img, int x, int y, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    if (x < 0 || y < 0 || x >= img->w || y >= img->h)
        return;
    Uint8 *p = (Uint8*)img->pixels + y * img->pitch + x * 4;
    Uint8 br = p[0];
    Uint8 bg = p[1];
    Uint8 bb = p[2];
    float af = a / 255.0f;
    float ia = 1.0f - af;
    p[0] = (Uint8)(br * ia + r * af);
    p[1] = (Uint8)(bg * ia + g * af);
    p[2] = (Uint8)(bb * ia + b * af);
}

typedef struct {float cx, cy, t1x, t1y, t2x, t2y;} Corner;

static void fill_quad(SDL_Surface *img, Corner *corners,
    float radius, Uint8 r, Uint8 g, Uint8 b)
{
    float minx = fminf(fminf(corners[0].cx,corners[1].cx), fminf(corners[2].cx,corners[3].cx));
    float maxx = fmaxf(fmaxf(corners[0].cx,corners[1].cx), fmaxf(corners[2].cx,corners[3].cx));
    float miny = fminf(fminf(corners[0].cy,corners[1].cy), fminf(corners[2].cy,corners[3].cy));
    float maxy = fmaxf(fmaxf(corners[0].cy,corners[1].cy), fmaxf(corners[2].cy,corners[3].cy));
    float threshold = powf(radius, POWER);
    for (int y = (int)miny; y <= (int)maxy; y++)
    for (int x = (int)minx; x <= (int)maxx; x++)
    {
        float vx = x + 0.5f;
        float vy = y + 0.5f;
        float d1 = (corners[1].cx-corners[0].cx)*(vy-corners[0].cy) - (corners[1].cy-corners[0].cy)*(vx-corners[0].cx);
        float d2 = (corners[2].cx-corners[1].cx)*(vy-corners[1].cy) - (corners[2].cy-corners[1].cy)*(vx-corners[1].cx);
        float d3 = (corners[3].cx-corners[2].cx)*(vy-corners[2].cy) - (corners[3].cy-corners[2].cy)*(vx-corners[2].cx);
        float d4 = (corners[0].cx-corners[3].cx)*(vy-corners[3].cy) - (corners[0].cy-corners[3].cy)*(vx-corners[3].cx);
        char found = 0;
        for (size_t i = 0; i < 4; i++)
        {
            float dx = x - corners[i].cx;
            float dy = y - corners[i].cy;
            int res = powf(fabsf(dx * -corners[i].t1x + dy * -corners[i].t1y), POWER)
                    + powf(fabsf(dx * -corners[i].t2x + dy * -corners[i].t2y), POWER);
            if (res < threshold)
            {
                found = 1;
                break;
            }
        }
        if (found) continue;
        if ((d1>=0 && d2>=0 && d3>=0 && d4>=0) ||
            (d1<=0 && d2<=0 && d3<=0 && d4<=0))
            put_pixel_alpha(img, x, y, r, g, b, ALPHA_FILL);
    }
}
static void thick_line(SDL_Surface *img,
    float x1, float y1, float x2, float y2,
    Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    float L = sqrtf(dx*dx + dy*dy);
    dx /= L;
    dy /= L;
    float px = -dy;
    float py = dx;
    float half = BORDER / 2.0f;
    for (float w = -half; w <= half; w += 0.5f)
    {
        float ox = px * w;
        float oy = py * w;
        float x = x1;
        float y = y1;
        for (int i = 0; i < (int)L; i++)
        {
            float fx = x + ox;
            float fy = y + oy;
            int ix = (int)fx;
            int iy = (int)fy;
            float tx = fx - ix;
            float ty = fy - iy;
            put_pixel_alpha(img, ix,  iy,   r,g,b,(Uint8)(a*(1-tx)*(1-ty)));
            put_pixel_alpha(img, ix+1,iy,   r,g,b,(Uint8)(a*  tx  *(1-ty)));
            put_pixel_alpha(img, ix,  iy+1, r,g,b,(Uint8)(a*(1-tx)*  ty));
            put_pixel_alpha(img, ix+1,iy+1, r,g,b,(Uint8)(a*  tx  *  ty));
            x += dx;
            y += dy;
        }
    }
}
static void aa_arc(SDL_Surface *img, float cx, float cy, float radius,
    float start_ang, float end_ang, Uint8 r, Uint8 g, Uint8 b)
{
    while (start_ang > end_ang)
        end_ang += 2.0f * M_PI;
    const float step = (3.0f * M_PI / 180.0f);
    float outer = radius + BORDER * 0.5f;
    float inner = radius - BORDER * 0.5f;
    if (inner < 0.5f) inner = 0.5f;
    for (float a = start_ang; a <= end_ang + 1e-7f; a += step)
    {
        float sx = cosf(a);
        float sy = sinf(a);
        for (float rr = inner; rr <= outer; rr += 0.5f)
        {
            float fx = cx + sx * rr;
            float fy = cy + sy * rr;
            int ix = (int)fx;
            int iy = (int)fy;
            float tx = fx - ix;
            float ty = fy - iy;
            put_pixel_alpha(img, ix,  iy,   r,g,b,(Uint8)(ALPHA_BORDER*(1-tx)*(1-ty)));
            put_pixel_alpha(img, ix+1,iy,   r,g,b,(Uint8)(ALPHA_BORDER*  tx  *(1-ty)));
            put_pixel_alpha(img, ix,  iy+1, r,g,b,(Uint8)(ALPHA_BORDER*(1-tx)*  ty));
            put_pixel_alpha(img, ix+1,iy+1, r,g,b,(Uint8)(ALPHA_BORDER*  tx  *  ty));
        }
    }
}
void draw_wordsearch_box(SDL_Surface *img, int x1, int y1, int x2, int y2, 
    int width, Uint8 r, Uint8 g, Uint8 b)
{
    width = (width + BORDER) / 2 + 4;
    int bx1 = x1, by1 = y1, bx2 = x2, by2 = y2;
    float dx = x2 - x1;
    float dy = y2 - y1;
    if (dx > TOLERANCE)
    {
        x1 -= width;
        x2 += width;
        if (dy < -TOLERANCE)
        {
            bx1 -= width/2;
            bx2 += width/2;
            by1 += width/2;
            by2 -= width/2;
        }
    }
    else if (dx < -TOLERANCE)
    {
        x1 += width;
        x2 -= width;
        if (dy > TOLERANCE)
        {
            bx1 += width/2;
            bx2 -= width/2;
            by1 -= width/2;
            by2 += width/2;
        }
    }
    if (dy > TOLERANCE)
    {
        y1 -= width;
        y2 += width;
    }
    else if (dy < -TOLERANCE)
    {
        y1 += width;
        y2 -= width;
    }
    float L = sqrtf(dx*dx + dy*dy);
    if (L < 1) L = 1;
    dx /= L;
    dy /= L;
    float nx = -dy;
    float ny = dx;
    thick_line(img, bx1 + nx*width, by1 + ny*width, bx2 + nx*width, by2 + ny*width, r,g,b,ALPHA_BORDER);
    thick_line(img, bx2 - nx*width, by2 - ny*width, bx1 - nx*width, by1 - ny*width, r,g,b,ALPHA_BORDER);
    float Ax = x1 + nx * width;
    float Ay = y1 + ny * width;
    float Bx = x2 + nx * width;
    float By = y2 + ny * width;
    float Cx = x2 - nx * width;
    float Cy = y2 - ny * width;
    float Dx = x1 - nx * width;
    float Dy = y1 - ny * width;
    Corner corners[4] = {
        { Ax, Ay, +dx, +dy, -nx, -ny },{ Bx, By, -nx, -ny, -dx, -dy },
        { Cx, Cy, -dx, -dy, +nx, +ny },{ Dx, Dy, +nx, +ny, +dx, +dy },};
    fill_quad(img, corners, width, r, g, b);
    for (int i = 0; i < 4; i++)
    {
        float cx = corners[i].cx + corners[i].t1x * width + corners[i].t2x * width;
        float cy = corners[i].cy + corners[i].t1y * width + corners[i].t2y * width;
        float start = atan2f(-corners[i].t2y, -corners[i].t2x);
        float end   = atan2f(-corners[i].t1y, -corners[i].t1x);
        aa_arc(img, cx, cy, width, start, end, r,g,b);
    }
}

int main(int argc, char **argv)
{
    if (argc != 6)
        errx(EXIT_FAILURE, "Usage: %s <grid.png> <grid_letters_folder> <word_letters_folder> <solver_output.txt> <annotated.png>\n", argv[0]);

    const char *grid_image = argv[1];
    const char *grid_letters_folder = argv[2];
    const char *word_letters_folder = argv[3];
    const char *solver_source = argv[4];
    const char *output_image = argv[5];

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        errx(EXIT_FAILURE, "SDL_Init error: %s\n", SDL_GetError());
    int flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(flags) & flags) != flags)
        errx(EXIT_FAILURE, "IMG_Init error: %s\n", IMG_GetError());

    SDL_Surface *img = IMG_Load(grid_image);
    if (!img)
        errx(EXIT_FAILURE, "IMG_Load(%s) failed: %s\n", grid_image, IMG_GetError());

    // load grid letters
    int nb_grid_letters = 0;
    LetterInfo *grid_letters = load_letters_from_folder(grid_letters_folder, &nb_grid_letters);
    if (!grid_letters || nb_grid_letters == 0)
        errx(EXIT_FAILURE, "No grid letters loaded from %s (nb=%d)\n", grid_letters_folder, nb_grid_letters);
    printf("nb_grid_letters = %i\n", nb_grid_letters);
    // optionally load word letters (for list words)
    int nb_word_letters = 0;
    LetterInfo *word_letters = NULL;
    int use_word_letters = (strcmp(word_letters_folder, "-") != 0);
    if (use_word_letters) {
        word_letters = load_letters_from_folder(word_letters_folder, &nb_word_letters);
        if (!word_letters || nb_word_letters == 0) {
            fprintf(stderr, "Warning: no word letters loaded from %s (nb=%d). List-words will be skipped.\n",
                    word_letters_folder, nb_word_letters);
            use_word_letters = 0;
        }
    }
    printf("nb_word_letters = %i\n", nb_word_letters);

    // open solver input: either file or popen
    FILE *solver_fp = fopen(solver_source, "r");
    if (!solver_fp)
        errx(EXIT_FAILURE, "fopen solver_source failed\n");
    // read lines and draw all ovals
    char line[1024];
    for (int word_id = 0; fgets(line, sizeof(line), solver_fp); word_id++)
    {
        // trim leading spaces
        char *p = line;
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == '\0' || *p == '\n') continue;

        if(strcmp("Not found\n", p) == 0)
        {
            printf("%s", p);
            LetterInfo *first = get_first_letter(word_letters, nb_word_letters, word_id);
            LetterInfo *last  = get_last_letter(word_letters, nb_word_letters, word_id);
            thick_line(img, first->x-first->w*0.5f, first->y+first->h*0.2f,
                last->x+last->w*1.5f, last->y+last->h*0.8f, COLOR_NOT_FOUND);
        }
        else
        {
            int col1, row1, col2, row2;
            int n = sscanf(p, "(%d,%d)(%d,%d)\n", &col1, &row1, &col2, &row2);
            if (n != 4)
            {
                fprintf(stderr, "Warning: detect %d/4 something in (%s), skipping\n", n, p);
                continue;
            }
            Uint8 rr, gg, bb;
            pick_random_color(word_id, &rr, &gg, &bb);
            // find letters in grid_letters
            LetterInfo *s = find_letter(grid_letters, nb_grid_letters, col1, row1);
            LetterInfo *e = find_letter(grid_letters, nb_grid_letters, col2, row2);
            int width_max = (int)fmax(5.0, fmax(fmax(s->w, s->h), fmax(e->w, e->h)));
            printf("s=%dx%d,%dx%d, e=%dx%d,%dx%d\n", s->col, s->row, s->w, s->h, e->col, e->row, e->w, e->h);
            if (!s)
            {
                fprintf(stderr, "Warning: grid letters not found for (%d,%d)-(%d,%d), skipping\n",
                        col1, row1, col2, row2);
                continue;
            }
            draw_wordsearch_box(img,
                s->x + s->w / 2, s->y + s->h / 2,
                e->x + e->w / 2, e->y + e->h / 2, width_max, rr, gg, bb);

            if (use_word_letters)
            {
                LetterInfo *first = get_first_letter(word_letters, nb_word_letters, word_id);
                LetterInfo *last  = get_last_letter(word_letters, nb_word_letters, word_id);
                if (!first || !last)
                {
                    fprintf(stderr, "Warning: no word letters with word_id=%d\n", word_id);
                    continue;
                }
                printf("list : (%d,%d)(%d,%d) and (%d,%d)(%d,%d)\n",
                    first->x, first->y, first->w, first->h,
                    last->x, last->y, last->w, last->h);
                draw_wordsearch_box(img, first->x + first->w / 2,
                    first->y + first->h / 2, last->x + last->w / 2,
                    last->y + last->h / 2, width_max, rr, gg, bb);
            }
        }
    }
    fclose(solver_fp);
    if (IMG_SavePNG(img, output_image) != 0)
        err(EXIT_FAILURE, "IMG_SavePNG failed: %s\n", IMG_GetError());
    printf("Annotated image saved to %s\n", output_image);
    SDL_FreeSurface(img);
    free(grid_letters);
    free(word_letters);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
