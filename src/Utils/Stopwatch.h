/*
 * This file is part of SurfelMapping.
 *
 * Copyright (C) 2015 Imperial College London
 * 
 * The use of the code within this file and all code within files that 
 * make up the software that is SurfelMapping is permitted for
 * non-commercial purposes only.  The full terms and conditions that 
 * apply to the code within this file are detailed within the LICENSE.txt 
 * file and at <http://www.imperial.ac.uk/dyson-robotics-lab/downloads/elastic-fusion/elastic-fusion-license/> 
 * unless explicitly stated.  By downloading this file you agree to 
 * comply with these terms.
 *
 * If you wish to use any of this code for commercial purposes then 
 * please email researchcontracts.engineering@imperial.ac.uk.
 *
 */

#ifndef STOPWATCH_H_
#define STOPWATCH_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>
#include <iostream>
#include <map>

#include <sys/time.h>
#include <unistd.h>


#ifndef DISABLE_STOPWATCH
#define STOPWATCH(name, expression) \
    do \
    { \
        const unsigned long long int startTime = Stopwatch::getInstance().getCurrentSystemTime(); \
        expression \
        const unsigned long long int endTime = Stopwatch::getInstance().getCurrentSystemTime(); \
        Stopwatch::getInstance().addStopwatchTiming(name, endTime - startTime); \
    } \
    while(false)

#define TICK(name) \
    do \
    { \
        Stopwatch::getInstance().tick(name, Stopwatch::getInstance().getCurrentSystemTime()); \
    } \
    while(false)

#define TOCK(name) \
    do \
    { \
        Stopwatch::getInstance().tock(name, Stopwatch::getInstance().getCurrentSystemTime()); \
    } \
    while(false)
#else
#define STOPWATCH(name, expression) \
    expression

#define TOCK(name) ((void)0)

#define TICK(name) ((void)0)

#endif

class Stopwatch
{
    public:
        static Stopwatch & getInstance()
        {
            static Stopwatch instance;
            return instance;
        }

        void addStopwatchTiming(std::string name, unsigned long long int duration)
        {
            if(duration > 0)
            {
                timings[name] = (float)(duration) / 1000.0f;
            }
        }

        const std::map<std::string, float> & getTimings()
        {
            return timings;
        }

        static unsigned long long int getCurrentSystemTime()
        {
            timeval tv;
            gettimeofday(&tv, 0);
            unsigned long long int time = (unsigned long long int)(tv.tv_sec * 1000000 + tv.tv_usec);
            return time;
        }

        void tick(std::string name, unsigned long long int start)
        {
        	tickTimings[name] = start;
        }

        void tock(std::string name, unsigned long long int end)
        {
        	float duration = (float)(end - tickTimings[name]) / 1000.0f;

            if(duration > 0)
            {
                timings[name] = duration;
            }
        }

    private:
        Stopwatch() {}

        timeval clock;
        std::map<std::string, float> timings;                       /// durations in ms
        std::map<std::string, unsigned long long int> tickTimings;  /// start time in us
};

#endif /* STOPWATCH_H_ */
