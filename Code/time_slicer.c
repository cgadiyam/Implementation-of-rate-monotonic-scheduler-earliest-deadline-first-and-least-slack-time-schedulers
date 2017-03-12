#include "time_slicer.h"

#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <sys/trace.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include <sys/siginfo.h>

#define TIMESLICE_PRIORITY 11
#define MAX_TIMESLICES 500

long int timeslice = 0;
bool terminate_program = false;
pthread_mutex_t timeslice_mutex;
pthread_cond_t timeslice_condition;

// scheduler thread condition
extern pthread_cond_t count_threshold_cv;

#define MY_PULSE_CODE   _PULSE_CODE_MINAVAIL

int DO_NOT_USE;
void * time_slice_func(void * empty) {
    // Set the priority
    pthread_setschedprio(pthread_self(), TIMESLICE_PRIORITY);

    // Init the timer
    struct timespec timeslice_timeout;
    // Change this value to change when the timer interrupt occurs
    timeslice_timeout.tv_nsec = TIMESLICE_PERIOD;

    /* Initialize mutex and condition variable objects */
    pthread_mutex_init(&timeslice_mutex, NULL);
    pthread_cond_init (&timeslice_condition, NULL);

    pthread_mutex_lock(&timeslice_mutex);

    while(terminate_program == false) {

        // Reset the current time
        //timeslice_timeout.tv_sec = time(0)+1;

        sleepon_t * pl;
        _sleepon_init( &pl, 0);
        _sleepon_lock(pl);

        _sleepon_wait( pl,&DO_NOT_USE,5000000);
        _sleepon_unlock(pl);
        _sleepon_destroy(pl);

        // Start the timeslice
        //pthread_cond_timedwait(&timeslice_condition, &timeslice_mutex, &timeslice_timeout);
        TraceEvent(_NTO_TRACE_INSERTSUSEREVENT, 103, 1, 11);
        //fprintf(stderr, "TIMESLICER pthread_cond_timedwait %s\n", strerror(ret));
        //printf("----------TIMESLICE %2ld\n", timeslice+1);
        timeslice++;
        if (timeslice == MAX_TIMESLICES) {
            terminate_program = true;
        }

        // Tell the scheduler to wake up
        pthread_cond_signal(&count_threshold_cv);
    }

    //puts("TERMINATEDxxx");
    pthread_mutex_unlock(&timeslice_mutex);
    pthread_exit(NULL);
}
long int get_time_slice(void) {
    return timeslice;
}

void reset_time_slice(void) {
    timeslice = 0;
}
