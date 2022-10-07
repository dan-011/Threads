#include <stdio.h>
#include <stdlib.h>
#include "unixtimer.h"

#define DEFAULT_LEN   1000000
#define DEFAULT_MODULUS     7
#define DEFAULT_NUM_THREADS 2
#define DEFAULT_SEED     3100

long dotprod_serial(long* a, long* b, long len)
{
   double dotprod = 0;
   for(long i=0; i<len; i++)
      dotprod += a[i]*b[i];

   return dotprod;
}

int main(int argc, char** argv)
{
   long len        = (argc < 2 ? DEFAULT_LEN : atol(argv[1]) );
   long modulus    = (argc < 3 ? DEFAULT_MODULUS : atol(argv[2]) );
   int num_threads = ( argc < 4 ? DEFAULT_NUM_THREADS : atoi(argv[3]) );
   unsigned int seed = ( argc < 5 ? DEFAULT_SEED : atoi(argv[4]) );

   // defined in dotprod.c
   long dotprod_threads(long* a, long* b, long len, int num_threads);

   // allocate and initialize vectors
   long* a = malloc( len*sizeof(long) );
   long* b = malloc( len*sizeof(long) );

   srand(seed);
   for(long i=0; i<len; i++)
      a[i] = rand() % modulus;
   for(long i=0; i<len; i++)
      b[i] = rand() % modulus;

   // Compute dot product serially
   start_clock();
   start_timer();
   printf ("Serial dot product =  %ld\n", dotprod_serial(a, b, len) );
   printf ("Serial clock time = %.2f CPU time =  %.2f\n", clock_seconds(), cpu_seconds() );

   // Compute dot product using threads
   start_clock();
   start_timer();
   printf ("Threads dot product =  %ld\n", dotprod_threads(a, b, len, num_threads) );
   printf ("Threads clock time = %.2f CPU time =  %.2f\n", clock_seconds(), cpu_seconds() );

   // cleanup
   free(a);
   free(b);

   return 0;
}

