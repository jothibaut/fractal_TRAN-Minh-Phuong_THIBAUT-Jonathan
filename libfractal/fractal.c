#include <stdlib.h>
#include "fractal.h"

struct fractal *fractal_new(const char *name, int width, int height, double a, double b)
{
    int i;
    struct fractal *theFract = (struct fractal *) malloc(sizeof(struct fractal));
    theFract->name = (char *) malloc(65*sizeof(char));
    theFract->name = name;
    theFract->height = height;
    theFract->width = width;
    theFract->a = a;
    theFract->b = b;
    theFract->pixTab = (int **) malloc(height*sizeof(int*));
    for(i=0;i<height;i++){
        theFract->pixTab[i] = (int *) malloc(width*sizeof(int));
    }
    return theFract;
}

void fractal_free(struct fractal *f)
{
    free((void*) f->name);
	int i;
	for (i = 0; i< (int) f->height; i++){
	free(f->pixTab[i]);
	}
	free(f->pixTab);
	free(f);
}

const char *fractal_get_name(const struct fractal *f)
{
     return f->name;
}

int fractal_get_value(const struct fractal *f, int x, int y)
{
     return f->pixTab[x][y];
}

void fractal_set_value(struct fractal *f, int x, int y, int val)
{
    f->pixTab[x][y] = val;
}

int fractal_get_width(const struct fractal *f)
{
   
    return f->width;
}

int fractal_get_height(const struct fractal *f)
{
    return f->height;
}

double fractal_get_a(const struct fractal *f)
{
    return f->a;
}

double fractal_get_b(const struct fractal *f)
{
    return f->b;
}
