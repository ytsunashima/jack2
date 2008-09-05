/*
Copyright (C) 2001-2003 Paul Davis
Copyright (C) 2004-2008 Grame

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#ifndef __JackTime_linux__
#define __JackTime_linux__

#ifdef __cplusplus
extern "C"
{
#endif

#include <unistd.h>

    static inline void JackSleep(long usec) 
    {
        usleep(usec);
    }

#ifdef GETCYCLE_TIME
#include "cycles.h"
    extern jack_time_t __jack_cpu_mhz;
    jack_time_t GetMhz();
    void InitTime();
    static inline jack_time_t GetMicroSeconds(void) 
    {
        return get_cycles() / __jack_cpu_mhz;
    }
#else
#include <time.h>
    void InitTime();
    static inline jack_time_t GetMicroSeconds(void) 
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (jack_time_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
    }
#endif

#ifdef __cplusplus
}
#endif

#endif

