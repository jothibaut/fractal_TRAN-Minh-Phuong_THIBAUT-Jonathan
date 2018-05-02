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

void compute(struct buffer *buf)
{

	while((nfile!=0)&&(buf->empty != buf->n)){
		struct fractal *fract = buf_remove(buf);
		int i,j;
		fract->average = 0;
		for(i = 0; i<fract->height; i++){
		 	for(j = 0; j<fract->width; j++){
				
				fract->average += fractal_compute_value(fract, i, j);
			}
		}
		fract->average = fract->average/(fract->height*fract->width);
		
	}
	pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
	int i;
	pthread_t *compThreads = (pthread_t *)malloc(nthreads*sizeof(pthread_t)); 
	if(compThreads == NULL){
		return -1;
	}
	
	int t;
	for( i = 0; i< nthreads; i++){
	t = pthread_create(compThreads +i, NULL, compute, buffer);
		if(t !=0){
			printf("ERROR; return code from pthread_create() is %d\n", t);
			return -1;
		}
	}
	
	free(compThreads);
	return 0;
}
