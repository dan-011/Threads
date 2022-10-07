void* computer_main(void * arg)
{
    // The computer receives its id (a number from 0 to c - 1)
    // and a pointer to the job queue 'jq'

    int     done = 0;
    int     njobs = 0;

    while (! done) {
        // computer works and prepares a job
        int     job = work();
        int     r;

        // wait until queue is ready
        r = q_can_submit(jq);
        while (r == 0) {
            r = q_can_submit(jq);
        }

        // r < 0 means all jobs are done
        if (r < 0) {
            done = 1;
            continue;
        }

        // submit a job to the queue
        // id is the id of this computer
        q_submit_job(jq, job, id);

        njobs ++;
    }
    // print out how many jobs this computer has submitted
    printf("Computer %d submitted %d jobs.\n", id, njobs);
}


void* printer_main(void * arg)
{
    // The printer receives its id (a number from 0 to p - 1)
    // and a pointer to the job queue 'jq'

    int done = 0;
    int njobs = 0;

    while (! done) {
        int     r;

        // wait until a job is available
        r = q_job_available(jq);
        while (r == 0)  {
            r = q_job_available(jq);
        }

        // r < 0 means all jobs have started
        if (r < 0) {
            done = 1;
            continue;
        }

        // fetch a job
        int job = q_fetch_job(jq, id);

        njobs ++;

        // print the job
        print_job(job);
    }

    // print out how many jobs this printer has submitted
    printf("Printer %d completed %d jobs.\n", id, njobs);
}

