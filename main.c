/*
* Auteurs : TRAN Minh-Phuoong & THIBAUT Jonathan
* Desciption : Programme multithread generant, a partir de donees stockees dans un fichier ou sur l'entree
* standard, des fichiers bitmaps representant des fractales
* Il lit des donnees de fractales dans un fichier texte ou sur l'entree standard afin de stocker
* des structures relatives aux fractales ddans un premier buffer.
* Il calcule les valeurs de chaque pixel sur base de donnes contenue dans le premier buffer et stocke la structure fractale
* dans un second buffer
* Il produit alors un ou plusieurs fichier bitmap contenant une fractale selon les arguments specifies au lancement du programme
* Arguments specifies au lancement du programme :
* ./main [-d] [--maxthreads X] file1 file2 file3 fileOut
* -d : Imprimer toutes les fractales sauf celles qui ont le meme nom. Chaque fichier portera le nom de la fractale.
* --maxthreads X : Borne le nombre de threads calculant la valeur des pixels a X.
* fileX : Fichier contenant des fractales devant etre lu.
* fileOut : Nom du fichier bitmap affichant la fractale.
* Par defaut, sans les arguments optionnels, c'est la fractale dont la valeur moyenne des pixels est la plus grande qui est affichee.
*/

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

pthread_mutex_t mutNCompThreadDone;
pthread_mutex_t mutcount;
pthread_mutex_t mutNFile;
pthread_mutex_t mutCompute;
pthread_mutex_t mutCompThreads;
pthread_mutex_t mutNameList;

struct nameList{char *name; struct nameList *next};

struct nameList *names;
sem_t semCompare;

int nthreads_max = 0;
int nthread = 0;
/*
* Initialise le buffer 
*
*/
void buf_init(struct buffer *buf,int n){
	buf->tab = (struct fractal **) malloc(n*sizeof(struct fractal *));
	int i;
	buf->n = n;
	buf->front = buf->rear = 0;
	pthread_mutex_init(&(buf->mutex),NULL);
	sem_init(&(buf->empty),0,n);
	sem_init(&(buf->full),0,0);
}

/*
* Libère la mémoire occupée par le buffer
*
*/
void buf_free(struct buffer *buf){
	int i;
	free(buf->tab);
	sem_destroy(&(buf->full));
	sem_destroy(&(buf->empty));
	pthread_mutex_destroy(&(buf->mutex));
	free(buf);
}

/*
* Insère un élément dans le buffer. Le buffer est de type FIFO.
* Grâce à l'expression "%", une fois que le buffer est entièrement rempli, les fractales sont a nouveau stockees
* a partir du debut.
*/
void buf_insert(struct buffer *buf,struct fractal *fract){
	sem_wait(&buf->empty);
	pthread_mutex_lock(&buf->mutex);
	buf->tab[buf->rear%buf->n] = fract;
	
	buf->rear++;
	pthread_mutex_unlock(&buf->mutex);
	sem_post(&buf->full);
}

/*
* La suppression de fractale se fait en deplacant la tete du buffer vers la queue.
* Les fractales inserees ecrasent donc le contenu des fractales "supprimees".
*/
struct fractal *buf_remove(struct buffer *buf){

	sem_wait(&(buf->full));
	pthread_mutex_lock(&(buf->mutex));
	
	struct fractal *res = buf->tab[buf->front%(buf->n)];
	buf->front++;
	
	pthread_mutex_unlock(&(buf->mutex));

	sem_post(&(buf->empty));

	return res;
}



/*
* Elimine une fractale du premier buffer, calcule la valeur de chaque pixel
* et la stocke dans le second buffer
*/
void *compute(void* argument)
{

	struct fractal *fract;
	struct buffer *buf1 = readFract;
	struct buffer *buf2 = compareFract;

	int empty;
	while(allFracComputed==false){
		pthread_mutex_lock(&mutCompute);
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
						
			pthread_mutex_lock(&mutNCompThreadDone);
			nCompThreadDone++;
			if(nCompThreadDone == nthreads_max){
				buf_insert(buf2,NULL);
			}
			pthread_mutex_unlock(&mutNCompThreadDone);
			
			pthread_exit(NULL);
		}else{
			pthread_mutex_unlock(&mutCompute);
			
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
		sem_post(&semCompare);
		sem_getvalue(&(buf1->empty), &empty);
		
		pthread_mutex_lock(&mutCompThreads);
		nConsFract++;
		NCompThreads--;
		pthread_mutex_unlock(&mutCompThreads);
		
	}

	pthread_mutex_lock(&mutNCompThreadDone);
	nCompThreadDone++;
	if(nCompThreadDone == nthreads_max){
		buf_insert(buf2,NULL);
	}
	pthread_mutex_unlock(&mutNCompThreadDone);
	
	pthread_exit(NULL);
}

