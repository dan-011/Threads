#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct{
    long n;
    pthread_cond_t* cond;
    pthread_mutex_t* mutex;
    char* can_carpe;
}thread_info;

// the thread that prints "Carpe "
void* print_Carpe(void* arg)
{
    thread_info* thread = (thread_info*)arg;
    long n = thread->n;
    pthread_cond_t* cond = thread->cond;
    pthread_mutex_t* mutex = thread->mutex;
    for (int k = 0; k < n; k++) {
        pthread_mutex_lock(mutex);
        while(!*thread->can_carpe){
            pthread_cond_wait(cond, mutex);
        }
        printf("Carpe ");
        *thread->can_carpe = 0;
        pthread_cond_signal(cond);
        pthread_mutex_unlock(mutex);
    }
    pthread_exit(NULL);
}

// the thread that prints "Diem\n"
void* print_Diem(void* arg)
{
    thread_info* thread = (thread_info*)arg;
    long n = thread->n;
    pthread_cond_t* cond = thread->cond;
    pthread_mutex_t* mutex = thread->mutex;
    for (int k = 0; k < n; k++) {
        pthread_mutex_lock(mutex);
        while(*thread->can_carpe){
            pthread_cond_wait(cond, mutex);
        }
        printf("Diem\n");
        *thread->can_carpe = 1;
        pthread_cond_signal(cond);
        pthread_mutex_unlock(mutex);
    }
    pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
    int rc1, rc2;
    pthread_t tid1, tid2;

    if(argc < 2) {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        exit(1);
    }
    long n = atol(argv[1]);

    char can_carpe = 1;
    pthread_cond_t cond;
    pthread_cond_init(&cond, NULL);
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    thread_info info = (thread_info){n, &cond, &mutex, &can_carpe};

    rc1 = pthread_create(&tid1, NULL, print_Carpe, (void*)&info);
    rc2 = pthread_create(&tid2, NULL, print_Diem, (void*)&info);
    if(rc1 || rc2) {
        fprintf(stderr, "ERROR: could not create threads\n");
        exit(-1);
    }

    rc1 = pthread_join(tid1, NULL);
    rc2 = pthread_join(tid2, NULL);
    if(rc1 || rc2) {
        fprintf(stderr, "ERROR: could not join threads\n");
        exit(-1);
    }
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);

    return 0;
}