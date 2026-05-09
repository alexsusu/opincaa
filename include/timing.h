/*
 *
 * File: timing.h
 *
 * Functions for time measurements.
 *
 */

int GetMilliCount();
int GetMilliSpan(int nTimeStart);
void CountMilliTime();

// A more accurate timer
long long GetNanos();
