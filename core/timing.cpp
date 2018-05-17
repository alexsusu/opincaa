/*
 *
 * File: timing.cpp
 *
 * Functions for time measurements.
 *
 */
#include <sys/timeb.h>
#include <stdio.h>
#include <time.h>

int GetMilliCount() {
  // Something like GetTickCount but portable
  // It rolls over every ~ 12.1 days (0x100000/24/60/60)
  // Use GetMilliSpan to correct for rollover
  timeb tb;
  ftime( &tb );
  int nCount = tb.millitm + (tb.time & 0xfffff) * 1000;
  return nCount;
}

int GetMilliSpan( int nTimeStart ) {
  int nSpan = GetMilliCount() - nTimeStart;
  if ( nSpan < 0 )
    nSpan += 0x100000 * 1000;
  return nSpan;
}

void CountMilliTime() {
    int i;
    int tstart;

    for (i = 9; i >= 0 ; i--) {
        tstart = GetMilliCount();
        printf("Countdown %d\n",i);
        while ( GetMilliSpan(tstart) < 1000)
            ; // wait
    }
}

// Alex: added more accurate timer
long long GetNanos() {
    long long res;
    // See for specs http://pubs.opengroup.org/onlinepubs/7908799/xsh/time.h.html
    struct timespec ts;

  // It works on ArchLinux
  #define BETTER_ARM_LINUX_THAN_THIS_DISTRO

  #ifdef BETTER_ARM_LINUX_THAN_THIS_DISTRO
    timespec_get(&ts, 1); //TIME_UTC);

    /*
    printf("GetNanos(): ts.tv_sec = %ld\n", ts.tv_sec);
    printf("GetNanos(): ts.tv_nsec = %ld\n", ts.tv_nsec);
    */

    res = (long)ts.tv_sec;
    res *= 1000000000L;
    res += ts.tv_nsec;
    return res;

    //return (long)ts.tv_sec * 1000000000L + ts.tv_nsec;
  #else
    return -1;
  #endif
}

