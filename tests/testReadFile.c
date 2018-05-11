#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "CUnit/CUnit.h"
#include "CUnit/Basic.h"

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#define bufSize 1024

bool blankDetected = false;
bool comDetected = false;

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

struct nameList{struct nameList *next; char *name};
struct nameList *fracNames;
sem_t semCompare;

int nthreads_max = 0;
int nthread = 0;

struct fractal {
    const char *name;
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

struct fractal *fractal_new(const char *name, int width, int height, double a, double b)
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

  pthread_mutex_lock(&mutNameList);

  if(fracNames->name == NULL){

    fracNames->name = (char *)malloc(sizeof(char)*65);
    strcpy(fracNames->name, array[0]);
    pthread_mutex_unlock(&mutNameList);
    struct fractal *theFract = fractal_new(name, (int) strtol(array[1], &eptr, 10), (int) strtol(array[2], &eptr, 10), atof(array[3]), atof(array[4]));
    pthread_mutex_lock(&mutCompThreads);
    nProdFract++;
    NCompThreads++;
    pthread_mutex_unlock(&mutCompThreads);
    buf_insert(readFract,theFract);

  }
  else{
    struct nameList *temp = fracNames;
    bool same = false;
    while((temp->next!=NULL)&&(same==false)){
      if(strcmp(temp->name, array[0])==0){
        same = true;
      }
      temp = temp->next;
    }
    if((same == true)||(strcmp(temp->name, array[0])==0)){
      perror("Warning : a fractal already has this name. This one will therefore be ignored\n");
      pthread_mutex_unlock(&mutNameList);
    }
    else{
      temp->next = (struct nameList *)malloc(sizeof(struct nameList));      
      temp->next->name = (char *)malloc(sizeof(char)*65);
      strcpy(temp->next->name, array[0]);
      pthread_mutex_unlock(&mutNameList);
      struct fractal *theFract = fractal_new(name, (int) strtol(array[1], &eptr, 10), (int) strtol(array[2], &eptr, 10), atof(array[3]), atof(array[4]));
      pthread_mutex_lock(&mutCompThreads);
      nProdFract++;
      NCompThreads++;
      pthread_mutex_unlock(&mutCompThreads);
      buf_insert(readFract,theFract);
    }
  }
    
}
 
void *readFile(void *fn){
  printf("Ouverture d'un fichier\n");
  bool fini = false;
  char *filename = (char *) fn;
  char buf[bufSize];
  if(strcmp("-",filename) == 0){
    printf("Veuillez inserer : nom longueur[pixels] largeur[pixels] partie_Reelle partie_Imaginaire \n");
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
        }else{
          if(strcmp(buf,"")==0 || strcmp(buf," ")==0){
            blankDetected = true;
          }
          if(buf[0] == '#'){
            comDetected = true;
          }
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








void test_assert_frac_name(void)
{
  CU_ASSERT_STRING_EQUAL(readFract->tab[0]->name, "fract1");
}

void test_assert_frac_width(void)
{
  CU_ASSERT(readFract->tab[1]->width == 900);
}

void test_assert_frac_height(void)
{
  CU_ASSERT(readFract->tab[2]->height == 500);
}

void test_assert_frac_a(void)
{
  CU_ASSERT(readFract->tab[3]->a == 0.0256);
}

void test_assert_frac_b(void)
{
  CU_ASSERT(readFract->tab[0]->b == 0.8);
}

void test_assert_detect_blank(void)
{
  CU_ASSERT(blankDetected);
}

void test_assert_detect_com(void)
{
  CU_ASSERT(comDetected);
}

void test_assert_maxthreads(void)
{
  CU_ASSERT(nthreads_max == 20);
}

void test_assert_displayAll(void)
{
  CU_ASSERT(display);
}

/*
void test_assert_removeDouble(void)
{
  CU_ASSERT(readFract->tab[3]->width != readFract->tab[4]->width);
}
*/







int launch_testReadFile1(int argc,  char *argv[]){
  int errSem;
  pthread_mutex_init(&mutNameList, NULL);
  pthread_mutex_init(&mutCompThreads,NULL);
  pthread_mutex_init(&mutNFile,NULL);
  pthread_mutex_init(&mutCompute,NULL);
  errSem = sem_init(&semCompare, 0, 0);
  if(errSem != 0){
    perror("sem_init compare");
  }
  int join;
  readFract = (struct buffer *) malloc(sizeof(struct buffer));
  //compareFract = (struct buffer *)malloc(sizeof(struct buffer));
  fracNames = (struct nameList *)malloc(sizeof(struct nameList));
  fracNames->next = NULL;
  fracNames->name = NULL;
  nthreads_max = 10;
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
  //buf_init(compareFract,nthreads_max);

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


  for(i=0;i<NFile;i++){
    join = pthread_join(threadReaders[i],NULL);
  }

}

int main(void){

  int argc = 6;
  char *argv[6] = {"./main","-d","--maxthreads","20","file1.txt","out"};
  launch_testReadFile1(argc,argv);

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

  if ((NULL == CU_add_test(pSuite, "Test assert frac name", test_assert_frac_name)) ||
     (NULL == CU_add_test(pSuite, "Test assert frac width", test_assert_frac_width)) ||
     (NULL == CU_add_test(pSuite, "Test assert frac height", test_assert_frac_width)) ||
     (NULL == CU_add_test(pSuite, "Test assert frac a", test_assert_frac_width)) ||
     (NULL == CU_add_test(pSuite, "Test assert frac b", test_assert_frac_width)) ||
     (NULL == CU_add_test(pSuite, "Test assert frac detect blank", test_assert_frac_width)) ||
     (NULL == CU_add_test(pSuite, "Test assert frac detect com", test_assert_frac_width)) ||
     (NULL == CU_add_test(pSuite, "Test assert frac maxthreads", test_assert_frac_width)) ||
     (NULL == CU_add_test(pSuite, "Test assert frac display all", test_assert_frac_width)))
  {
    CU_cleanup_registry();
    return CU_get_error();
  }

  CU_basic_run_tests();
  CU_basic_show_failures(CU_get_failure_list());
}