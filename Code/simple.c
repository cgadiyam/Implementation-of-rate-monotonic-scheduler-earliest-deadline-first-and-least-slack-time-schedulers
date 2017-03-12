#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <pthread.h>
#include <time.h>
#include <sys/trace.h>

#include "time_slicer.h"
#include "function_helpers.h"

#define TCOUNT 10
#define COUNT_LIMIT 12

int     count = 0;
long int iterations;
pthread_mutex_t count_mutex;
pthread_cond_t count_threshold_cv;
extern bool terminate_program;

void *idle_func(void *meta) {
    pthread_setschedprio(pthread_self(), 2);
    while(terminate_program == false);
    pthread_exit(NULL);
}

void *task_thread(void *meta)
{
    int i=0;
    int j;
    thread_meta_t * my_meta = (thread_meta_t *) meta;
    int my_id =  my_meta->id;

    if (my_meta->id == 0) {
        //printf("\n\nim in %d\n\n", my_meta->id);
    }
    while(terminate_program == false) {
        // wait for a new period to start
        while(my_meta->finished == true && terminate_program == false) {
        }
        //printf("\n\nim out %d\n\n", my_meta->id);
        struct sched_param param;
        int policy;
        // Set them all high and then drop them so they go to the front of the queue
        pthread_getschedparam(my_meta->thread, &policy,&param);
        //printf("finished = %d, with priority %d, %d, for thread %d\n",my_meta->finished,param.sched_curpriority, param.sched_priority, my_meta->id);;
        i++;

        //This is the function "processing"
        //printf("  ***STARTED thread = %d, count = %d\n", my_meta->id, i);
        TraceEvent(_NTO_TRACE_INSERTSUSEREVENT, 104, 1, 11);
        //fflush(stdout);
        //wait for 2 seconds
        my_meta->Exec_Cycles = 0;
        for (j=0; (j<(my_meta->execution_time)); j++) {
            //wait for .01 seconds
            nanospin_ns(5000000);
            (my_meta->Exec_Cycles)++;
            //pthread_getschedparam(my_meta->thread, &policy,&param);
            //printf("finished = %d, with priority %d, %d, for thread %d\n",my_meta->finished,param.sched_curpriority, param.sched_priority, my_meta->id);;

        }

        //printf("  ###FINISHED thread = %d, count = %d\n", my_id, i);
        TraceEvent(_NTO_TRACE_INSERTSUSEREVENT, 105, 1, 11);
        my_meta->finished = true;
        //puts("----------THREAD FINISHED FOR THE PERIOD");
        pthread_cond_signal(&count_threshold_cv);
    }
    pthread_exit(NULL);
}

