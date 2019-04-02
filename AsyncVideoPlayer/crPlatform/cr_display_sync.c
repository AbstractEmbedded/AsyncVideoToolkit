//
//  cr_flt_mesh.c
//  crGeometry
//
//  Created by Joe Moulton on 1/4/17.
//  Copyright © 2017 Joe Moulton. All rights reserved.
//

#include "cr_display_sync.h"//"../crPlatform.h"
//#include "cr_flt_mesh.h"
#include "cr_time_utils.h"
#include <string.h>
#include <stdio.h>
//#include "cr_vsync_ext.h"
//#include "LARGE_INTEGER.h"

#include <assert.h>

//======================================================================
//======================================================================
//======================================================================
//
//  VSYNC synchronization service (single instance only)
//
//======================================================================
//======================================================================
//======================================================================

static bool g_bSuspendVsyncServer=false;	//message boolean
static bool _bVsyncServiceRunning=false;	//status boolean
//static CRITICAL_SECTION _vilock;			//lock a critical section of code using a Native windows mechanism
//static HANDLE _hVFront = NULL;				//a native windows event associated with the front buffer that can be waited on for thread synchronization
//static HANDLE _hVBack = NULL;				//a native windows event associated with the front buffer that can be waited on for thread synchronization
//static HANDLE _hVThread=NULL;				//handle to (native windows) vsync update thread
//static unsigned int _hVThreadID=0;

// last vblank time, and period stats
#define PMAX 60

void vsyncSuspend( bool );
LPSTR vsyncGetDebugMessage( LPSTR );

LARGE_INTEGER vsyncGetLast();

static CR_LARGE_INTEGER _myvsync=0;
static int _pnums[PMAX];
static int _psum=0;
static int _pnum=0;
static int _poff=0;

// debug / self monitoring performance stats
static char _vdebug[100]={0};
static CR_LARGE_INTEGER _vtfirst=0;
static CR_LARGE_INTEGER _vused=0;
static CR_LARGE_INTEGER _vsleep=0;

/*
//----------------------------------------------------------------------
void vsyncSuspend( bool bSuspend ) {
    g_bSuspendVsyncServer = bSuspend;
    Sleep(100);
    }
//----------------------------------------------------------------------
LPSTR vsyncGetDebugMessage( _Out_ LPSTR lpMsg ) {
    EnterCriticalSection(&_vilock); 
    lstrcpy( lpMsg, _vdebug ); 
    LeaveCriticalSection(&_vilock);
    return lpMsg;
    }
//----------------------------------------------------------------------
bool vsyncGetNewTimingInfo( _Out_ LARGE_INTEGER*pvblank, _Out_ LARGE_INTEGER*pperiod ) {
    bool bRet=false;
    EnterCriticalSection(&_vilock); 
    int period = _pnum ? _psum/_pnum : 0;
    if (pvblank) {
        *pvblank = _myvsync;
        }
    if (pperiod) {
        *pperiod = period;
        }
    bool bOK = _myvsync && period;
    //todo: return default 60Hz info on failure???
    LeaveCriticalSection(&_vilock);
    return bOK;
    }
//----------------------------------------------------------------------
void vsyncHaveNewTimingInfo( _In_ LARGE_INTEGER now ) {
    // TODO: These numbers are samples, and have micro-jitter.  It IS possible to make
    // a super accurate measurement (see the Hz object at www.vsynctester.com).  That will
    // not help this code to 'wake up' any more accurately, but it might be very useful
    // from a 'monitoring' standpoint.
    EnterCriticalSection(&_vilock);
    if (_myvsync) {
        _psum -= (_pnum==PMAX) ? _pnums[_poff] : 0;
        _pnums[_poff] = (int)(now-_myvsync);
        _psum += _pnums[_poff];
        _poff = (_poff+1)%PMAX;
        _pnum += (_pnum<PMAX)?1:0;
        }
    _myvsync = now;
    LeaveCriticalSection(&_vilock);
    }
//----------------------------------------------------------------------
void vsyncClearTimingInfo() {
    EnterCriticalSection(&_vilock);
    _psum = 0;
    _pnum = 0;
    _poff = 0;
    _myvsync = 0;
    LeaveCriticalSection(&_vilock);
    }
//----------------------------------------------------------------------
LARGE_INTEGER vsyncGetLast() {
    return _myvsync;
    }




//----------------------------------------------------------------------
LARGE_INTEGER vsyncAddPerformanceStats( LARGE_INTEGER now, LARGE_INTEGER cpu ) {
    static int nTimes=0;
    _vused += cpu;
    if (nTimes<10) {
        _vtfirst = now;
        _vsleep = 0;
        _vused = 0;
        }
    ++nTimes;
    return now;
    }






//----------------------------------------------------------------------
// not multi-thread safe (intended to be called at app shutdown / WinMain)
void vsyncStopService() {
    if (_bVsyncServiceRunning) {
        _bVsyncServiceRunning = false;
        WaitForSingleObject(_hVThread,5000);
        }
    }

	*/

