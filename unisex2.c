#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

#define ITERATIONS 10
#define DEFAULT_NFEMALES 10
#define DEFAULT_NMALES 10
#define DEFAULT_NSTALLS 2

__thread unsigned short buff[3];

#define RANDOM_INIT()  (buff[0]=buff[1]=buff[2]=(unsigned short)pthread_self())

#define WORK() msleep( nrand48(buff) % 50 )

#define USE_BATHROOM() msleep( nrand48(buff) % 10 )

void msleep(int tms)
{
    struct timeval tv;
    tv.tv_sec  = tms / 1000;
    tv.tv_usec = (tms % 1000) * 1000;
    select (0, NULL, NULL, NULL, &tv);
}

typedef struct bathroom {
   int    bfemales;        // number of females in the bathroom
   int    bmales;          // number of males in the bathroom
   int available_stalls;
   int waiting_females;
   pthread_mutex_t mutex;  // mutex to protect shared info
   pthread_cond_t  available;   // condition variable
} bathroom_t;

typedef struct thread_data {
   int id;
   bathroom_t* shared_info;
} thr_data;

void* female(void* thread_arg) {
   thr_data* arg = (thr_data*)thread_arg;
   int i, id = arg->id;
   bathroom_t* shared_info = arg->shared_info;

   RANDOM_INIT();

   for(i=0; i<ITERATIONS; i++){
      WORK();
      pthread_mutex_lock(&shared_info->mutex);
      if(shared_info->bmales > 0 || shared_info->available_stalls == 0){
         shared_info->waiting_females++; // ISSUE: every iteration adds an unwanted wait
      }
      while(shared_info->bmales > 0 || shared_info->available_stalls == 0) {  // must wait
         printf("female #%d waits\n", id);
         pthread_cond_wait(&shared_info->available, &shared_info->mutex);
      }
      shared_info->bfemales++;
      if(shared_info->waiting_females > 0){
         shared_info->waiting_females--;
      }
      shared_info->available_stalls--;
      printf("female #%d enters bathroom\n", id);
      pthread_mutex_unlock(&shared_info->mutex);
      USE_BATHROOM();
      pthread_mutex_lock(&shared_info->mutex);
      shared_info->bfemales--;
      shared_info->available_stalls++;
      pthread_cond_broadcast(&shared_info->available);
      printf("female #%d exits bathroom\n", id);
      pthread_mutex_unlock(&shared_info->mutex);
   }
   return NULL;
}

void* male(void* thread_arg) {
   thr_data* arg = (thr_data*)thread_arg;
   int i, id = arg->id;
   bathroom_t* shared_info = arg->shared_info;

   RANDOM_INIT();

   for(i=0; i<ITERATIONS; i++){
      WORK();
      pthread_mutex_lock(&shared_info->mutex);
      //printf("BEFORE females: %d, stalls: %d, waiting: %d\n", shared_info->bfemales, shared_info->available_stalls, shared_info->waiting_females);
      while(shared_info->bfemales > 0 || shared_info->available_stalls == 0 || shared_info->waiting_females > 0) {  // must wait
         //printf("IN LOOP females: %d, stalls: %d, waiting: %d\n", shared_info->bfemales, shared_info->available_stalls, shared_info->waiting_females);
         printf("male #%d waits\n", id);
         pthread_cond_wait(&shared_info->available, &shared_info->mutex);
      }
      //printf("AFTER females: %d, stalls: %d, waiting: %d\n", shared_info->bfemales, shared_info->available_stalls, shared_info->waiting_females);
      shared_info->bmales++;
      shared_info->available_stalls--;
      printf("male #%d enters bathroom\n", id);
      pthread_mutex_unlock(&shared_info->mutex);
      USE_BATHROOM();
      pthread_mutex_lock(&shared_info->mutex);
      shared_info->bmales--;
      shared_info->available_stalls++;
      pthread_cond_broadcast(&shared_info->available);
      printf("male #%d exits bathroom\n", id);
      pthread_mutex_unlock(&shared_info->mutex);
   }
   return NULL;
}


int main(int argc, char *argv[]) {
   int i, status, n, nfemales = DEFAULT_NFEMALES, nmales = DEFAULT_NMALES, nstalls = DEFAULT_NSTALLS;

   for(i = 1; i < argc; i++) {
      if(strncmp(argv[i], "-f", strlen("-f")) == 0 && i + 1 < argc) {
         nfemales = atoi(argv[++i]);
      }
      else if(strncmp(argv[i], "-m", strlen("-m")) == 0 && i + 1 < argc) {
         nmales = atoi(argv[++i]);
      }
      else if(strncmp(argv[i], "-s", strlen("-s")) == 0 && i + 1 < argc) {
         nstalls = atoi(argv[++i]);
      }
      else {
         fprintf(stderr, "Usage: %s [-f N|-females N] [-m N|-males N] [-s N|-stalls N]\n", argv[0]);
         return 1;
      }
   }

   bathroom_t shared_info;
   shared_info.bfemales = shared_info.bmales = 0;
   shared_info.available_stalls = nstalls;
   shared_info.waiting_females = 0;
   pthread_mutex_init(&shared_info.mutex, NULL);
   pthread_cond_init(&shared_info.available, NULL);

   n = nfemales + nmales;      // number of threads to create
   pthread_t tid[n];
   thr_data  data[n];

   for(i = 0; i < nfemales; i++) {
      data[i] = (thr_data){i,&shared_info};
      status = pthread_create(&tid[i], NULL, female, &data[i]);
      assert(status == 0);
   }
   for(i = nfemales; i < n; i++) {
      data[i] = (thr_data){i-nfemales,&shared_info};
      status = pthread_create(&tid[i], NULL, male, &data[i]);
      assert(status == 0);
   }
   for(i = 0; i < n; i++) {
      pthread_join(tid[i], NULL);
   }

   return 0;
}
