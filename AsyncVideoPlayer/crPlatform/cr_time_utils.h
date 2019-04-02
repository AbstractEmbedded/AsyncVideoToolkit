
#ifndef CR_TIME_UTILS_H
#define CR_TIME_UTILS_H

#ifndef CR_UTILS_INLINE
#ifdef _WIN32
#define CR_UTILS_INLINE __inline
#else
#define CR_UTILS_INLINE
#endif
#endif

//HEADER PORTION
#ifdef _WIN32
#include <SYS\STAT.H>
#include <direct.h>
#ifndef STRICT
#define STRICT // for windows.h
#endif
#include <windows.h>
#else // LINUX OR DARWIN
#include <unistd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
#define _MAX_PATH NAME_MAX
#define _access access 
#define __stat64 stat64
#define _getcwd getcwd
#define _chdir chdir
#define _stat64 stat64
#define _ctime64 ctime

/* includes for Linux version of QueryPerformanceCounter timer */
#include <stdint.h>
#include <stdbool.h>


#endif // _WIN32

#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif


//only define bool if not previoulsy defined by CoreRender crWindow global header
//and not using C++
#ifndef __cplusplus
//typedef unsigned char bool;
#ifndef bool
#define bool int
#define true 1
#define false 0
#endif
#endif

//IMPLEMENTATION PORTION

static bool _timingInit = false;
static bool _frameRate = true;		//indicates whether to print frame rate when capturing timing

#ifdef _WIN32
typedef __int64 CR_LARGE_INTEGER;
static LARGE_INTEGER _frequency; 
static LARGE_INTEGER _start, _end;
#else
 typedef struct //use same syntax in linux as win32
 {
	int64_t QuadPart;
 } CR_LARGE_INTEGER;
 
static CR_LARGE_INTEGER _frequency; 
static CR_LARGE_INTEGER _start, _end;
#endif

        static float aveFrameRate = 0;
        static float curAveFrameRate = 0;
        static float elapsedTime = 0;
        static float curMinFrameRate = 0;
        static float minFrameRate = 0;
        static int elapsedFrames = 0;

static void CR_UTILS_INLINE crFramerate()
{

	float _time;

	if( !_timingInit )
	{
		//printf("crFrameRate\n");
#ifdef _WIN32
        QueryPerformanceFrequency(&_frequency);//linux version now working (in platform.h)
        QueryPerformanceCounter(&_start);
#else
        /* gettimeofday reports to microsecond accuracy */
        _frequency.QuadPart = 1000000;//usec_per_sec;
        
        struct timeval timeInit;
        
        /* Grab the current time. */
        gettimeofday(&timeInit, NULL);
        _start.QuadPart = timeInit.tv_usec + timeInit.tv_sec * 1000000;
        
#endif 
		_timingInit = true;
		//printf("crFrameRate2\n");
	}



#ifdef _WIN32
        QueryPerformanceCounter(&_end);
#else
        struct timeval time;
        
        /* Grab the current time. */
        gettimeofday(&time, NULL);
        _end.QuadPart = time.tv_usec + time.tv_sec * 1000000;
#endif

        _time = (float)(_end.QuadPart - _start.QuadPart);
        _time /= _frequency.QuadPart;

        if(elapsedTime > 1) 
        {
            aveFrameRate += 1/_time;
            aveFrameRate /= (elapsedFrames+1);
            curAveFrameRate = aveFrameRate;
            curMinFrameRate = minFrameRate;
            minFrameRate = 100000;
            elapsedFrames = 0;
            elapsedTime = 0;
            aveFrameRate = 0;

			//int currentWidth = _sensim->getWidth();
            //int currentHeight = _sensim->getHeight();
			//printf("before print\n");
            if (_frameRate)
            {

                //char tempChars[100];
                printf("Avg frame rate - %.2f Hz Min - %.2f Hz\n", curAveFrameRate, curMinFrameRate/*, currentWidth, currentHeight*/);
                //std::cout << tempChars;
            }
        } 
		else
        {
			//printf("elapsed time <= 1\n");

            aveFrameRate += 1/_time;
            elapsedFrames++;
            elapsedTime += _time;
            if ((1/_time < minFrameRate) || (minFrameRate == 0))
                minFrameRate = 1/_time;
        }

#ifdef _WIN32
        QueryPerformanceCounter(&_start);
#else
        /* Grab the current time. */
        gettimeofday(&time, NULL);
        _start.QuadPart = time.tv_usec + time.tv_sec * 1000000;
#endif
}