//======================================================================
//======================================================================
//======================================================================
//
//  Dynamic function binding
//
//======================================================================
//======================================================================
//======================================================================

//----------------------------------------------------------------------
FARPROC fnBind( LPSTR pDll, LPSTR lpFnName ) 
{
    FARPROC lpfn;
	HMODULE hMod = GetModuleHandle(pDll);
    if (!hMod) 
	{
        hMod = LoadLibrary(pDll);
    }
    lpfn = hMod ? GetProcAddress(hMod, lpFnName) : NULL;
    //dout( "...fnBind(%s,%s)=%d", pDll, lpFnName, lpfn );
    //if (!lpfn) {
    //    dout( "*** Failed to bind to: %s:%s", pDll, lpFnName );
    //    }
    return lpfn;
}

HANDLE display_sync_signal_event = NULL;
HANDLE backBufferEvent = NULL;

HANDLE cr_get_display_sync_event_handle()
{
	return display_sync_signal_event;
}


unsigned int frameCount = 0;

// NOTE: waking up on an external signal is OPTIONAL
// or another way: per thread (TLS) wait object, on a wait, into queue; on notify, all in queue.
//----------------------------------------------------------------------
// Designed to that any number of callers can use at once, and wake up on VSYNC
// reason for return: vsync interrupted (true) or timeout (false)
// only use this when the display sync update is occurring from the same instance of this dll
unsigned int cr_wait_for_display_sync( unsigned long dwMilliseconds ) 
{
    return WAIT_OBJECT_0==WaitForSingleObject(display_sync_signal_event, dwMilliseconds);
		
	//if false, the semaphore was not signaled, which means a timeout occurred and the semaphore is still unsignaled 
	//if true, the semaphore was signaled, and we can allow our listeners to stop waiting
	//otherwise is still waiting/sleeping
	//return WAIT_OBJECT_0 == WaitForSingleObject( _verticalRetraceSemaphore, dwMilliseconds);           

}

