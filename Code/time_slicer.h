#ifndef TIME_SLICER_H
#define TIME_SLICER_H

#define TIMESLICE_PERIOD 10000 //This is in nanoseconds

// This is the function that handles defining time slices.
void * time_slice_func(void * empty);

// This function returns the number of time slices that have passed.
long int get_time_slice(void);

// Reset the time slice counter
void reset_time_slice(void);

#endif
