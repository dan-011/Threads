#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "unixtimer.h"

#define DEFAULT_NUM_THREADS   2
#define DEFAULT_LEN 10000000
#define SEED 3100
#define MODULUS  7

typedef struct {
  long  *a;
  long  *b;
  long  len;
  long  dotprod;
} dot_struct;

dot_struct  dot_data;
pthread_mutex_t dotprod_mutex;

long num_threads = DEFAULT_NUM_THREADS;

void* dotprod(void* arg)
{
  long id = (long)arg;
  long start = id*(dot_data.len/num_threads);
  long end   = (id==num_threads ? dot_data.len : start + dot_data.len/num_threads);

  for(long i=start; i<end; i++) {
    pthread_mutex_lock( &dotprod_mutex );
    dot_data.dotprod += (dot_data.a[i] * dot_data.b[i]);
    pthread_mutex_unlock( &dotprod_mutex );
  }

  pthread_exit(NULL);
}

int main(int argc, char** argv)
{
  long len = DEFAULT_LEN;
  if(argc > 1)  num_threads = atoi(argv[1]);
  if(argc > 2)  len = atoi(argv[2]);

  // initialize vectors
  long* a = malloc( len*sizeof(long) );
  long* b = malloc( len*sizeof(long) );
  srand(SEED);
  for(long i=0; i<len; i++) {
    b[i] = a[i] = rand() % MODULUS;
  }

  pthread_t thread[num_threads];

  start_timer();
  start_clock();

  // initialize global struct and mutex
  dot_data.a   = a;
  dot_data.b   = b;
  dot_data.len = len;
  dot_data.dotprod = 0;
  pthread_mutex_init(&dotprod_mutex, NULL);

  // create threads and pass them the thread index
  for(long i=0; i<num_threads; i++) {
    pthread_create(&thread[i], NULL, dotprod, (void *)i);
  }

  // join threads
  for(long i=0; i<num_threads; i++) {
    pthread_join(thread[i], NULL);
  }

  // print dot product
  printf("Dot product =  %ld \n", dot_data.dotprod);
  printf("Time: %lf wall clock sec, %lf CPU sec\n", clock_seconds(), cpu_seconds() );

  // cleanup
  free(a);
  free(b);
  pthread_mutex_destroy(&dotprod_mutex);

  pthread_exit(NULL);
}

