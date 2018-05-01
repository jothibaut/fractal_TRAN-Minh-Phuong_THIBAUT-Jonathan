#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "fractal.h"
#include <pthread.h>
#include <semaphore.h>

pthread_mutex_t mutcount;
int count = 0;
int nthread = 0;
void buf_init(struct buffer *buf,int n){
	buf->tab = (struct fractal **) malloc(n*sizeof(struct fractal *));
	int i;
	for(i=0;i<n;i++){
		buf->tab[i] = (struct fractal *) malloc(sizeof(struct fractal));
	}
	buf->n = n;
	buf->front = buf->rear = 0;
	pthread_mutex_init(&(buf->mutex),NULL);
	sem_init(&(buf->empty),0,n);
	sem_init(&(buf->full),0,0);
}

void buf_free(struct buffer *buf){
	free(buf->tab);
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

void compute(struct buffer *buf){
	
	int countloc;
	pthread_mutex_lock(&mutcount);
	countloc = count;
	pthread_mutex_unlock(&mutcount);
	while(countloc< nthread){

		struct fractal *fract = buf_remove(buf);
		int i,j;
		fract->average = 0;
		for(i = 0; i<fract->height; i++){
		 	for(j = 0; j<fract->width; j++){
				
				fract->average += fractal_compute_value(fract, i, j);

			}
		}
		fract->average = fract->average/(fract->height*fract->width);
		pthread_mutex_lock(&mutcount);
		countloc = count;
		pthread_mutex_unlock(&mutcount);
	}
}

int main(int argc, char* argv[])
{
    return 0;
}