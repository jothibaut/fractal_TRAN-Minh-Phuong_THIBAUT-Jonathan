#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "fractal.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <string.h>
#define bufSize 1024


bool display = false;

char* OutFile;

int NFile = 0;
int NCompThreads = 0;

struct buffer *readFract;
struct buffer *compareFract;
struct result{struct result *next; struct fractal *frac};
struct arguments{struct buffer *buf1; struct buffer *buf2};

pthread_mutex_t mutcount;
pthread_mutex_t mutNFile;
pthread_mutex_t mutCompThreads;
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
	int i;
	for(i=0;i<buf->n;i++){
		free(buf->tab[i]);
	}
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

void *compute(void* argument)
{
	struct arguments *args = (struct arguments *)argument;
	struct buffer *buf1 = args->buf1;
	struct buffer *buf2 = args->buf2;
	int nfile;
	pthread_mutex_lock(&mutNFile);
	nfile = NFile;
	pthread_mutex_unlock(&mutNFile);
	int empty;
	sem_getvalue(&(buf1->empty), &empty);
	while((nfile>0)&&(empty != buf1->n)){
		struct fractal *fract = buf_remove(buf1);
		int i,j;
		fract->average = 0;
		for(i = 0; i<fract->height; i++){
		 	for(j = 0; j<fract->width; j++){
				
				fract->average += fractal_compute_value(fract, i, j);
			}
		}
		fract->average = fract->average/(fract->height*fract->width);
		buf_insert(buf2, fract);

		pthread_mutex_lock(&mutNFile);
		nfile = NFile;
		pthread_mutex_unlock(&mutNFile);
		
	}
	
	pthread_mutex_lock(&mutCompThreads);
	NCompThreads--;
	pthread_mutex_unlock(&mutCompThreads);
	pthread_exit(NULL);
}

void *compare(void *bufargs){
	struct buffer *buf = (struct buffer *)bufargs;
	int empty;
	sem_getvalue(&(buf->empty), &empty);
	int nCompThreads;

	if(!display){
		
		struct result *res;
		res->frac = buf_remove(buf);
		struct result *temp = res;
		double max = res->frac->average;
		int nCompThreads;
		int countMax = 1;
		pthread_mutex_lock(&mutCompThreads);
		nCompThreads = NCompThreads;
		pthread_mutex_unlock(&mutCompThreads);
		
		while ((nCompThreads>0)&&(empty != buf->n)){
			struct fractal *frac = buf_remove(buf);
			if(frac->average > max){
				temp = res;
				while(temp!=NULL){
				fractal_free(temp->frac);
				temp = temp->next;
				}
				max = frac->average;
				res->frac = frac;
				countMax = 1;
			}
			else if(frac-> average = max){
				struct result *a;
				a->frac = frac;
				a->next = NULL;
				temp->next = a;
				temp = temp->next;
				countMax++;	
			}
			else{
			fractal_free(frac);
			}
		pthread_mutex_lock(&mutCompThreads);
		nCompThreads = NCompThreads;
		pthread_mutex_unlock(&mutCompThreads);
		}
		
		int t;
		if(countMax == 1){
			t = write_bitmap_sdl(res->frac, OutFile);	
			if(t!=0){
			printf("ERROR write_bitmap_sdl returned %d", t);
			}	
			fractal_free(res->frac);
		}
		else {
			while (res != NULL){
				t = write_bitmap_sdl(res->frac, res->frac->name);	
				if(t!=0){
				printf("ERROR write_bitmap_sdl returned %d", t);
				}
				fractal_free(res->frac);
				res = res->next;
			}
		}

	}
	else{//(!display)
		pthread_mutex_lock(&mutCompThreads);
		nCompThreads = NCompThreads;
		pthread_mutex_unlock(&mutCompThreads);
		int t;

		while ((nCompThreads>0)&&(empty != buf->n)){
			struct fractal *frac = buf_remove(buf);
			t = write_bitmap_sdl(frac, frac->name);	
			if(t!=0){
			printf("ERROR write_bitmap_sdl returned %d", t);
			}
			pthread_mutex_lock(&mutCompThreads);
			nCompThreads = NCompThreads;
			pthread_mutex_unlock(&mutCompThreads);
		}
	
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
 
void *readFile(void *fn){
	char *filename = (char *)fn;
	FILE* fp;
	char buf[bufSize];
	if ((fp = fopen(filename, "r")) == NULL){
		perror("fopen source-file");
	}
	while (fgets(buf, sizeof(buf), fp) != NULL){
		buf[strlen(buf) - 1] = '\0';
		split(buf);
	}
	fclose(fp);
	pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
	readFract = (struct buffer *) malloc(sizeof(struct buffer));
	compareFract = (struct buffer *)malloc(sizeof(struct buffer));
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
	buf_init(compareFract,nthreads_max);

	NFile = argc - indexfile - 1;
	OutFile = argv[argc-1]; 
	pthread_t *threadReaders = (pthread_t *) malloc(NFile*sizeof(pthread_t));
	char *argsThreadReaders[NFile];
	int err;
	long i;
	for(i = 0; i<NFile; i++){
		argsThreadReaders[i] = argv[indexfile];
		err=pthread_create(&(threadReaders[i]),NULL,&readFile,(void *) &(argsThreadReaders[i]));
		if(err!=0){
			perror("pthread_create");
		}
		
		indexfile++;
	}
	
	pthread_t *compThreads = (pthread_t *)malloc(nthread*sizeof(pthread_t)); 
	if(compThreads == NULL){
		return -1;
	}
	
	int t;
	struct arguments* args;
	args->buf1 = readFract;
	args->buf2 = compareFract;
	for( i = 0; i< nthread; i++){
	t = pthread_create(compThreads +i, NULL, &compute, (void *)args);
		if(t !=0){
			printf("ERROR; return code from pthread_create() is %d\n", t);
			return -1;
		}
	}
	
	pthread_t finalThread;
	t = pthread_create(&finalThread, NULL, &compare, (void *)compareFract);
	if(t !=0){
		printf("ERROR; return code from pthread_create() for compare() is %d\n", t);
		return -1;
	}
	int v = pthread_join(finalThread, NULL);
	
	free(threadReaders);
	free(compThreads);
	
	buf_free(readFract);
	buf_free(compareFract);
	return 0;
}
