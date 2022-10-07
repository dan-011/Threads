#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

#define DEFAULT_NUM_JOBS      50
#define DEFAULT_NUM_COMPUTERS 2
#define DEFAULT_NUM_PRINTERS  2
#define DEFAULT_QUEUE_SIZE    10

__thread unsigned short buff[3];

#define RANDOM_INIT()  (buff[0]=buff[1]=buff[2]=(unsigned short)pthread_self())

#define WORK() (msleep( nrand48(buff) % 200 ), nrand48(buff) % 100)

#define PRINT_JOB(x) msleep( x )

void msleep(int tms)
{
    struct timeval tv;
    tv.tv_sec  = tms / 1000;
    tv.tv_usec = (tms % 1000) * 1000;
    select (0, NULL, NULL, NULL, &tv);
}

typedef struct {
    unsigned char *jobs;            // buffer that keeps job info
    int     njobs_submitted;        // number of jobs that have been submitted
    int     njobs_started;          // number of jobs that have fetched by printer
    int     njobs_max;              // number of jobs to perform
    int     q_size;                 // size of the queue
    // add additional struct fields as needed
    // TODO
    pthread_mutex_t mutex;
    pthread_cond_t printer_condition;
    pthread_cond_t computer_condition;

} job_queue_t;

// The computers and printers can share the same structure
typedef struct {
    int id;
    int seed;
    job_queue_t * jq;
} thread_data_t;

///////////////////// BEGINNING OF QUEUE /////////////////////
/*
 * Implementation of Q. Not a small, fixed-size buffer,
 * but good enough for this assignment.
 */

// init q
// init fields you have added outside of this function
int q_init(job_queue_t *q, int q_size, int max_jobs)
{
    q->njobs_submitted = 0;
    q->njobs_started = 0;
    q->njobs_max = max_jobs;
    q->q_size = q_size;
    q->jobs = malloc(max_jobs);
    assert(q->jobs != NULL);
    return 0;
}

// check if a computer can submit a job
// Return values:
// 1: yes. a job can be accepted.
// 0: no. the queue is full.
// -1: max number of jobs have been submitted.
int q_can_submit (job_queue_t *q)
{
    if (q->njobs_submitted >= q->njobs_max)
        return -1;
    return (q->njobs_submitted - q->njobs_started) < q->q_size;
}

int q_submit_job (job_queue_t *q, int t, int id)
{
    int r = q_can_submit(q);
    if (r < 0) {
        fprintf(stderr, "Error: computer %d tries to submit a job "
               "after the max number of jobs have been submitted.", id);
        exit(1);
    } else if (r == 0)  {
        fprintf(stderr, "Error: computer %d tries to submit a job "
               "when the queue is full.", id);
        exit(1);
    }
    printf("Computer %d submitted job %d\n", id, q->njobs_submitted);
    q->jobs[q->njobs_submitted ++] = t;
    return 0;
}

// check if a printer can get a job to start
// Return values:
// 1: yes. a job is available to start.
// 0: no. the queue is empty.
// -1: max number of jobs have been done.
int q_job_available (job_queue_t *q)
{
    if (q->njobs_started >= q->njobs_max)
        return -1;
    return q->njobs_submitted != q->njobs_started;
}

// printer id get a job to print
// Return value:
// value > 0: a value indicating the time needed for the job.
int q_fetch_job (job_queue_t *q, int id)
{
    int r = q_job_available (q);
    if (r < 0) {
        fprintf(stderr, "Error: printer %d tries to get a job "
               "after the max number of jobs have been started.", id);
        exit(1);
    } else if (r == 0) {
        fprintf(stderr, "Error: printer %d tries to start a job "
               "while no job is pending.", id);
        exit(1);
    }
    printf("Printer %d fetched job %d\n", id, q->njobs_started);
    return q->jobs[q->njobs_started ++];
}

// again, destroy your fields outside of this function
void q_destroy(job_queue_t *q)
{
    if (q->jobs) {
        free(q->jobs);
        q->jobs = NULL;
    }
}
///////////////////// END OF QUEUE /////////////////////


/*
 * main function for threads simulating computers
 */
void * computer_main(void * arg_orig)
{
    // initialize the state for random numbers
    // Do not remove the following line
    RANDOM_INIT();

    // You can start from the skeleton code included in tasks.c
    // TODO

    thread_data_t* info = (thread_data_t*)arg_orig;
    job_queue_t* jq = info->jq;
    int id = info->id;

    // The computer receives its id (a number from 0 to c - 1)
    // and a pointer to the job queue 'jq'

    int     done = 0;
    int     njobs = 0;

    while (! done) {
        // computer works and prepares a job
        int     job = WORK();
        int     r;

        // wait until queue is ready
        //printf("Computer Enter\n");
        pthread_mutex_lock(&jq->mutex);
        while (!q_can_submit(jq)) {
            //printf("computer waiting\n");
            pthread_cond_wait(&jq->computer_condition, &jq->mutex);
        }
        r = q_can_submit(jq);
        // r < 0 means all jobs are done
        if (r < 0) {
            done = 1;
            pthread_cond_broadcast(&jq->printer_condition);
            pthread_mutex_unlock(&jq->mutex);
            continue;
        }

        // submit a job to the queue
        // id is the id of this computer
        q_submit_job(jq, job, id);
        njobs ++;
        pthread_cond_broadcast(&jq->printer_condition);
        pthread_mutex_unlock(&jq->mutex);
    }
    // print out how many jobs this computer has submitted
    printf("Computer %d submitted %d jobs.\n", id, njobs);

    return NULL;
}

