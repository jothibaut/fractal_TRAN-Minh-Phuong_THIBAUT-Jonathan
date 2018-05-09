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
int nFileRemaining = 0;
int waitingFiles = 0;
int nProdFract = 0;
int nConsFract = 0;
int nCompThreadDone = 0;
bool allFracComputed = false;
bool firstFract = true;

bool isDisplayDone = false;
bool isNotDispDone = false;

struct buffer *readFract;
struct buffer *compareFract;
struct result{struct result *next; struct fractal *frac};
struct arguments{struct buffer *buf1; struct buffer *buf2};

pthread_mutex_t mutNCompThreadDone;
pthread_mutex_t mutcount;
pthread_mutex_t mutNFile;
pthread_mutex_t mutCompute;
pthread_mutex_t mutCompThreads;
sem_t semCompare;

int nthreads_max = 0;
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
	if(fract!=NULL){
	printf("++++%s |%p\n",fract->name, buf);
	}
	else{
	printf("++++NULL |%p\n", buf);
	}
	buf->rear++;
	pthread_mutex_unlock(&buf->mutex);
	sem_post(&buf->full);
}

struct fractal *buf_remove(struct buffer *buf){

	sem_wait(&(buf->full));
	pthread_mutex_lock(&(buf->mutex));
	
	struct fractal *res = buf->tab[buf->front%(buf->n)];
	buf->front++;
	if(res!=NULL){
	printf("----%s|%p\n",res->name, buf);
	}
	else{
	printf("----NULL|%p\n", buf);
	}
	pthread_mutex_unlock(&(buf->mutex));

	sem_post(&(buf->empty));

	return res;
}

/*
void *compute(void* argument)
{
	struct arguments *args = (struct arguments *)argument;
	struct buffer *buf1 = args->buf1;isDisplayDone
	struct buffer *buf2 = args->buf2;
	int nfile;
	pthread_mutex_lock(&mutNFile);
	nfile = nFileRemaining; //nfile = NFile; -> nfile = nFileRemaining;
	pthread_mutex_unlock(&mutNFile);
	int empty;
	sem_getvalue(&(buf1->empty), &empty);
	printf("nfile = %d\n", nfile);
	printf("empty = %d\n", empty);
	while((nfile>0) || (empty != buf1->n)){ //while((nfile>0)&&(empty != buf1->n)) --> while((nfile>0) || (empty != buf1->n))
		struct fractal *fract = buf_remove(buf1);
		//printf("%s\n", fract->name);
		int i,j;
		fract->average = 0;
		for(i = 0; i<fract->width; i++){
		 	for(j = 0; j<fract->height; j++){
				
				fract->average += fractal_compute_value(fract, i, j);
			}
		}
		printf("Taupe\n");
		fract->average = fract->average/(fract->height*fract->width);
		buf_insert(buf2, fract);
		pthread_mutex_lock(&mutCompThreads);
		NCompThreads--;
		pthread_mutex_unlock(&mutCompThreads);
		printf("%s\n", buf2->tab[0]->name);
		

		pthread_mutex_lock(&mutNFile);
		nfile = nFileRemaining;
		pthread_mutex_unlock(&mutNFile);
		printf("%d\n", nfile);

		sem_getvalue(&(buf1->empty), &empty); //p-ê dans un sémaphore
	}

	pthread_exit(NULL);
}
*/

