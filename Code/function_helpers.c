#include "function_helpers.h"
#include "time_slicer.h"

#include <stdio.h>
#include <sys/trace.h>

#define TCOUNT 10
#define COUNT_LIMIT 12
#define SCHEDULER 3

extern long int timeslice;

void check_missed_deadline(thread_meta_t * all_threads) {
    int x;
    for (x=0; x<NUM_THREADS; x++) {
        if (all_threads[x].finished == false) {
            if(timeslice%all_threads[x].deadline ==0) {
                if (all_threads[x].last_time_slice_finish_cleared != timeslice) {
                    TraceEvent(_NTO_TRACE_INSERTSUSEREVENT, 106, 1, 11);
                }
            }
        }
    }


}
void check_new_periods(thread_meta_t * all_threads) {
    int x;
    for (x=0; x<NUM_THREADS; x++) {
        if (all_threads[x].finished == true ) {
            if(timeslice%all_threads[x].deadline ==0) {
                if (all_threads[x].last_time_slice_finish_cleared != timeslice) {
                    all_threads[x].last_time_slice_finish_cleared = timeslice;
                    //printf("....RESET THREAD %d\n", all_threads[x].id);
                    all_threads[x].finished = false;
                }
            }
        }
    }
}


void run_next_thread (thread_meta_t * all_threads) {
    int x;
    for (x=0; x<NUM_THREADS; x++) {
        // Set them all high and then drop them so they go to the front of the queue
        pthread_setschedprio(all_threads[x].thread, 8);
        pthread_setschedprio(all_threads[x].thread, all_threads[x].priority);
    }
    for (x=0; x<NUM_THREADS; x++) {
        struct sched_param param;
        int policy;
        // Set them all high and then drop them so they go to the front of the queue
        pthread_getschedparam(all_threads[x].thread, &policy,&param);
        if (param.sched_priority != all_threads[x].priority) {
            //puts("WE HAVE FAILED THIS CITY");
        }
        if (param.sched_curpriority != all_threads[x].priority) {
            //puts("WE HAVE FAILED THIS CITY");
        }
        //printf("priority = %d\n", all_threads[x].priority);

    }

}

void calc_next_thread(thread_meta_t * all_threads)
{
    int i,count,temp;
    bool exit=false;

    int x;
    int lowest_deadline_thread = 0;
    int lowest_deadline = 999999999;
    int Earliest_dealine_thread = 0;
    int Earliest_dealine = 999999999;
    int Least_Slack_Time = 999999999;
    int Least_Slack_Time_Thread = 0;

    switch(SCHEDULER)
    {
    case 1:
        lowest_deadline_thread = 99;
        for (x=0; x<NUM_THREADS; x++) {
            //reset all the priorities
            all_threads[x].priority = 1;
            if (all_threads[x].finished == false && all_threads[x].deadline < lowest_deadline) {
                lowest_deadline = all_threads[x].deadline;
                lowest_deadline_thread = x;
                //printf("new lowest %d\n", x);
            }
            if (all_threads[x].finished == true) {
                //printf("not eligible %d\n", x);
            }
        }
        if (lowest_deadline_thread != 99) {
            all_threads[lowest_deadline_thread].priority = 8;
            //printf("Next running thread should be %d!\n", all_threads[lowest_deadline_thread].id);
        }
        else {
            //puts("Idle thread should run!!!!");
        }


        break;
    case 2:
        //earliest deadline first scheduling
        count = get_time_slice();
        //printf("\ntime slice: %ld",count);

        //calculates next deadline for each thread
        for(i=0; i<3; i++)
        {
            temp = all_threads[i].deadline;
            exit = false;
            while(!exit)
            {
                if(temp>count)
                {
                    if(all_threads[i].finished == true)
                    {
                        temp+=all_threads[i].deadline;
                    }

                    all_threads[i].next_deadline=temp;
                    //printf("\nthread %d next deadline: %d",i,temp);
                    exit=true;
                }
                temp+=all_threads[i].deadline;
            }
        }

        Earliest_dealine_thread = 99;
        for (x=0; x<NUM_THREADS; x++) {
            //reset all the priorities
            all_threads[x].priority = 1;
            if (all_threads[x].finished == false && all_threads[x].next_deadline < Earliest_dealine) {
                Earliest_dealine = all_threads[x].next_deadline;
                Earliest_dealine_thread = x;
                //printf("new lowest %d\n", x);
            }
            if (all_threads[x].finished == true) {
                //printf("not eligible %d\n", x);
            }
        }
        if (Earliest_dealine_thread != 99) {
            all_threads[Earliest_dealine_thread].priority = 8;
            //printf("\nNext running thread should be %d!\n", all_threads[Earliest_dealine_thread].id);
        }
        else {
            //puts("Idle thread should run!!!!");
        }
        break;
    case 3:
        //least slack time scheduler
        //least slack time scheduler Exec_Cycles
        //calculate slack time for each task
        for(i=0; i<3; i++)
        {
            all_threads[i].Slack_Time = all_threads[i].execution_time - (all_threads[i].Exec_Cycles/40);
            //printf("\nthread %d slack time: %d",i,all_threads[i].Slack_Time);
        }

        Least_Slack_Time_Thread = 99;
        for (x=0; x<NUM_THREADS; x++) {
            //reset all the priorities
            all_threads[x].priority = 1;
            if (all_threads[x].finished == false && all_threads[x].deadline < Least_Slack_Time) {
                Least_Slack_Time = all_threads[x].Slack_Time;
                Least_Slack_Time_Thread = x;
                //printf("new lowest %d\n", x);
            }
            if (all_threads[x].finished == true) {
                //printf("not eligible %d\n", x);
            }
        }
        if (Least_Slack_Time_Thread != 99) {
            all_threads[Least_Slack_Time_Thread].priority = 8;
            //printf("\nNext running thread should be %d!\n", all_threads[Least_Slack_Time_Thread].id);
        }
        else {
            //puts("Idle thread should run!!!!");
        }
        break;
    default:
        break;
        //printf("Invalid scheduler input !\n");
    }
}
