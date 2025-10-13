#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int Check(int x,int y, int dx, int dy, char* word,int rows , int cols, char**grid)
{
    int x1 = x;
    int y1 = y;
    int i = 1;
    while(word[i] != '\0')
    {
        x += dx;
        y += dy;
        if(x < 0 || y < 0 || y >= rows || x >= cols)
            return 1;
        if(grid[y][x] != word[i])
            return 1;
        i++;
    }
    printf("(%d,%d)(%d,%d)\n",x1,y1,x,y);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <gridfile> <word>\n", argv[0]);
        return 1;
    }

    char *filename = argv[1];
    char *word = argv[2];

    int wordlen = 0;
    while(word[wordlen] != '\0')
    {
        if(word[wordlen] >= 97 && word[wordlen] <= 122 )
        {
            word[wordlen] -= 32;
        }
        wordlen++;
    }

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Erreur ouverture fichier");
        return 1;
    }
    char **grid = NULL;
    char buffer[1024];
    int rows = 0, cols = 0;

    while (fgets(buffer, sizeof(buffer), fp)) 
    {
        int len = strlen(buffer);
        if (buffer[len - 1] == '\n') buffer[len - 1] = '\0';
        if (cols == 0) cols = strlen(buffer);

        grid = realloc(grid, (rows + 1) * sizeof(char *));
        grid[rows] = malloc((cols + 1) * sizeof(char));
        strcpy(grid[rows], buffer);
        rows++;
    }
    fclose(fp);

    for (int y = 0; y < rows; y++) 
    {
        for (int x = 0; x < cols; x++) 
        {
            if (grid[y][x] == word[0]) 
            {
                if(!Check(x,y,1,0,word,rows,cols,grid))
                {
                    for(int i = 0; i < rows;i++)
                    {
                        free(grid[i]);
                    }
                    free(grid);
                    return 0;
                }
                if(!Check(x,y,-1,0,word,rows,cols,grid))
                {
                    for(int i = 0; i < rows;i++)
                    {
                        free(grid[i]);
                    }
                    free(grid);
                    return 0;
                }
                if(!Check(x,y,0,1,word,rows,cols,grid))
                {
                    for(int i = 0; i < rows;i++)
                    {
                        free(grid[i]);
                    }
                    free(grid);
                    return 0;
                }
                if(!Check(x,y,0,-1,word,rows,cols,grid))
                {
                    for(int i = 0; i < rows;i++)
                    {
                        free(grid[i]);
                    }
                    free(grid);
                    return 0;
                }
                if(!Check(x,y,1,1,word,rows,cols,grid))
                {
                    for(int i = 0; i < rows;i++)
                    {
                        free(grid[i]);
                    }
                    free(grid);
                    return 0;
                }
                if(!Check(x,y,-1,1,word,rows,cols,grid))
                {
                    for(int i = 0; i < rows;i++)
                    {
                        free(grid[i]);
                    }
                    free(grid);
                    return 0;
                }
                if(!Check(x,y,1,-1,word,rows,cols,grid))
                {
                    for(int i = 0; i < rows;i++)
                    {
                        free(grid[i]);
                    }
                    free(grid);
                    return 0;
                }
                if(!Check(x,y,-1,-1,word,rows,cols,grid))
                {
                    for(int i = 0; i < rows;i++)
                    {
                        free(grid[i]);
                    }
                    free(grid);
                    return 0;
                }
            }
        }
    }


    for(int i = 0; i < rows;i++)
    {
        free(grid[i]);
    }
    free(grid);
    printf("Not found\n");

    return 0;
}