void *scheduler_thread_func(void * array_of_thread_meta_t)
{
    thread_meta_t * all_threads = (thread_meta_t  *) array_of_thread_meta_t;
    //this is just to test. This is the wrong thread id
    int my_id=99;
    int x;
    //printf("Starting MASTER THREDDER: thread %d\n", my_id);
    pthread_t self = pthread_self();
    // Set the priority to 10. All other threads must be lower.
    // WARNING IF PRIORTY GREATER THAN 10 IT IS HIGHER THAN MANY ESSENTIAL DEBUGGING FUNCTIONS SUCH AS PRINTING.
    // TODO: clean this up and add MAX_PRIORITY, HIGH_PRIORITY, LOW_PRIORITY macros
    int policy;
    struct sched_param param;
    //printf("@@MASTER thread_id %d\n", self);
    pthread_getschedparam(self, &policy, &param);
    //printf("    MASTER priority = %d\n", param.sched_priority);
    //printf("    MASTER policy = %d\n", policy);
    param.sched_priority = 10;
    pthread_setschedparam(self, SCHED_FIFO, &param);
    //printf("@@MASTER thread_id %d\n", self);
    pthread_getschedparam(self, &policy, &param);
    //printf("    MASTER priority = %d\n", param.sched_priority);
    //printf("    MASTER policy = %d\n", policy);

    pthread_mutex_lock(&count_mutex);
    // Wait for the time slice to trip so we can synchronize
    pthread_cond_wait(&count_threshold_cv, &count_mutex);
    // Reset the time slice counter
    reset_time_slice();

    calc_next_thread(all_threads);
    // Set the highest priority thread as such
    run_next_thread(all_threads);


    pthread_cond_wait(&count_threshold_cv, &count_mutex);
    reset_time_slice();
    //open the flood gates
    for (x=0; x<NUM_THREADS; x++) {
        all_threads[x].finished = false;
    }

    while (terminate_program == false) {
        TraceEvent(_NTO_TRACE_INSERTSUSEREVENT, 100, 1, 11);
        check_missed_deadline(all_threads);
        check_new_periods(all_threads);
        //calculate next thread
        calc_next_thread(all_threads);
        // Set next thread to run
        run_next_thread(all_threads);
        iterations++;
        TraceEvent(_NTO_TRACE_INSERTSUSEREVENT, 101, 1, 11);

        pthread_cond_wait(&count_threshold_cv, &count_mutex);
        ////puts("RETURN OF THE JEDI!!");
        //fprintf(stderr, "MASTER THREAD pthread_cond_timedwait %s\n", strerror(ret));
        for (x=0; x<NUM_THREADS; x++) {
            struct sched_param param;
            int policy;
            // Set them all high and then drop them so they go to the front of the queue
            pthread_getschedparam(all_threads[x].thread, &policy,&param);
            //printf("finished = %d, with priority %d, %d, for thread %d\n",all_threads[x].finished,param.sched_curpriority, param.sched_priority, all_threads[x].id);;
        }

    }

    //kill the idle thread
    for (x=0; x<NUM_THREADS; x++) {
        all_threads[x].priority = 1;
    }
    run_next_thread(all_threads);

    run_next_thread(all_threads);
    pthread_mutex_unlock(&count_mutex);
    pthread_exit(NULL);
}

