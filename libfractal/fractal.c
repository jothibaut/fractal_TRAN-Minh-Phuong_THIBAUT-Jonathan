#include <stdlib.h>
#include "fractal.h"

struct fractal *fractal_new(const char *name, int width, int height, double a, double b)
{
    /* TODO */
	// coucou test 1 2 3
    return NULL;
}

void fractal_free(struct fractal *f)
{
    	free(f->name);
	int i;
	for (int i = 0; i<f->height; i++){
	free(f->pixTab[i]);
	}
	free(f->pixTab);
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
