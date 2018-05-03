#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "fractal.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#define bufSize 1024


bool display = false;
int nfiles = 0;
struct buffer *readFract;

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

void split(char buf[]){
	int i = 0;
    char *p = strtok (buf, " ");
    //Considérer erreurs
    char *array[5];
    char *eptr;

    while (p != NULL)
    {
        array[i++] = p;
        p = strtok (NULL, " ");
    }
    char *name = (char *) malloc(65*sizeof(char));
    name[64] = '\0';
    strcpy(name,array[0]);
    struct fractal *theFract = fractal_new(name, (int) strtol(array[1], &eptr, 10), (int) strtol(array[2], &eptr, 10), atof(array[3]), atof(array[4]));
    buf_insert(readFract,theFract);
    
}
 
int readFile(char *filename){
	FILE* fp;
	char buf[bufSize];
	if ((fp = fopen(filename, "r")) == NULL){
		perror("fopen source-file");
		return 1;
	}
	while (fgets(buf, sizeof(buf), fp) != NULL){
		buf[strlen(buf) - 1] = '\0';
		split(buf);
	}
	fclose(fp);
	return 0;
}

int main(int argc, char* argv[])
{
	readFract = (struct buffer *) malloc(sizeof(struct buffer));
	int nthreads_max = 10;
	int indexfile;


	if(strcmp(argv[1],"-d") == 0){
		display = true;
		if(strcmp(argv[2], "--maxthreads") == 0){
			nthreads_max = *argv[3];
			indexfile = 4;
		}else{
			indexfile = 2;
		}
	}else{
		if(strcmp(argv[1], "--maxthreads") == 0){
			nthreads_max = *argv[2];
			indexfile = 3;
		}else{
			indexfile = 1;
		}
	}
	buf_init(readFract,nthreads_max);

	nfiles = argc - indexfile - 1;

	pthread_t *threadReaders = (pthread_t *) malloc(nfiles*sizeof(pthread_t));
	char *argsThreadReaders[nfiles];
	int err;
	long i;
	for(i = 0; i<nfiles; i++){
		argsThreadReaders[i] = argv[indexfile];
		err=pthread_create(&(threadReaders[i]),NULL,&readFile,(void *) &(argsThreadReaders[i]));
		if(err!=0){
			error(err,"pthread_create");
		}
		
		indexfile++;
	}
	
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
