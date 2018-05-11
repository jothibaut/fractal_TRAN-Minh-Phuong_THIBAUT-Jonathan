/*
* Ce test presente une version simplifiee de la fonction compare.
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


struct buffer *buf;
struct fractal *fract1;
struct fractal *fract2;
struct fractal *fract3;
struct result{struct result *next; struct fractal *frac;};
bool display = false;
bool isNotDispDone;
bool isDisplayDone;
int count = 0;
int res[5];

struct fractal {
    char *name;
    int **pixTab;
    unsigned int height;
    unsigned int width;
    double a;
    double b;
    double average;
};

struct buffer {
	struct fractal **tab;
	int n;
	int front;
	int rear;
	pthread_mutex_t mutex;
	sem_t full;
	sem_t empty;
};

struct fractal *fractal_new(char *name, int width, int height, double a, double b)
{
    int i;
    struct fractal *theFract = (struct fractal *) malloc(sizeof(struct fractal));
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

void buf_init(struct buffer *buf,int n){
	buf->tab = (struct fractal **) malloc(n*sizeof(struct fractal *));
	int i;
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

int compare(){
	if(!display){
		printf("perruche\n");
		struct result *res = (struct result *) malloc(sizeof(struct result));
		res->next = NULL;
		res->frac = buf_remove(buf);
		printf("arabe\n");
		struct result *temp = res;
		struct result *temp2 = res;
		struct result *temp3 = res;
		double max = res->frac->average;
		int nCompThreads;
		int countMax = 1;
		
		while (!isNotDispDone){

			struct fractal *frac = buf_remove(buf);
			printf("coucou\n");
			if(frac!=NULL){
				printf("hey\n");
				if(frac->average > max){ //Si on trouve un nouveau max, on vide la liste chainee creee au prealable
					temp = res;
					temp2 = temp;
					while(temp!=NULL){
						//fractal_free(temp->frac);
						
						temp = temp->next;
						if((temp2!=NULL)&&(temp2!=res)){
							//free(temp2);
						}
						temp2 = temp;
					}
					
					max = frac->average;
					res->frac = frac;
					
					temp = res;
					countMax = 1;
				}
				else if(frac->average == max){
					printf("pinguin\n");
					struct result *a = (struct result *) malloc(sizeof(struct result));
					a->frac = frac;
					a->next = NULL;
					temp->next = a;
					temp = temp->next;
					countMax++;
				}
				else{
					//fractal_free(frac);
				}
			}
			else{
				isNotDispDone = true;
			}
		}
		
		if(countMax==1){
			printf("DISPLAYING FRACTAL %s\n",res->frac->name);
			count++;
			//fractal_free(res->frac);
			//free(res);
		}
		else{
			temp = res;
			temp2 = temp;
			while(temp!=NULL){

				printf("DISPLAYING FRACTAL %s\n",temp->frac->name);
				count++;
				//fractal_free(temp->frac);
				temp = temp->next;
				if((temp2!=NULL)&&(temp2!=res)){
					free(temp2);
				}
				temp2 = temp;
			}

			//free(res);
		}
	}
	else{
		
		while (!isDisplayDone){
			struct fractal *frac = buf_remove(buf);
			if(frac==NULL){
				isDisplayDone = true;
				pthread_exit(NULL);
			}
			
			printf("DISPLAYING FRACTAL %s\n", frac->name);
			count++;
			//fractal_free(frac);
		}
		
	}
	return 0;
}

void test_assert_count0(void)
{
  CU_ASSERT(res[0] == 3);
}

void test_assert_count1(void)
{
  CU_ASSERT(res[1] == 2);
}

void test_assert_count2(void)
{
  CU_ASSERT(res[2] == 1);
}

void test_assert_count3(void)
{
  CU_ASSERT(res[3] == 3);
}


int launcher(){
	buf = (struct buffer *) malloc(sizeof(struct buffer));
	fract1 = fractal_new("fract1",1920,1080,0.2,0.7);	
	fract2 = fractal_new("fract2",1920,1080,0.2,0.7);	
	fract3 = fractal_new("fract3",1920,1080,0.2,0.7);

	fract1->average = 0.5;
	fract2->average = 0.5;
	fract3->average = 0.5;

	buf = (struct buffer *) malloc(sizeof(struct buffer));
	buf_init(buf,4);
	buf_insert(buf,fract1);
	buf_insert(buf,fract2);
	buf_insert(buf,fract3);
	buf_insert(buf,NULL);

	display = false;

	compare();

	res[0] = count;

	fract1->average = 0.1;
	fract2->average = 0.5;
	fract3->average = 0.5;

	count = 0;
	res[1] = count;

	fract1->average = 0.5;
	fract2->average = 0.5;
	fract3->average = 0.7;

	count = 0;
	res[2] = count;

	display = true;

	fract1->average = 0.1;
	fract2->average = 0.2;
	fract3->average = 0.3;

	count = 0;
	res[3] = count;


	return 0;
}


int main(void){
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

  if ((NULL == CU_add_test(pSuite, "First test assert compare", test_assert_count0)) ||
  	(NULL == CU_add_test(pSuite, "Second test assert compare", test_assert_count1)) ||
  	(NULL == CU_add_test(pSuite, "Third test assert compare", test_assert_count2)) ||
  	(NULL == CU_add_test(pSuite, "Fourth test assert compare", test_assert_count3)))
  {
    CU_cleanup_registry();
    return CU_get_error();
  }

  CU_basic_run_tests();
  CU_basic_show_failures(CU_get_failure_list());
  
}