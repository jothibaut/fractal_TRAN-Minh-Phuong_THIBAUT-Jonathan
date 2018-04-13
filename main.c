#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "fractal.h"

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
}

void buf_remove(struct buffer *buf){
	
}

int main()
{
    return 0;
}
