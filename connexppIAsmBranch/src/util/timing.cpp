/*
 *
 * File: timing.cpp
 *
 * Functions for time measurements.
 *
 */
#include <sys/timeb.h>
#include <stdio.h>
int GetMilliCount()
{
  // Something like GetTickCount but portable
  // It rolls over every ~ 12.1 days (0x100000/24/60/60)
  // Use GetMilliSpan to correct for rollover
  timeb tb;
  ftime( &tb );
  int nCount = tb.millitm + (tb.time & 0xfffff) * 1000;
  return nCount;
}

int GetMilliSpan( int nTimeStart )
{
  int nSpan = GetMilliCount() - nTimeStart;
  if ( nSpan < 0 )
    nSpan += 0x100000 * 1000;
  return nSpan;
}

void CountMilliTime()
{
    int i;
    int tstart;

    for (i = 9; i >= 0 ; i--)
    {
        tstart = GetMilliCount();
        printf("Countdown %d\n",i);
        while ( GetMilliSpan(tstart) < 1000)
            ;//wait
    }
}
