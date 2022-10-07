#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#define     DEFAULT_DARTS_PER_THREAD 500000
#define     DEFAULT_NUM_THREADS           2

/* Using TLS for the random number generator buffer to avoid data races between threads
   */
__thread unsigned short buff[3];

/*
   Call the following macro in your thread function before any calls to RANDOM_DOUBLE()
   By seeding the random number generator based on the thread ID each thread gets a different
   seed, and the seeds are different in different program runs.
   */
#define RANDOM_INIT()  (buff[0]=buff[1]=buff[2]=(unsigned short)pthread_self())

/*
   RANDOM_DOUBLE() returns a random number uniformly distributed between $[0, 1)$
   */
#define RANDOM_DOUBLE() (erand48(buff))


///////////////////////////////////////////////////////////////////////

// Define any necessary macros, types, and additional functions here
// TODO

typedef struct
{
   //double* xs;  // data array
   //double* ys;
   long p;
   long start;
   long end;
} thread_info;

int test_point(double x, double y){
    return ((x*x) + (y*y)) < 1;
}

void* generate_points(void* arg){
    thread_info* thread = (thread_info*)arg;
    //long* xs = thread->xs;
    //long* ys = thread->ys;
    long start = thread->start;
    long end = thread->end;
    long p = 0;
    RANDOM_INIT();
    for(long i = start; i < end; i++){
        double x = RANDOM_DOUBLE();
        double y = RANDOM_DOUBLE();
        //xs[i] = x;
        //ys[i] = y;
        if(test_point(x, y)){
            p++;
        }
    }
    thread->p = p;
    pthread_exit(NULL);
}

int main(int argc, char ** argv)
{
    int     num_threads      = DEFAULT_NUM_THREADS;
    int     darts_per_thread = DEFAULT_DARTS_PER_THREAD;

    if (argc > 2) {
        num_threads = atoi(argv[1]);
        darts_per_thread = atoi(argv[2]);
        assert( num_threads>0 && darts_per_thread>0 );
    }
    else {
        fprintf(stderr, "Usage: %s <num_threads> <darts_per_thread>\n", argv[0]);
        fprintf(stderr, "Currently using the default values\n");
    }

    long long n = (long long)num_threads * darts_per_thread;

    printf("Throwing %lld darts using %d thread%s\n", n, num_threads, (num_threads>1 ? "s" : "") );

    // Add code to create num_threads threads, each simulating the twrowing of
    // darts_per_thread darts at the [0, 1) × [0, 1) square.  Compute in p the
    // number of darts that fall within the circle of radius 1 centered at (0,0)

    // TODO
    pthread_t   tid[num_threads];     // to store thread IDs
    thread_info arg[num_threads];

    //double* xs = (double*)calloc(total, sizeof(double));
    //double* ys = (double*)calloc(total, sizeof(double));
    long start = 0;
    long end = 0;
    for(int thread_i = 0; thread_i < num_threads; thread_i++){
        start = end;
        end = end + darts_per_thread;
        arg[thread_i] = (thread_info){0, start, end};
        pthread_create(&tid[thread_i], NULL, generate_points, (void *)&(arg[thread_i]));
    }

    long p = 0;
    for(int i = 0; i < num_threads; i++){
        pthread_join(tid[i], NULL);
        p += arg[i].p;
    }

    // DO NOT MODIFY THE CODE BELOW
    // estimate pi and compare with M_PI, the value defined in math.h
    double my_pi = (double)p / n * 4.0;
    printf("Pi approximation: %.16f Error: %.16f\n", my_pi, fabs(my_pi - M_PI));
    return 0;
}
