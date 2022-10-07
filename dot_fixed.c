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
  long*  dotprod;
  pthread_mutex_t* mutex;
  long num_threads;
  long i;
} dot_struct;

void* dotprod(void* arg)
{
  dot_struct* thread = (dot_struct*)arg;
  long len = thread->len;
  long* a = thread->a;
  long* b = thread->b;
  long num_threads = thread->num_threads;
  pthread_mutex_t* mutex = thread->mutex;
  long id = thread->i;
  long start = id*(len/num_threads);
  long end   = (id==num_threads ? len : start + len/num_threads);
  long local_dotprod = 0;
  for(long i=start; i<end; i++) {
    local_dotprod += a[i] * b[i];
  }
  pthread_mutex_lock( mutex );
  *thread->dotprod += local_dotprod;
  pthread_mutex_unlock( mutex );
  pthread_exit(NULL);
}

int main(int argc, char** argv)
{
  long len = DEFAULT_LEN;
  long num_threads = DEFAULT_NUM_THREADS;
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
  pthread_mutex_t dotprod_mutex;
  dot_struct arg[num_threads];


  start_timer();
  start_clock();

  // initialize global struct and mutex
  long _dotprod = 0;
  pthread_mutex_init(&dotprod_mutex, NULL);

  // create threads and pass them the thread index
  for(long i=0; i<num_threads; i++) {
    arg[i] = (dot_struct){a, b, len, &_dotprod, &dotprod_mutex, num_threads, i};
    pthread_create(&thread[i], NULL, dotprod, (void *)&arg[i]);
  }

  // join threads
  for(long i=0; i<num_threads; i++) {
    pthread_join(thread[i], NULL);
  }

  // print dot product
  printf("Dot product =  %ld \n", _dotprod);
  printf("Time: %lf wall clock sec, %lf CPU sec\n", clock_seconds(), cpu_seconds() );

  // cleanup
  free(a);
  free(b);
  pthread_mutex_destroy(&dotprod_mutex);

  pthread_exit(NULL);
}