int main (int argc, char *argv[])
{
    int j;
    //calibrate the nanospin (probably only needs to be run once)
    for (j=0; j<0; j++) {
        //wait for .01 seconds
        nanospin_ns((unsigned long)100000000);
    }
    //end calibration

    //scheduler task
    pthread_t scheduler_thread;
    //idle thread
    pthread_t idle_thread;


    pthread_t time_slice_thread;
    thread_meta_t thread_test[NUM_THREADS] = {{0}};
    /***************************************************************************/
    //(3,10,10), (3,10,10), (3,10,10) ALL TASK ARE EQUAL
    //Task 1
    thread_test[0].id = 0;
    thread_test[0].execution_time = 3;
    thread_test[0].deadline = 10;
    thread_test[0].finished = true;
    thread_test[0].last_time_slice_finish_cleared = 0;

    //Task 2
    thread_test[1].id = 1;
    thread_test[1].execution_time = 3;
    thread_test[1].deadline = 10;
    thread_test[1].finished = true;
    thread_test[1].last_time_slice_finish_cleared = 0;

    //Task 3
    thread_test[2].id = 2;
    thread_test[2].execution_time = 3;
    thread_test[2].deadline = 10;
    thread_test[2].finished = true;
    thread_test[2].last_time_slice_finish_cleared = 0;

    /***************************************************************************/
    //(3,4,4), (2,6,6), (3,9,9) FAILING TASK
    //Task 1
    /*thread_test[0].id = 0;
    thread_test[0].execution_time = 3;
    thread_test[0].deadline = 4;
    thread_test[0].finished = true;
    thread_test[0].last_time_slice_finish_cleared = 0;

    //Task 2
    thread_test[1].id = 1;
    thread_test[1].execution_time = 2;
    thread_test[1].deadline = 6;
    thread_test[1].finished = true;
    thread_test[1].last_time_slice_finish_cleared = 0;

    //Task 3
    thread_test[2].id = 2;
    thread_test[2].execution_time = 3;
    thread_test[2].deadline = 9;
    thread_test[2].finished = true;
    thread_test[2].last_time_slice_finish_cleared = 0;*/


    /***************************************************************************/
    //(1,4,4), (2,5,5), (1,8,8), (1,10,10) set 3
    //Task 1
    /*thread_test[0].id = 0;
    thread_test[0].execution_time = 1;
    thread_test[0].deadline = 4;
    thread_test[0].finished = true;
    thread_test[0].last_time_slice_finish_cleared = 0;

    //Task 2
    thread_test[1].id = 1;
    thread_test[1].execution_time = 2;
    thread_test[1].deadline = 5;
    thread_test[1].finished = true;
    thread_test[1].last_time_slice_finish_cleared = 0;

    //Task 3
    thread_test[2].id = 2;
    thread_test[2].execution_time = 1;
    thread_test[2].deadline = 8;
    thread_test[2].finished = true;
    thread_test[2].last_time_slice_finish_cleared = 0;

    //Task 4
    thread_test[3].id = 3;
    thread_test[3].execution_time = 1;
    thread_test[3].deadline = 10;
    thread_test[3].finished = true;
    thread_test[3].last_time_slice_finish_cleared = 0;*/


    /***************************************************************************/
    //(1,3,3), (2,5,5), (1,10,10) set 2
    /*thread_test[0].id = 0;
     thread_test[0].execution_time = 1;
     thread_test[0].deadline = 3;
     thread_test[0].finished = true;
     thread_test[0].last_time_slice_finish_cleared = 0;

     //Task 2
     thread_test[1].id = 1;
     thread_test[1].execution_time = 2;
     thread_test[1].deadline = 5;
     thread_test[1].finished = true;
     thread_test[1].last_time_slice_finish_cleared = 0;

     //Task 3
     thread_test[2].id = 2;
     thread_test[2].execution_time = 1;
     thread_test[2].deadline = 10;
     thread_test[2].finished = true;
     thread_test[2].last_time_slice_finish_cleared = 0;*/

    /***************************************************************************/

    //(1,7,7), (2,5,5), (1,8,8), (1,10,10), (2,16,16) set 1
    /*thread_test[0].id = 0;
        thread_test[0].execution_time = 1;
        thread_test[0].deadline = 7;
        thread_test[0].finished = true;
        thread_test[0].last_time_slice_finish_cleared = 0;

        //Task 2
        thread_test[1].id = 1;
        thread_test[1].execution_time = 2;
        thread_test[1].deadline = 5;
        thread_test[1].finished = true;
        thread_test[1].last_time_slice_finish_cleared = 0;

        //Task 3
        thread_test[2].id = 2;
        thread_test[2].execution_time = 1;
        thread_test[2].deadline = 8;
        thread_test[2].finished = true;
        thread_test[2].last_time_slice_finish_cleared = 0;

        //Task 4
        thread_test[3].id = 3;
        thread_test[3].execution_time = 1;
        thread_test[3].deadline = 10;
        thread_test[3].finished = true;
        thread_test[3].last_time_slice_finish_cleared = 0;

        //Task 5
        thread_test[4].id = 4;
        thread_test[4].execution_time = 2;
        thread_test[4].deadline = 16;
        thread_test[4].finished = true;
        thread_test[4].last_time_slice_finish_cleared = 0;*/
    /***************************************************************************/
    /* Initialize mutex and condition variable objects */
    pthread_mutex_init(&count_mutex, NULL);
    pthread_cond_init (&count_threshold_cv, NULL);

    /* For portability, explicitly create threads in a joinable state */
    // This thread will probably remain in main
    // //printf("MASTER thread_id %d\n", &scheduler_thread.thread);
    //These are the task threads.
    pthread_create(&thread_test[0].thread, NULL, task_thread, &thread_test[0]);
    pthread_create(&thread_test[1].thread, NULL, task_thread, &thread_test[1]);
    pthread_create(&thread_test[2].thread, NULL, task_thread, &thread_test[2]);
    //pthread_create(&thread_test[3].thread, NULL, task_thread, &thread_test[3]);
    //pthread_create(&thread_test[4].thread, NULL, task_thread, &thread_test[4]);
    pthread_create(&time_slice_thread,NULL, time_slice_func, NULL);
    pthread_create(&idle_thread,NULL, idle_func, NULL);
    pthread_create(&scheduler_thread, NULL, scheduler_thread_func, thread_test);
    /* Wait for all threads to complete */
    int i;
    for (i=0; i<NUM_THREADS; i++) {
        pthread_join(thread_test[i].thread, NULL);
    }
    printf ("Main(): Waited on %ld  iterations. Done.\n", iterations);

    /* Clean up and exit */
    pthread_mutex_destroy(&count_mutex);
    pthread_cond_destroy(&count_threshold_cv);
    pthread_exit(NULL);
}
