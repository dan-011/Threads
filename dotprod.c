#include <pthread.h>

// shared global variables protected by a mutex

long* glob_a;
long* glob_b;
long  glob_len;
long  glob_num_threads;
long  glob_dotprod;
pthread_mutex_t dotprod_mutex;

void* dotprod(void* arg)
{
   long id = (long)arg;
   long dotprod = 0;
   long* a = glob_a;
   long* b = glob_b;
   long len = glob_len;
   long num_threads = glob_num_threads;
   for(long i = id; i < len; i += num_threads) {
      dotprod +=  a[i] * b[i];
   }
   pthread_mutex_lock( &dotprod_mutex );
   glob_dotprod += dotprod;
   pthread_mutex_unlock( &dotprod_mutex );


   pthread_exit(NULL);
}

long dotprod_threads(long* a, long* b, long len, int num_threads)
{
   pthread_t thread[num_threads];

   // initialize global variables and mutex
   glob_a = a;
   glob_b = b;
   glob_len = len;
   glob_num_threads = num_threads;
   glob_dotprod = 0;
   pthread_mutex_init(&dotprod_mutex, NULL);

   // create threads and pass them the thread index
   for(long i=0; i<num_threads; i++) {
      pthread_create(&thread[i], NULL, dotprod, (void *)i);
   }

   // join threads
   for(long i=0; i<num_threads; i++) {
      pthread_join(thread[i], NULL);
   }

   // destroy mutex
   pthread_mutex_destroy(&dotprod_mutex);

   return glob_dotprod;
}