void *compute(void* argument)
{
	struct fractal *fract;
	struct arguments *args = (struct arguments *)argument;
	struct buffer *buf1 = args->buf1;
	struct buffer *buf2 = args->buf2;

	int empty;
	while(allFracComputed==false){
		pthread_mutex_lock(&mutCompute);
		//sem_getvalue(&(buf1->empty), &empty);
		if(allFracComputed == true){
			pthread_mutex_unlock(&mutCompute);
			pthread_mutex_lock(&mutNCompThreadDone);
			nCompThreadDone++;
			if(nCompThreadDone == nthreads_max){
				buf_insert(buf2,NULL);
			}
			pthread_mutex_unlock(&mutNCompThreadDone);
			
			pthread_exit(NULL);
		}
		fract = buf_remove(buf1);
		if(fract == NULL && nFileRemaining == NFile){
			allFracComputed = true;
			pthread_mutex_unlock(&mutCompute);
			printf("table\n");
			
			pthread_mutex_lock(&mutNCompThreadDone);
			nCompThreadDone++;
			if(nCompThreadDone == nthreads_max){
				buf_insert(buf2,NULL);
			}
			pthread_mutex_unlock(&mutNCompThreadDone);
			
			pthread_exit(NULL);
		}else{
			printf("chaise\n");
			pthread_mutex_unlock(&mutCompute);
			printf("nFileRemaining = %d\n", nFileRemaining);
		}
		int i,j;
		fract->average = 0;
		for(i = 0; i<fract->width; i++){
		 	for(j = 0; j<fract->height; j++){
				fract->average += fractal_compute_value(fract, i, j);
			}
		}
		//printf("Taupe\n");
		fract->average = fract->average/(fract->height*fract->width);
		buf_insert(buf2, fract);
		sem_post(&semCompare);
		sem_getvalue(&(buf1->empty), &empty);
		printf("emptyseg = %d\n", empty);
		pthread_mutex_lock(&mutCompThreads);
		nConsFract++;
		NCompThreads--;
		pthread_mutex_unlock(&mutCompThreads);
		
		//printf("%s\n", buf2->tab[0]->name);
		printf("walibi\n");
		printf("allFracComputed = %d\n", allFracComputed);
	}

	pthread_mutex_lock(&mutNCompThreadDone);
	nCompThreadDone++;
	if(nCompThreadDone == nthreads_max){
		buf_insert(buf2,NULL);
	}
	pthread_mutex_unlock(&mutNCompThreadDone);
	
	printf("fauteuil\n");
	pthread_exit(NULL);
}

