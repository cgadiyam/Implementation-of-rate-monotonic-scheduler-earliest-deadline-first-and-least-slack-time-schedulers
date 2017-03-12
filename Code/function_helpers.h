#ifndef FUNCTION_HELPERS_H
#define FUNCTION_HELPERS_H

#include <pthread.h>
#include <stdbool.h>

#define NUM_THREADS  3

typedef struct {
    // Add whatever you want here.
    pthread_t thread;
    int id;
    int execution_time;
    int deadline;
    bool finished;
    int priority;
    int next_deadline;
    int Exec_Cycles;
    int Slack_Time;
    long int last_time_slice_finish_cleared;
} thread_meta_t;

void check_missed_deadline(thread_meta_t * all_threads);

void check_new_periods(thread_meta_t * all_threads);

void run_next_thread (thread_meta_t * all_threads);

void calc_next_thread(thread_meta_t * all_threads);

#endif