/*
 * main function for threads simulating printers
 */
void * printer_main(void * arg_orig)
{
    // You can start from the skeleton code included in tasks.c
    // TODO

    thread_data_t* info = (thread_data_t*)arg_orig;
    int id = info->id;
    job_queue_t* jq = info->jq;

    // The printer receives its id (a number from 0 to p - 1)
    // and a pointer to the job queue 'jq'



    int done = 0;
    int njobs = 0;

    while (! done) {
        int     r;

        // wait until a job is available
        //printf("Printer Enter\n");
        pthread_mutex_lock(&jq->mutex);
        while (!q_job_available(jq))  {
            //printf("Printer waiting\n");
            pthread_cond_wait(&jq->printer_condition, &jq->mutex);

        }
        //printf("Printer done waiting\n");
        r = q_job_available(jq);
        // r < 0 means all jobs have started
        if (r < 0) {
            done = 1;
            pthread_cond_broadcast(&jq->computer_condition);
            pthread_mutex_unlock(&jq->mutex);
            continue;
        }

        // fetch a job
        int job = q_fetch_job(jq, id);
        njobs ++;
        pthread_cond_broadcast(&jq->computer_condition);
        pthread_mutex_unlock(&jq->mutex);
        



        // print the job
        PRINT_JOB(job);
    }

    // print out how many jobs this printer has submitted
    printf("Printer %d completed %d jobs.\n", id, njobs);

    return NULL;
}


int main(int argc, char *argv[])
{
    int num_printers  = DEFAULT_NUM_PRINTERS;
    int num_computers = DEFAULT_NUM_COMPUTERS;
    int num_jobs      = DEFAULT_NUM_JOBS;
    int q_size        = DEFAULT_QUEUE_SIZE;

    int i, status;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            num_printers = atoi(argv[++i]);
            assert(num_printers > 0);
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            num_computers = atoi(argv[++i]);
            assert(num_computers> 0);
        } else if (strcmp(argv[i], "-j") == 0 && i + 1 < argc) {
            num_jobs = atoi(argv[++i]);
            assert(num_jobs > 0);
        } else if (strcmp(argv[i], "-q") == 0 && i + 1 < argc) {
            q_size = atoi(argv[++i]);
            assert(q_size > 0);
        } else {
            fprintf(stderr, "Usage: %s [-c N] [-p N] [-j N] [-q N]\n", argv[0]);
            fprintf(stderr, "-c: number of computers.\n");
            fprintf(stderr, "-p: number of printers.\n");
            fprintf(stderr, "-j: total number of jobs.\n");
            fprintf(stderr, "-q: number of entries in queue.\n");
            return 1;
        }
    }
    printf("num_jobs=%d\n", num_jobs);
    printf("num_computers=%d\n", num_computers);
    printf("num_printers=%d\n", num_printers);
    printf("q_size=%d\n", q_size);

    // define job_queue and initialize it
    job_queue_t job_queue;

    q_init(&job_queue, q_size, num_jobs);

    // add any necessary init functions below
    // TODO
    pthread_mutex_init(&job_queue.mutex, NULL);
    pthread_cond_init(&job_queue.printer_condition, NULL);
    pthread_cond_init(&job_queue.computer_condition, NULL);



    // start and wait on the computer and printer threads
    int n = num_computers + num_printers;      // number of threads to create

    pthread_t tid[n];
    thread_data_t data[n];

    int seed = (num_jobs << 24) ^ (num_computers << 16) ^ (num_printers << 8);
    for(i = 0; i < num_computers; i++) {
        data[i].id = i;
        data[i].jq = &job_queue;
        data[i].seed = seed + i;
        status = pthread_create(&tid[i], NULL, computer_main, &data[i]);
        assert(status == 0);
    }
    for(i = num_computers; i < n; i++) {
        data[i].id = i - num_computers;
        data[i].jq = &job_queue;
        data[i].seed = 0;
        status = pthread_create(&tid[i], NULL, printer_main, &data[i]);
        assert(status == 0);
    }

    for(i = 0; i < n; i++) {
        status = pthread_join(tid[i], NULL);
        assert(status == 0);
    }

    // add any necessary destroy functions below
    // TODO
    pthread_cond_destroy(&job_queue.printer_condition);
    pthread_cond_destroy(&job_queue.computer_condition);
    pthread_mutex_destroy(&job_queue.mutex);


    q_destroy(&job_queue);
    return 0;
}