//DWORD WINAPI vsyncUpdateCallback(LPVOID lpParam) 
unsigned int __stdcall cr_start_display_sync_listener(void * params)
{
    //dout( "+vsyncThread()" );

	int oldprio; //store the current thread priority when this function is called
	CR_LARGE_INTEGER cpuSum;
	cr_display_sync_event_params * sync_event_params;
	LPFND3DKMT_OPENADAPTERFROMHDC lpfnKTOpenAdapterFromHdc;
	LPFND3DKMT_WAITFORVERTICALBLANKEVENT lpfnKTWaitForVerticalBlankEvent;
	D3DKMT_OPENADAPTERFROMHDC oa;
	CR_LARGE_INTEGER t1;

	//Handle frontBufferEvent;
	//HANDLE frontBufferEvent;
	//HANDLE backBufferEvent;

	// When D3DKMT wakes up this thread, THIS CODE MUST RUN ASAP -- it runs in no time and then IMMEDIATELY right back to waiting/sleeping

	sync_event_params = (cr_display_sync_event_params*)params;
	//potentially elevate or lower thread priority based on client param
	//TO DO:  error checking on thredPriority int

	//check the current priority of the current thread we are running on 
	oldprio = GetThreadPriority( GetCurrentThread() );

	//escalate/deescalte the current thread priority for the duration of the following display sync update loop
	if( oldprio != sync_event_params->threadPriority )
		SetThreadPriority( GetCurrentThread(), sync_event_params->threadPriority );


	//load d3dkmht.h WinDDK functions directly from the system gdi32.dll user-mode dll through the system ntdll.dll user-mode dll to the system ntoskrnl.dll kernel-mode dll
    lpfnKTOpenAdapterFromHdc = (LPFND3DKMT_OPENADAPTERFROMHDC)fnBind("gdi32","D3DKMTOpenAdapterFromHdc");
    lpfnKTWaitForVerticalBlankEvent = (LPFND3DKMT_WAITFORVERTICALBLANKEVENT)fnBind("gdi32","D3DKMTWaitForVerticalBlankEvent");
    

	assert( sync_event_params->syncEvent != NULL);
	//create sync notification event if not passed by client
	if(sync_event_params->syncEvent != NULL )
		display_sync_signal_event = sync_event_params->syncEvent;
	else
		display_sync_signal_event = CreateEvent(NULL, TRUE, FALSE, NULL); //sync_event_params->syncEvent;

	//backBufferEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	tickInit();
	// get initial Win32 QueryPerformanceCounter CPU Tick Count
    t1 = tick();//= vsyncAddPerformanceStats(tick(), 0);

	cpuSum = 0;
	if (lpfnKTOpenAdapterFromHdc && lpfnKTWaitForVerticalBlankEvent) {
        D3DKMT_WAITFORVERTICALBLANKEVENT we;
        bool bBound = false;
		bool bRunning = true;
        while (bRunning) 
		{
            if (!bBound) 
			{
				// Get a Handle to the primary display monitor
				// NULL = primary display monitor; NOT tested with multiple monitor setup; tested/works with hAppWnd
                oa.hDc = GetDC(NULL);  
				
				bBound = (S_OK==(*lpfnKTOpenAdapterFromHdc)(&oa));
                if (bBound) 
				{
                    we.hAdapter = oa.hAdapter;
                    we.hDevice = 0;
                    we.VidPnSourceId = oa.VidPnSourceId;
                }
            }
            
			if(bBound)
			{
				//bool bWaited;
				 // wait for vblank on display device
				CR_LARGE_INTEGER t2 = tick();
				CR_LARGE_INTEGER cpu = t2-t1;
				 //fprintf( stderr, "\n%I64d\t%.3f ms; freq = %I64d\n", t2, tick2ms(cpu), _freq );
				//printf("\n%f ms\n", 
				//t1 = tick();
				t1 = t2; //set t1 to current time
				if(STATUS_SUCCESS==(*lpfnKTWaitForVerticalBlankEvent)(&we))
				{
					//printf("\n pulse vblank \n");
					PulseEvent(display_sync_signal_event);
				}
					//vsyncAddPerformanceStats(t1, cpu);
			}
            else 
			{
				printf("\n vysnc not bound \n");
                bBound = false;
                //dout( "*** vsync service in recovery mode..." );
                Sleep(1000);
                }
            }
    }
	else
	{
		fprintf(stderr, "\ncr_init_display_sync() : Failed to dynamically load functions through system DLLs. \n");
		return CR_DISPLAY_SYNC_DLL_FAILURE;
	}

	//reset to original thread priority
	if( oldprio != sync_event_params->threadPriority )
		SetThreadPriority( GetCurrentThread(), oldprio );

	//dout( "-vsyncThread()" );
    return CR_DISPLAY_SYNC_API_SUCCESS;
}