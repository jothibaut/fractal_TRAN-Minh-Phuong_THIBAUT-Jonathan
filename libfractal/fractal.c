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
    /* TODO */
    //test
}

const char *fractal_get_name(const struct fractal *f)
{
    /* TODO */
    return NULL;
}

int fractal_get_value(const struct fractal *f, int x, int y)
{
    /* TODO */
    return 0;
}

void fractal_set_value(struct fractal *f, int x, int y, int val)
{
    /* TODO */
}

int fractal_get_width(const struct fractal *f)
{
    /* TODO */
    return 0;
}

int fractal_get_height(const struct fractal *f)
{
    /* TODO */
    return 0;
}

double fractal_get_a(const struct fractal *f)
{
    /* TODO */
    return 0;
}

double fractal_get_b(const struct fractal *f)
{
    /* TODO */
    return 0;
}
