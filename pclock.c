#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include "guess-mt.h"

typedef struct thread_arg_tag {
    gmn_t* sb;
    /* TODO 
     * Add mutexes and condition variables required
    */
    pthread_mutex_t mutex;
    pthread_cond_t can_guess;
    pthread_cond_t can_check;
} thread_arg_t;

typedef struct{
    thread_arg_t* thread_data;
    int min_value;
    int max_value;
}producer_data;

typedef struct{
    thread_arg_t* thread_data;
}consumer_data;

void *thread_c(void* consumer_thread_data);
void *thread_p(void* producer_thread_data);

int main(int argc,char* argv[])
{
    
    /* TODO
     * Create an instance of thread_arg_t and initialize it. The value to be guessed is taken as input from the command line arguments.
     * Create two threads - one each for the parent and child
     * Reap the thread resources and destroy the mutexes and condition variables used.
    */
    pthread_mutex_t mutex;
    pthread_cond_t can_guess;
    pthread_cond_t can_check;
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&can_guess, NULL);
    pthread_cond_init(&can_check, NULL);
    gmn_t* sb = gmn_init(atoi(argv[1]));
    thread_arg_t thread_data = (thread_arg_t){sb, mutex, can_guess, can_check};
    pthread_t pid;
    pthread_t cid;
    producer_data p = (producer_data){&thread_data, 0, MAX_VALUE};
    consumer_data c = (consumer_data){&thread_data};
    printf("%s", sb->message);
    pthread_create(&pid, NULL, thread_p, (void*)&p);
    pthread_create(&cid, NULL, thread_c, (void*)&c);
    pthread_join(pid, NULL);
    pthread_join(cid, NULL);

    free(sb);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&can_guess);
    pthread_cond_destroy(&can_check);
    return 0;
}

void* thread_p(void *producer_thread_data) {
   /* TODO
    * This is a parent thread.
    * repeat the following until guess is correct
        guess a number between min and max. initially max is set to MAX_VALUE
        send the guess to the child and wait for a result (thread_c)
        if result is 0 i.e. you guessed the number correctly, end the thread
        if result is -1 or 1, update the search interval
   */
    producer_data* p = (producer_data*)producer_thread_data;
    thread_arg_t* thread_data = (thread_arg_t*)p->thread_data;
    gmn_t* sb = thread_data->sb;
    while(sb->result){
        pthread_mutex_lock(&thread_data->mutex);
        while(sb->status){ // while a guess is being processed
            pthread_cond_wait(&thread_data->can_guess, &thread_data->mutex);
        }
        if(sb->result == 0){
            break;
        }
        else if(sb->result == -1){
            p->min_value = sb->guess;
        }
        else if(sb->result == 1){
            p->max_value = sb->guess;
        }
        //printf("guess between %d and %d\n", p->min_value, p->max_value);
        if(sb->guess == MAX_VALUE - 1 && sb->result == -1){
            sb->guess = MAX_VALUE;
        }
        else{
            sb->guess = (p->max_value + p->min_value)/2;
        }
        sb->status = 1;
        printf("My guess is %d.\n", sb->guess);
        pthread_cond_signal(&thread_data->can_check);
        pthread_mutex_unlock(&thread_data->mutex);
    }
    pthread_exit(NULL);
    //return NULL;
}

void* thread_c(void * consumer_thread_data)
{
    /* TODO
     * This is a child thread.
     * repeat the following until guess is correct 
     *      wait for a guess from the parent (thread_p) 
     *      call gmn_check() to compare the guess with the user input value
     *      send the result to thread_p
     */
    producer_data* c = (producer_data*)consumer_thread_data;
    thread_arg_t* thread_data = (thread_arg_t*)c->thread_data;
    gmn_t* sb = thread_data->sb;
    while(sb->result){
        pthread_mutex_lock(&thread_data->mutex);
        while(!sb->status){
            pthread_cond_wait(&thread_data->can_check, &thread_data->mutex);
        }
        gmn_check(sb);
        printf("%s", sb->message);
        sb->status = 0;
        if(sb->result == 0){
            pthread_cond_signal(&thread_data->can_guess);
            pthread_mutex_unlock(&thread_data->mutex);
            break;
        }
        pthread_cond_signal(&thread_data->can_guess);
        pthread_mutex_unlock(&thread_data->mutex);
    }
    pthread_exit(NULL);
    //return NULL;
}
