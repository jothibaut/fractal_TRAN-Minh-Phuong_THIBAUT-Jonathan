/*
* Ce test presente une version simplifiee de la fonction compute.
* En effet, le but ici est de tester le comportement de la fonction en elle-meme
* et non la maniere dont elle gere le multithreading, suffisamment bien illustre
* par l'affichage des fractales.
*/ 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <string.h>
#include "CUnit/CUnit.h"
#include "CUnit/Basic.h"
#define MAX_ITER 4096


struct buffer *buf1;
struct buffer *buf2;
struct fractal *fract1;
struct fractal *fract2;
bool allFracComputed = false;

struct fractal {
    char *name;
    int **pixTab;
    unsigned int height;
    unsigned int width;
    double a;
    double b;
    double average;
};

struct fractal *fractal_new(char *name, int width, int height, double a, double b)
{
    int i;
    struct fractal *theFract = (struct fractal *) malloc(sizeof(struct fractal));
    //theFract->name = (char *) malloc(65*sizeof(char));
    theFract->name = name;
    theFract->height = height;
    theFract->width = width;
    theFract->a = a;
    theFract->b = b;
    theFract->pixTab = (int **) malloc(width*sizeof(int*));
    for(i=0;i<width;i++){
        theFract->pixTab[i] = (int *) malloc(height*sizeof(int));
    }
    return theFract;
}

void fractal_free(struct fractal *f)
{
    free(f->name);
  int i;
  for (i = 0; i< f->width; i++){
  free(f->pixTab[i]);
  }
  free(f->pixTab);
  free(f);
}



struct buffer {
  struct fractal **tab;
  int n;
  int front;
  int rear;
  pthread_mutex_t mutex;
  sem_t full;
  sem_t empty;
};

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

static int iter_julia(double zx, double zy, double a, double b, int it)
{
    /* prevent infinite loop */
    if (it > MAX_ITER)
        return 0;

    /* prevent leaving Julia set
     * if distance to origin >= 2
     */
    if (zx*zx + zy*zy >= 4.0)
        return it;

    /* compute next iteration
     * f(z) = z^2 + c
     * z = x + yi
     * c = a + bi
     */
    return iter_julia(zx*zx - zy*zy + a, 2*zx*zy + b, a, b, it+1);
}

int fractal_compute_value(struct fractal *f, int x, int y)
{
    double zx, zy;
    double a, b;
    int val;
    int w, h;

    a = fractal_get_a(f);
    b = fractal_get_b(f);

    w = fractal_get_width(f);
    h = fractal_get_height(f);

    zx = ((double)x / (double)w) * 3.0 - 1.5;
    zy = ((double)y / (double)h) * 3.0 - 1.5;

    val = iter_julia(zx, zy, a, b, 0);
    fractal_set_value(f, x, y, val);

    return val;
}

void buf_init(struct buffer *buf,int n){
  buf->tab = (struct fractal **) malloc(n*sizeof(struct fractal *));
  buf->n = n;
  buf->front = buf->rear = 0;
  pthread_mutex_init(&(buf->mutex),NULL);
  sem_init(&(buf->empty),0,n);
  sem_init(&(buf->full),0,0);
}

void buf_free(struct buffer *buf){
  int i;
  free(buf->tab);
  sem_destroy(&(buf->full));
  sem_destroy(&(buf->empty));
  pthread_mutex_destroy(&(buf->mutex));
  free(buf);
}

void buf_insert(struct buffer *buf,struct fractal *fract){
  sem_wait(&buf->empty);
  pthread_mutex_lock(&buf->mutex);
  buf->tab[buf->rear%buf->n] = fract;
  
  buf->rear++;
  pthread_mutex_unlock(&buf->mutex);
  sem_post(&buf->full);
}

struct fractal *buf_remove(struct buffer *buf){

  sem_wait(&(buf->full));
  pthread_mutex_lock(&(buf->mutex));
  
  struct fractal *res = buf->tab[buf->front%(buf->n)];
  buf->front++;
  
  pthread_mutex_unlock(&(buf->mutex));

  sem_post(&(buf->empty));

  return res;
}

int compute()
{
  struct fractal *fract = (struct fractal *) malloc(sizeof(struct fractal));
  int empty;
  while(allFracComputed==false){
    if(allFracComputed == true){
      return 0;
    }
    fract = buf_remove(buf1);
    //printf("%s\n", fract->name);
    if(fract == NULL){
      allFracComputed = true;
      buf_insert(buf2,NULL);
      return 0;
    }
    int i,j;
    fract->average = 0;
    for(i = 0; i<fract->width; i++){
      for(j = 0; j<fract->height; j++){
        fract->average += fractal_compute_value(fract, i, j);
      }
    }
    fract->average = fract->average/(fract->height*fract->width);
    buf_insert(buf2, fract);
  }
  return 0;
}

int launchCompute(){
  buf1 = malloc(sizeof(struct buffer));
  buf2 = malloc(sizeof(struct buffer));
  buf_init(buf1,5);
  buf_init(buf2,5);

  fract1 = fractal_new("fract1",1920,1080,0.2,0.7);
  fract2 = NULL;

  buf_insert(buf1,fract1);
  buf_insert(buf1,fract2);

  compute();

  printf("%f\n", buf2->tab[0]->average);

  return 0;
}

void test_assert_average_change(void)
{
  CU_ASSERT(buf2->tab[0]->average > 0);
}

void test_assert_last_fract_null(void)
{
  CU_ASSERT(buf2->tab[1] == NULL);
}


int main(void){

  launchCompute();


  if(CUE_SUCCESS != CU_initialize_registry())
    return CU_get_error();

  int setup(void)  { return 0; }
  int teardown(void) { return 0; }

  CU_pSuite pSuite = NULL;

  pSuite = CU_add_suite("ma_suite", setup, teardown);
  if (NULL == pSuite) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  if ((NULL == CU_add_test(pSuite, "Test assert true", test_assert_average_change)) ||
    (NULL == CU_add_test(pSuite, "Test assert true", test_assert_last_fract_null)))
  {
    CU_cleanup_registry();
    return CU_get_error();
  }

  CU_basic_run_tests();
  CU_basic_show_failures(CU_get_failure_list());

  buf_free(buf1);
  buf_free(buf2);

}
