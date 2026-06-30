/*
    Copyright (c) 2007 by Cisco Systems, Inc.
    All rights reserved.

    float_test.c - Float point arithmetic performance test
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include INTERNAL_INC(lc/include_private/common.h)
#include INTERNAL_INC(lc/mb/include_private/bench.h)


float f1 = 1.;
float f2 = 2.;
float f3;
double d1 = 1.;
double d2 = 1.;
double d3;
int i1 = 1;
int i2 = 1;
int i3;
long l1 = 1;
long l2 = 1;
long l3;
long long ll1 = 1;
long long ll2 = 1;
long long ll3;


int main (int argc, char** argv)
{
    int i;
    uint32 start, stop, run_time;

    // Single precision add
    start = get_time();
    for (i=0; i<1000; i++) {
        f3 = f1 + f2;
    }
    stop = get_time();
    run_time = get_time_diff(start, stop);
    printf("Single precision floating point add: %d ns\n",
           CYCLE_TO_US(run_time));

    // Single precision multiply
    start = get_time();
    for (i=0; i<1000; i++) {
        f3 = f1 * f2;
    }
    stop = get_time();
    run_time = get_time_diff(start, stop);
    printf("Single precision floating point multiply: %d ns\n",
           CYCLE_TO_US(run_time));

    // Single precision divide
    start = get_time();
    for (i=0; i<1000; i++) {
        f3 = f1 / f2;
    }
    stop = get_time();
    run_time = get_time_diff(start, stop);
    printf("Single precision floating point divide: %d ns\n",
           CYCLE_TO_US(run_time));


    // Double precision add
    start = get_time();
    for (i=0; i<1000; i++) {
        d3 = d1 + d2;
    }
    stop = get_time();
    run_time = get_time_diff(start, stop);
    printf("Double precision floating point add: %d ns\n",
           CYCLE_TO_US(run_time));

    // Double precision multiply
    start = get_time();
    for (i=0; i<1000; i++) {
        d3 = d1 * d2;
    }
    stop = get_time();
    run_time = get_time_diff(start, stop);
    printf("Double precision floating point multiply: %d ns\n",
           CYCLE_TO_US(run_time));

    // Double precision divide
    start = get_time();
    for (i=0; i<1000; i++) {
        d3 = d1 / d2;
    }
    stop = get_time();
    run_time = get_time_diff(start, stop);
    printf("Double precision floating point divide: %d ns\n",
           CYCLE_TO_US(run_time));


    // Integer add
    start = get_time();
    for (i=0; i<1000000; i++) {
        i3 = i1 + i2;
    }
    stop = get_time();
    run_time = get_time_diff(start, stop);
    printf("Integer add: %d ps\n",
           CYCLE_TO_US(run_time));

    // Integer multply
    start = get_time();
    for (i=0; i<1000000; i++) {
        i3 = i1 * i2;
    }
    stop = get_time();
    run_time = get_time_diff(start, stop);
    printf("Integer multiply: %d ps\n",
           CYCLE_TO_US(run_time));

    // Integer divide
    start = get_time();
    for (i=0; i<1000000; i++) {
        i3 = i1 / i2;
    }
    stop = get_time();
    run_time = get_time_diff(start, stop);
    printf("Integer divide: %d ps\n",
           CYCLE_TO_US(run_time));


    // Long add
    start = get_time();
    for (i=0; i<1000000; i++) {
        l3 = l1 + l2;
    }
    stop = get_time();
    run_time = get_time_diff(start, stop);
    printf("Long integer add: %d ps\n",
           CYCLE_TO_US(run_time));

    // Long multply
    start = get_time();
    for (i=0; i<1000000; i++) {
        l3 = l1 * l2;
    }
    stop = get_time();
    run_time = get_time_diff(start, stop);
    printf("Long integer multiply: %d ps\n",
           CYCLE_TO_US(run_time));

    // Long divide
    start = get_time();
    for (i=0; i<1000000; i++) {
        l3 = l1 / l2;
    }
    stop = get_time();
    run_time = get_time_diff(start, stop);
    printf("Long integer divide: %d ps\n",
           CYCLE_TO_US(run_time));


    // Long long add
    start = get_time();
    for (i=0; i<1000000; i++) {
        ll3 = ll1 + ll2;
    }
    stop = get_time();
    run_time = get_time_diff(start, stop);
    printf("Long long integer add: %d ps\n",
           CYCLE_TO_US(run_time));

    // Long long multply
    start = get_time();
    for (i=0; i<1000000; i++) {
        ll3 = ll1 * ll2;
    }
    stop = get_time();
    run_time = get_time_diff(start, stop);
    printf("Long long integer multiply: %d ps\n",
           CYCLE_TO_US(run_time));

    // Long long divide
    start = get_time();
    for (i=0; i<1000000; i++) {
        ll3 = ll1 / ll2;
    }
    stop = get_time();
    run_time = get_time_diff(start, stop);
    printf("Long long integer divide: %d ps\n",
           CYCLE_TO_US(run_time));

    return 0;
}
