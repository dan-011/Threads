#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include "unixtimer.h"

#define DEFAULT_N  1000000
#define DEFAULT_NTHREADS 2
#define EVEN(n)((n)%2 == 0 ? 1 : 0)

///////////////////////////////////////////////////////////////////////

// Define any necessary macros, types, and additional functions here
// TODO

typedef struct{
   double sum;
   long start;
   long end;
} thread_info;

double eval_i(long i){
   double num;
   if(i%2==0)
      num = 1.0/(2*i+1);
   else
      num = (-1)*(1.0/(2*i+1));
   return num;
}

void* get_sum(void* arg){
   thread_info* argument = (thread_info*)arg;
   double sum = 0.0;
   long start = argument->start;
   long end = argument->end;
   for(long i = start; i < end; i++){
      //printf("i: %ld eval: %f sum: %f\n", i, eval_i(i), sum);
      sum += eval_i(i);
   }
   argument->sum = sum;
   pthread_exit(NULL);
}



/////////////////////////////////////////////////////////////////
//
// Compute the sum of first n terms of the Madhava-Leibniz series
// using num_threads threads
//
double sum_threads(long n, int num_threads)
{
   pthread_t   tid[num_threads];     // to store thread IDs
   thread_info arg[num_threads];
   long start = 0;
   long end = 0;
   long deduct = n;
   long i_per_thread = n/num_threads;

   for(int thread = 0; thread < num_threads; thread++){

      start = end;
      end = end + i_per_thread;
      deduct -= i_per_thread;
      if(thread == (num_threads - 1)){
         end += deduct;
      }
      //printf("Start: %ld End: %ld\n", start, end);
      arg[thread] = (thread_info){0, start, end};
      pthread_create(&tid[thread], NULL, get_sum, (void *)&(arg[thread]));
   }
   double sum = 0.0;
   for(int i = 0; i < num_threads; i++){
      pthread_join(tid[i], NULL);
      printf("%f\n", arg[i].sum);
      sum += arg[i].sum;
   }
   return sum;
}

/////////////////////////////////////
// STUDY BUT DO NOT MODIFY CODE BELOW

int main(int argc, char *argv[])
{
   long n           = (argc < 2 ? DEFAULT_N : atoi(argv[1]) );
   int  num_threads = (argc < 3 ? DEFAULT_NTHREADS : atoi(argv[2]) );
   double PI25 = 3.141592653589793238462643;

   // Compute and print the approximation of pi
   start_clock();
   start_timer();
   double my_pi = 4 * sum_threads(n, num_threads);
   printf("pi approximation: %.16f Error: %.16f\n", my_pi, fabs(my_pi - PI25));
   printf ("Clock time = %.2f CPU time =  %.2f\n", clock_seconds(), cpu_seconds() );

   return 0;
}
