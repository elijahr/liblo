/*
 *  Copyright (C) 2014 Steve Harris et al. (see AUTHORS)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 2.1 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  $Id$
 */

#include "lo_types_internal.h"
#include "lo/lo.h"

#if defined(USE_ANSI_C) || defined(DLL_EXPORT)
lo_timetag lo_get_tt_immediate()
{
    lo_timetag tt = { 0U, 1U };
    return tt;
}
#endif
#ifndef _MSC_VER
#include <sys/time.h>
#endif
#include <time.h>

#if defined(__MACH__) && !defined(HAVE_CLOCK_REALTIME)
// on OS X <= 10.11, use mach clock_get_time
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#define JAN_1970 0x83aa7e80     /* 2208988800 1970 - 1900 in seconds */

double lo_timetag_diff(lo_timetag a, lo_timetag b)
{
    return (double) a.sec - (double) b.sec +
        ((double) a.frac - (double) b.frac) * 0.00000000023283064365;
}

void lo_timetag_now(lo_timetag * t)
{
#if defined(WIN32) || defined(_MSC_VER)
    /*
       FILETIME is the time in units of 100 nsecs from 1601-Jan-01
       1601 and 1900 are 9435484800 seconds apart.
     */
    FILETIME ftime;
    double dtime;
    GetSystemTimeAsFileTime(&ftime);
    dtime =
        ((ftime.dwHighDateTime * 4294967296.e-7) - 9435484800.) +
        (ftime.dwLowDateTime * 1.e-7);

    t->sec = (uint32_t) dtime;
    t->frac = (uint32_t) ((dtime - t->sec) * 4294967296.);
#else
#ifdef HAVE_CLOCK_REALTIME
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    // tv_sec is seconds since UNIX epoch, but OSC timetags are seconds
    // since Jan 1 1900, so add the difference
    t->sec = ts.tv_sec + JAN_1970;
    t->frac = ts.tv_nsec * 4294967.295;
#else
#ifdef __MACH__
    // on OS X <= 10.11, use mach clock_get_time
    clock_serv_t clock_serv;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &clock_serv);
    clock_get_time(clock_serv, &mts);
    mach_port_deallocate(mach_task_self(), clock_serv);
    // tv_sec is seconds since UNIX epoch, but OSC timetags are seconds
    // since Jan 1 1900, so add the difference
    t->sec = mts.tv_sec + JAN_1970;
    t->frac = mts.tv_nsec * 4294967.295;
#else
    // Fallback to gettimeofday()
    struct timeval tv;
    gettimeofday(&tv, NULL);
    // tv_sec is seconds since UNIX epoch, but OSC timetags are seconds
    // since Jan 1 1900, so add the difference
    t->sec = tv.tv_sec + JAN_1970;
    t->frac = tv.tv_usec * 4294.967295;
#endif
#endif
#endif
}