void *compare(void *bufargs){
	printf("compère\n");
	struct buffer *buf = (struct buffer *)bufargs;
	
	//printf("%s\n",compareFract->tab[0]->name);
	
	int empty;
	sem_wait(&semCompare);
	sem_getvalue(&(buf->empty), &empty);
	printf("emptyComp = %d\n",empty );
	int nCompThreads;
	printf("consoeur\n");
	if(!display){
		struct result *res = (struct result *) malloc(sizeof(struct result));
		res->next = NULL;
		//res->frac = (struct fractal *) malloc(sizeof(struct fractal));
		res->frac = buf_remove(buf);
		sem_getvalue(&(buf->empty), &empty);
		struct result *temp = res;
		struct result *temp2 = res;
		double max = res->frac->average;
		int nCompThreads;
		int countMax = 1;
		pthread_mutex_lock(&mutCompThreads);
		nCompThreads = NCompThreads;
		pthread_mutex_unlock(&mutCompThreads);

		//printf("nCompThreads =%d\n",nCompThreads);
		//è!çprintf("empty = %d\n", empty);
		
		while ((!isNotDispDone)||(nCompThreads>0) || (empty != buf->n)){ //&& --> || car si on met && il se peut que nCompthreads = 0 car le buf est partiellement rempli mais qu'il ne soit pas encore vide

			
			printf("dindon\n");
			struct fractal *frac = buf_remove(buf);
			if(frac!=NULL){
				
				printf("pintade\n");
				if(frac->average > max){ //Si on trouve un nouveau max, on vide la lisste chaînée créée au préalable
					
					printf("carotte\n");
					temp = res;
					temp2 = temp;
					while(temp!=NULL){
						fractal_free(temp->frac);
						printf("gingembre\n");
						temp = temp->next;
						if(temp2!=NULL){
							free(temp2);
						}
						temp2 = temp;
					}
					printf("concombre\n");
					max = frac->average;
					res->frac = frac;
					
					temp = res;
					countMax = 1;
				}
				else if(frac->average == max){
					printf("carotte2\n");
					struct result *a = (struct result *) malloc(sizeof(struct result));
					a->frac = frac;
					a->next = NULL;
					temp->next = a;
					temp = temp->next;
					countMax++;
				}
				else{
					printf("carotte3\n");
					fractal_free(frac);
					
				}
				pthread_mutex_lock(&mutCompThreads);
				nCompThreads = NCompThreads;
				pthread_mutex_unlock(&mutCompThreads);
				sem_getvalue(&(buf->empty), &empty);
				printf("cochon\n");
			}

			else{
			isNotDispDone = true;
			pthread_mutex_lock(&mutCompThreads);
			nCompThreads = NCompThreads;
			pthread_mutex_unlock(&mutCompThreads);
			sem_getvalue(&(buf->empty), &empty);
			}
		}
		
		int t;
		if(countMax == 1){
			printf("helico\n");
			t = write_bitmap_sdl(res->frac, OutFile);	
			if(t!=0){
			printf("ERROR write_bitmap_sdl returned %d", t);
			}	
			fractal_free(res->frac);
			free(res);
		}
		else {
			printf("nuage\n");
			temp = res;
			while (res != NULL){
				t = write_bitmap_sdl(res->frac, res->frac->name);	
				if(t!=0){
				printf("ERROR write_bitmap_sdl returned %d", t);
				}
				fractal_free(res->frac);
				res = res->next;
				if(temp!=NULL){
					free(temp);
				}
				
				temp = res;
			}
		}


	}
	else{//(!display)
		printf("venezuela\n");
		pthread_mutex_lock(&mutCompThreads);
		nCompThreads = NCompThreads;
		pthread_mutex_unlock(&mutCompThreads);
		int t;
		//sem_wait(&semCompare);
		//sem_getvalue(&(buf->empty), &empty);
		printf("nCompThreads =%d\n",nCompThreads);
		printf("emptyComp-d = %d\n", empty);
		printf("isDisplayDone = %d\n", isDisplayDone);
		while ((!isDisplayDone)||((nCompThreads>0) || (empty != buf->n))){//(nCompThreads>0) || (empty != buf->n) || !allFracComputed //|| (nProdFract==nConsFract && nFileRemaining!=0) // || ((nFileRemaining != NFile) && !(nProdFract==nConsFract && nFileRemaining!=0))
			printf("grenouille\n");
			struct fractal *frac = buf_remove(buf);
			if(frac==NULL){
				isDisplayDone = true;
				pthread_exit(NULL);
			}
			//printf("%s\n%d\n%d\n%f\n%f\n",frac->name,frac->height,frac->width,frac->a,frac->b);
			t = write_bitmap_sdl(frac, frac->name);
			if(t!=0){
				printf("ERROR write_bitmap_sdl returned %d\n", t);
			}
			printf("!!!!!!!!!!!DISPLAYING FRACT!!!!!!!!!!\n");
			printf("mulot\n");
			pthread_mutex_lock(&mutCompThreads);
			nCompThreads = NCompThreads;
			pthread_mutex_unlock(&mutCompThreads);
			sem_getvalue(&(buf->empty), &empty);
			printf("nCompThreads =%d\n",nCompThreads);
			printf("empty = %d\n", empty);
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
    pthread_mutex_lock(&mutCompThreads);
    nProdFract++;
    NCompThreads++;
    pthread_mutex_unlock(&mutCompThreads);
    buf_insert(readFract,theFract);
    printf("holala\n");
}
 
void *readFile(void *fn){
	/*
	pthread_mutex_lock(&mutNFile);
	nFileRemaining++;
	pthread_mutex_unlock(&mutNFile);*/
	bool fini = false;
	char *filename = (char *) fn;
	char buf[bufSize];
	if(strcmp("-",filename) == 0){
		while( fgets(buf, bufSize , stdin) ) //break with ^D or ^Z 
		{
			buf[strlen(buf) - 1] = '\0';
			split(buf);
		}
	}else{
		FILE* fp;
		if ((fp = fopen(filename, "r")) == NULL){
			perror("fopen source-file");
		}
		while (!fini){
			if(fgets(buf, sizeof(buf), fp) == NULL){
				fini = true;
			}
			else{
				buf[strlen(buf) - 1] = '\0';
				if(buf[0] != '#' && strcmp(buf,"")!=0 && strcmp(buf," ")!=0){
				split(buf);
				}
				printf("banana split\n");
			}			
			
		}
		fclose(fp);
	}
	pthread_mutex_lock(&mutNFile);
	printf("nFileRemain = %d\n", nFileRemaining);
	nFileRemaining++;
	if(nFileRemaining == NFile){
		struct fractal *theFract = NULL;
		buf_insert(readFract,theFract);
	}
	pthread_mutex_unlock(&mutNFile);
	
	//printf("Nombre de fichiers n'ayant pas encore été retranscrits : %d\n", nFileRemaining);
	pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
	int errSem;
	pthread_mutex_init(&mutCompThreads,NULL);
	pthread_mutex_init(&mutNFile,NULL);
	pthread_mutex_init(&mutCompute,NULL);
	errSem = sem_init(&semCompare, 0, 0);
	if(errSem != 0){
		perror("sem_init compare");
	}
	int join;
	readFract = (struct buffer *) malloc(sizeof(struct buffer));
	printf("readfract %p\n", readFract);
	compareFract = (struct buffer *)malloc(sizeof(struct buffer));
	nthreads_max = 3;
	int indexfile;


	if(strcmp(argv[1],"-d") == 0){
		display = true;
		if(strcmp(argv[2], "--maxthreads") == 0){
			nthreads_max = atoi(argv[3]);
			indexfile = 4;
		}else{
			indexfile = 2;
		}
	}else{
		if(strcmp(argv[1], "--maxthreads") == 0){
			nthreads_max = atoi(argv[2]);
			indexfile = 3;
		}else{
			indexfile = 1;
		}
	}
	buf_init(readFract,nthreads_max); //
	buf_init(compareFract,nthreads_max);

	NFile = argc - indexfile - 1;
	nFileRemaining = 0;
	OutFile = argv[argc-1]; 
	pthread_t *threadReaders = (pthread_t *) malloc(NFile*sizeof(pthread_t));
	if(threadReaders == NULL){
		return -1;
	}
	char *argsThreadReaders[NFile];
	int err;
	long i;

	for(i = 0; i<NFile; i++){
		printf("arg = %d\n", indexfile);
		argsThreadReaders[i] = argv[indexfile];
		
		err=pthread_create(&(threadReaders[i]),NULL,&readFile,(void *) argsThreadReaders[i]);
		if(err!=0){
			perror("pthread_create");
		}
		indexfile++;
	}

	
	/*
	//==============Fonction permettant de tester les threads threadReaders.==========================
	for(i=0;i<NFile;i++){
		join = pthread_join(threadReaders[i],NULL);
	}
	printf("%s\n%d\n%d\n%f\n%f\n\n",readFract->tab[0]->name,readFract->tab[0]->height,readFract->tab[0]->width,readFract->tab[0]->a,readFract->tab[0]->b);
	//printf("%s\n%d\n%f\n\n",readFract->tab[1]->name,readFract->tab[1]->height,readFract->tab[1]->a);
	//printf("%s\n%d\n%f\n\n",readFract->tab[2]->name,readFract->tab[2]->height,readFract->tab[2]->a);
	//printf("%s\n%d\n%f\n\n",readFract->tab[3]->name,readFract->tab[3]->height,readFract->tab[3]->a);
	printf("Tous les fichiers ont été retranscrits dans le buffer readFract\n\n");
	//=================================================================================================
	
	*/
	pthread_t *compThreads = (pthread_t *)malloc(nthreads_max*sizeof(pthread_t)); //nthread-->nthreads_max
	if(compThreads == NULL){
		return -1;
	}
	
	int t;
	struct arguments* args = (struct arguments*) malloc(sizeof(struct arguments)); //pas malloc -> malloc
	args->buf1 = (struct buffer*) malloc(sizeof(struct buffer)); //pas malloc -> malloc
	args->buf2 = (struct buffer*) malloc(sizeof(struct buffer)); //pas malloc -> malloc

	args->buf1 = readFract;
	args->buf2 = compareFract;
	for( i = 0; i< nthreads_max; i++){

		t = pthread_create(&(compThreads[i]), NULL, &compute, (void *)args); //compThreads + i --> compThreads[i]
		//printf("bureau\n");
		if(t !=0){
			printf("ERROR; return code from pthread_create() is %d\n", t);
			return -1;
		}
	}
	
	
	/*
	//=================================================================================================
	for(i=0;i<nthreads_max;i++){
		join = pthread_join(compThreads[i],NULL);
	}
	printf("%s\n",compareFract->tab[0]->name);
	printf("%f\n",compareFract->tab[0]->average);
	printf("Les fractales ont été retranscrites dans le buffer compareFract\n\n");
	//=================================================================================================
	*/
	
	
	pthread_t finalThread;
	t = pthread_create(&finalThread, NULL, &compare, (void *)compareFract);
	if(t !=0){
		printf("ERROR; return code from pthread_create() for compare() is %d\n", t);
		return -1;
	}

	
	
	for(i=0;i<NFile;i++){
		join = pthread_join(threadReaders[i],NULL);
	}
	
	
	for(i=0;i<nthread;i++){
		join = pthread_join(compThreads[i],NULL);
	}
	
	
	int v = pthread_join(finalThread, NULL);
	
	free(args->buf1);
	free(args->buf2);
	free(args);
	printf("Licorne\n");
	free(threadReaders);
	free(compThreads);
	printf("Troll\n");
	
	printf("readfract %p\n", readFract);
	//buf_free(readFract);
	printf("Bitch\n");
	//buf_free(compareFract);
	printf("Gobelin\n");

	free(args->buf1);
	free(args->buf2);
	free(args);

	sem_destroy(&semCompare);
	if(errSem != 0){
		perror("sem_destroy");
	}
	printf("Pégase\n");
	return 0;
	
}
