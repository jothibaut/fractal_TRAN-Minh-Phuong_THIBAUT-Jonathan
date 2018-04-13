#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "fractal.h"
#include <pthread.h>
#include <semaphore.h>

void buf_init(struct buffer *buf,int n){
	buf->tab = (sruct fractal *) malloc(n*sizeof(struct fractal));
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

void buf_remove(struct buffer *buf){

	sem_wait(&(buf->full));
	pthread_mutex_lock(&(buf->mutex));
	buf->front++;
	pthread_mutex_unlock(&(buf->mutex));
	sem_post(&(buf->empty));
}

int main(int argc, char *argv[])
{
    return 0;
}