/*
* Si -d en arguments, affiche toutes les fractales portant des noms differents
* Si -d pas spécifie, compare les valeurs moyennes de toutes les fractales et affiche celle qui a la valeur
* moyenne la plus elevee.
*/ 
void *compare(void *bufargs){

	struct buffer *buf = compareFract;
	
	int empty;
	sem_wait(&semCompare);
	sem_getvalue(&(buf->empty), &empty);
	int nCompThreads;
	if(!display){
		struct result *res = (struct result *) malloc(sizeof(struct result));
		res->next = NULL;
		res->frac = buf_remove(buf);
		sem_getvalue(&(buf->empty), &empty);
		struct result *temp = res;
		struct result *temp2 = res;
		struct result *temp3 = res;
		double max = res->frac->average;
		int nCompThreads;
		int countMax = 1;
		pthread_mutex_lock(&mutCompThreads);
		nCompThreads = NCompThreads;
		pthread_mutex_unlock(&mutCompThreads);
		
		while ((!isNotDispDone)||(nCompThreads>0) || (empty != buf->n)){ //&& --> || car si on met && il se peut que nCompthreads = 0 car le buf est partiellement rempli mais qu'il ne soit pas encore vide

			
			
			struct fractal *frac = buf_remove(buf);
			if(frac!=NULL){
				
				
				if(frac->average > max){ //Si on trouve un nouveau max, on vide la lisste chaînée créée au préalable
					
					
					temp = res;
					temp2 = temp;
					while(temp!=NULL){
						fractal_free(temp->frac);
						
						temp = temp->next;
						if((temp2!=NULL)&&(temp2!=res)){
							free(temp2);
						}
						temp2 = temp;
					}
					
					max = frac->average;
					res->frac = frac;
					
					temp = res;
					countMax = 1;
				}
				else if(frac->average == max){
					
					struct result *a = (struct result *) malloc(sizeof(struct result));
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
				sem_getvalue(&(buf->empty), &empty);
				
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
		
		
		temp = res;
		temp2 = temp;
		

		if(countMax == 1){
			
			t = write_bitmap_sdl(res->frac, OutFile);	
			if(t!=0){
			printf("ERROR write_bitmap_sdl returned %d", t);
			}	
			fractal_free(res->frac);
		}
		else {
			temp = res;
			temp2 = temp;
			while (temp!= NULL){
				t = write_bitmap_sdl(temp->frac, temp->frac->name);	
				if(t!=0){
				printf("ERROR write_bitmap_sdl returned %d", t);
				}
				fractal_free(temp->frac);
				temp = temp->next;

				if((temp2!=NULL)&&(temp2!=res)){

					free(temp2);

				}

				temp2 = temp;

			}
		}
				
		free(res);

	}
	else{//(!display)
		
		pthread_mutex_lock(&mutCompThreads);
		nCompThreads = NCompThreads;
		pthread_mutex_unlock(&mutCompThreads);
		int t;
		
		while ((!isDisplayDone)||((nCompThreads>0) || (empty != buf->n))){
			
			struct fractal *frac = buf_remove(buf);
			if(frac==NULL){
				isDisplayDone = true;
				pthread_exit(NULL);
			}
			
			t = write_bitmap_sdl(frac, frac->name);
			fractal_free(frac);
			if(t!=0){
				printf("ERROR write_bitmap_sdl returned %d\n", t);
			}
			
			pthread_mutex_lock(&mutCompThreads);
			nCompThreads = NCompThreads;
			pthread_mutex_unlock(&mutCompThreads);
			sem_getvalue(&(buf->empty), &empty);
			
		}
		
	}
	pthread_exit(NULL);
}
	
/*
* Separe une chaine de caractere en plusieurs mots separes par des espaces
*
*/ 
void split(char buf[]){

	int i = 0;
	char *p = strtok (buf, " ");
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
	char *name1 = (char *) malloc(65*sizeof(char));
	name1[64] = '\0';
	strcpy(name1,array[0]);

	pthread_mutex_lock(&mutNameList);
	bool same = false;
	struct nameList *temp = names;
	if(temp->name == NULL){
		temp->name = name;
		pthread_mutex_unlock(&mutNameList);
		struct fractal *theFract = fractal_new(name1, (int) strtol(array[1], &eptr, 10), (int) strtol(array[2], &eptr, 10), atof(array[3]), atof(array[4]));

		pthread_mutex_lock(&mutCompThreads);
		nProdFract++;
		NCompThreads++;
		pthread_mutex_unlock(&mutCompThreads);
		buf_insert(readFract,theFract);
		

	}
	else{
		while((temp->next!=NULL)&&(same == false)){

			if(strcmp(temp->name, name)==0){
					
				same=true;

			}

			temp = temp->next;
		}

		if(same ==true){

		pthread_mutex_unlock(&mutNameList);
		perror("!!!Warning a fractal of the same name already exists ! This one will therefore be ignored.");

		}

		else if((strcmp(temp->name, name)==0)){
			perror("!!!Warning a fractal of the same name already exists ! This one will therefore be ignored.");
			pthread_mutex_unlock(&mutNameList);


		}
		else{					

			struct nameList *new = (struct nameList *)malloc(sizeof(struct nameList));
			new->next = NULL;
			new->name = name;
			temp->next = new;
			pthread_mutex_unlock(&mutNameList);

			struct fractal *theFract = fractal_new(name1, (int) strtol(array[1], &eptr, 10), (int) strtol(array[2], &eptr, 10), atof(array[3]), atof(array[4]));

			pthread_mutex_lock(&mutCompThreads);
			nProdFract++;
			NCompThreads++;
			pthread_mutex_unlock(&mutCompThreads);
			buf_insert(readFract,theFract);

		}

	}
    
}
 
 /*
 * Lit le ficher dont le nom est entre en argument
 *
 */
void *readFile(void *fn){
	//Ouverture d'un fichier
	bool fini = false;
	char *filename = (char *) fn;
	char buf[bufSize];
	if(strcmp("-",filename) == 0){
		printf("Please insert : fractal_name length[pixels] width[pixels] real_part imaginary_part \n NB : to start the computation of this fractal break with ^D\n");
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
				
			}			
			
		}
		fclose(fp);
	}
	pthread_mutex_lock(&mutNFile);
	
	nFileRemaining++;
	if(nFileRemaining == NFile){
		struct fractal *theFract = NULL;
		buf_insert(readFract,theFract);
	}
	pthread_mutex_unlock(&mutNFile);

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
	compareFract = (struct buffer *)malloc(sizeof(struct buffer));
	names = (struct nameList *)malloc(sizeof(struct nameList));
	names->next = NULL;
	nthreads_max = 3;
	int indexfile;
	char cond1[3];
	
	if(strcmp(argv[1], "-d")==0){
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
	buf_init(readFract,nthreads_max); 
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
		argsThreadReaders[i] = argv[indexfile];
		
		err=pthread_create(&(threadReaders[i]),NULL,&readFile,(void *) argsThreadReaders[i]);
		if(err!=0){
			perror("pthread_create");
		}
		indexfile++;
	}

	
	pthread_t *compThreads = (pthread_t *)malloc(nthreads_max*sizeof(pthread_t)); 
	if(compThreads == NULL){
		return -1;
	}
	
	int t;
	
	for( i = 0; i< nthreads_max; i++){

		t = pthread_create(&(compThreads[i]), NULL, &compute, NULL); 
		if(t !=0){
			printf("ERROR; return code from pthread_create() is %d\n", t);
			return -1;
		}
	}
	
	
	pthread_t finalThread;
	t = pthread_create(&finalThread, NULL, &compare, NULL);//(void *)compareFract
	if(t !=0){
		printf("ERROR; return code from pthread_create() for compare() is %d\n", t);
		return -1;
	}

	
	
	for(i=0;i<NFile;i++){
		join = pthread_join(threadReaders[i],NULL);
	}
	
	
	for(i=0;i<nthreads_max;i++){
		join = pthread_join(compThreads[i],NULL);
	}
	
	
	int v = pthread_join(finalThread, NULL);
	/*
	pthread_mutex_lock(&mutNameList);
	struct nameList *temp = names;
	struct nameList *temp2 = temp;
	while((temp!=NULL)&&(temp2!=NULL)){
		
		temp = temp->next;
		free(temp2);
	}
	pthread_mutex_unlock(&mutNameList);*/

	free(threadReaders);
	free(compThreads);

	

	buf_free(readFract);

	buf_free(compareFract);



	sem_destroy(&semCompare);
	
	if(errSem != 0){
		perror("sem_destroy");
	}

	return 0;
	
}