static void CR_UTILS_INLINE crTime()
{
	float _time;

	if( !_timingInit )
	{
		//printf("crGetTime\n");
#ifdef _WIN32
        QueryPerformanceFrequency(&_frequency);//linux version now working (in platform.h)
        QueryPerformanceCounter(&_start);
#else
        /* gettimeofday reports to microsecond accuracy */
        _frequency.QuadPart = 1000000;//usec_per_sec;
        
        struct timeval timeInit;
        
        /* Grab the current time. */
        gettimeofday(&timeInit, NULL);
        _start.QuadPart = timeInit.tv_usec + timeInit.tv_sec * 1000000;
#endif 
		_timingInit = true;
		//printf("crGetTIme initialized\n");
	}



#ifdef _WIN32
        QueryPerformanceCounter(&_end);
#else
        struct timeval time;
        
        /* Grab the current time. */
        gettimeofday(&time, NULL);
        _end.QuadPart = time.tv_usec + time.tv_sec * 1000000;
#endif

        _time = (float)(_end.QuadPart - _start.QuadPart);

		//printf("time:	%f\n", _time);
		//printf("quadPart:	%f", (float)_frequency.QuadPart);
        _time /= _frequency.QuadPart;

		printf("time:	%f\n", _time);

        if(elapsedTime > 1) 
        {
			//printf("elapsed time > 1\n");

            aveFrameRate += 1/_time;
            aveFrameRate /= (elapsedFrames+1);
            curAveFrameRate = aveFrameRate;
            curMinFrameRate = minFrameRate;
            minFrameRate = 100000;
            elapsedFrames = 0;
            elapsedTime = 0;
            aveFrameRate = 0;
        } 
		else
        {
			//printf("elapsed time <= 1\n");

            aveFrameRate += 1/_time;
            elapsedFrames++;
            elapsedTime += _time;
            if ((1/_time < minFrameRate) || (minFrameRate == 0))
                minFrameRate = 1/_time;

			printf("elapsedTime = %f\n", elapsedTime);
        }

#ifdef _WIN32
        QueryPerformanceCounter(&_start);
#else
        /* Grab the current time. */
        gettimeofday(&time, NULL);
        _start.QuadPart = time.tv_usec + time.tv_sec * 1000000;
    
#endif
}


// NOTE: we don't care about math overflow conditions, since this is rapid
// prototyping code only meant to run for minutes
#ifdef _WIN32
static __int64 _first;
static __int64 _freq;

//----------------------------------------------------------------------
// It is a curious note that Chrome tests to see if the processor supports
// invariant TSC before using Windows QPC, but then FAILS to verify that
// Windows QPC is then actually using invariant TSC as the time source!
// The QPC frequency is a dead giveaway.  There are well known values
// that indicate non-TSC -- that are MUCH slower than TSC
static void CR_UTILS_INLINE tickInit() {
    LARGE_INTEGER li;
    QueryPerformanceCounter( &li );
    _first = li.QuadPart;
    QueryPerformanceFrequency( &li );
    _freq = li.QuadPart;
    }
//----------------------------------------------------------------------
static CR_LARGE_INTEGER CR_UTILS_INLINE qpc2tick( ULONGLONG qpc ) {
    return qpc-_first;
    }
//----------------------------------------------------------------------
static CR_LARGE_INTEGER CR_UTILS_INLINE tick() {
    LARGE_INTEGER qpc;
    QueryPerformanceCounter( &qpc );
    return qpc2tick( *(ULONGLONG*)&qpc );
    }
//----------------------------------------------------------------------
static double CR_UTILS_INLINE tick2us( CR_LARGE_INTEGER t ) {
    return t*1000000.0/_freq;
    }
//----------------------------------------------------------------------
static double CR_UTILS_INLINE tick2ms( CR_LARGE_INTEGER t ) {
    return tick2us(t)/1000;
    }
//----------------------------------------------------------------------
static double CR_UTILS_INLINE tick2sec( CR_LARGE_INTEGER t ) {
    return tick2us(t)/1000000;
    }
#endif

#ifdef __cplusplus
}
#endif


#endif //CR_TIME_UTILS_H
