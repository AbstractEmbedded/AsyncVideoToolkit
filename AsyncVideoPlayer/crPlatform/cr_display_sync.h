//
// Compensate for recent Windows header files
//
#ifndef CR_DISPLAY_SYNC_H
#define CR_DISPLAY_SYNC_H

#include <windows.h>
//#include <crUtils/cr_time_utils.h>

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

#ifdef __cplusplus
extern "C" {
#endif



//inline doesn't exist in C89, __inline is MSVC specific
#ifndef CR_PLATFORM_INLINE
#ifdef _WIN32
#define CR_PLATFORM_INLINE __inline
#else
#define CR_PLATFORM_INLINE
#endif
#endif

//__decspec doesn't exist in C89, __declspec is MSVC specific
#ifndef CR_PLATFORM_DECLSPEC
#ifdef _WIN32
#define CR_PLATFORM_DECLSPEC __declspec
#else
#define CR_PLATFORM_DECLSPEC
#endif
#endif

//align functions are diffent on windows vs iOS, Linux, etc.
#ifndef CR_PLATFORM_ALIGN//(X)
#ifdef _WIN32
#define CR_PLATFORM_ALIGN(X) (align(X))
#else
#define CR_PLATFORM_ALIGN(X) __attribute__ ((aligned(X)))
#endif
#endif


//#define _In_
//#define _Out_
#define _Out_Opt_
#define _InOut_

//fyi, HMONITOR is defined in #include <dxgi.h> as well
//typedef HANDLE HMONITOR;

//#define DISPLAY_DEVICE_ACTIVE 0x1

#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif 

/*
typedef struct tagMONITORINFOEX {
    DWORD cbSize;
    RECT  rcMonitor;
    RECT  rcWork;
    DWORD dwFlags;
    TCHAR szDevice[CCHDEVICENAME];
    } MONITORINFOEX, *LPMONITORINFOEX;
#define MONITOR_DEFAULTTONEAREST 0x2
*/
//#define MONITORINFOF_PRIMARY 0x1

typedef LONG NTSTATUS;
#define STATUS_SUCCESS  ((NTSTATUS)0x00000000L)
#define STATUS_NOT_IMPLEMENTED ((NTSTATUS)0xC0000002L)

typedef UINT D3DKMT_HANDLE;
typedef UINT D3DDDI_VIDEO_PRESENT_SOURCE_ID;
typedef struct _D3DKMT_OPENADAPTERFROMHDC {
    HDC                             hDc;            // in:  DC that maps to a single display
    D3DKMT_HANDLE                   hAdapter;       // out: adapter handle
    LUID                            AdapterLuid;    // out: adapter LUID
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;  // out: VidPN source ID for that particular display
    } D3DKMT_OPENADAPTERFROMHDC;
typedef struct _D3DKMT_CLOSEADAPTER {
    D3DKMT_HANDLE hAdapter;
    } D3DKMT_CLOSEADAPTER;
typedef struct _D3DKMT_WAITFORVERTICALBLANKEVENT {
    D3DKMT_HANDLE                   hAdapter;      // in: adapter handle
    D3DKMT_HANDLE                   hDevice;       // in: device handle [Optional]
    D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId; // in: adapter's VidPN Source ID
    } D3DKMT_WAITFORVERTICALBLANKEVENT;
typedef NTSTATUS (APIENTRY *LPFND3DKMT_OPENADAPTERFROMHDC)(D3DKMT_OPENADAPTERFROMHDC*);
typedef NTSTATUS (APIENTRY *LPFND3DKMT_CLOSEADAPTER)(D3DKMT_CLOSEADAPTER*);
typedef NTSTATUS (APIENTRY *LPFND3DKMT_WAITFORVERTICALBLANKEVENT)(D3DKMT_WAITFORVERTICALBLANKEVENT*);



#pragma pack(push)
#pragma pack(4)

//----------------------------------------------------------------------
// https://msdn.microsoft.com/en-us/library/windows/desktop/aa969503(v=vs.85).aspx
//typedef struct _DWM_TIMING_INFO {
//  UINT32          cbSize;              // 0:1
//  UNSIGNED_RATIO  rateRefresh;         // 1:2
//  QPC_TIME        qpcRefreshPeriod;    // 3:2 *** offset 3
//  UNSIGNED_RATIO  rateCompose;         // 5:2
//  QPC_TIME        qpcVBlank;           // 7:2 *** offset 7
//   qpc2tick(*(LARGE_INTEGER*)(aTI+7));    // qpcVBlank
//   *(OSTICK*)(aTI+3);                    // qpcRefreshPeriod

typedef ULONGLONG DWM_FRAME_COUNT;
typedef ULONGLONG QPC_TIME;

typedef struct _UNSIGNED_RATIO {
    UINT32 uiNumerator;
    UINT32 uiDenominator;
    } UNSIGNED_RATIO;

typedef struct _DWM_TIMING_INFO {
    UINT32 cbSize;
    UNSIGNED_RATIO rateRefresh;
    QPC_TIME qpcRefreshPeriod;
    UNSIGNED_RATIO rateCompose;
    QPC_TIME qpcVBlank;
    DWM_FRAME_COUNT cRefresh;
    UINT cDXRefresh;
    QPC_TIME qpcCompose;
    DWM_FRAME_COUNT cFrame;
    UINT cDXPresent;
    DWM_FRAME_COUNT cRefreshFrame;
    DWM_FRAME_COUNT cFrameSubmitted;
    UINT cDXPresentSubmitted;
    DWM_FRAME_COUNT cFrameConfirmed;
    UINT cDXPresentConfirmed;
    DWM_FRAME_COUNT cRefreshConfirmed;
    UINT cDXRefreshConfirmed;
    DWM_FRAME_COUNT cFramesLate;
    UINT cFramesOutstanding;
    DWM_FRAME_COUNT cFrameDisplayed;
    QPC_TIME qpcFrameDisplayed;
    DWM_FRAME_COUNT cRefreshFrameDisplayed;
    DWM_FRAME_COUNT cFrameComplete;
    QPC_TIME qpcFrameComplete;
    DWM_FRAME_COUNT cFramePending;
    QPC_TIME qpcFramePending;
    DWM_FRAME_COUNT cFramesDisplayed;
    DWM_FRAME_COUNT cFramesComplete;
    DWM_FRAME_COUNT cFramesPending;
    DWM_FRAME_COUNT cFramesAvailable;
    DWM_FRAME_COUNT cFramesDropped;
    DWM_FRAME_COUNT cFramesMissed;
    DWM_FRAME_COUNT cRefreshNextDisplayed;
    DWM_FRAME_COUNT cRefreshNextPresented;
    DWM_FRAME_COUNT cRefreshesDisplayed;
    DWM_FRAME_COUNT cRefreshesPresented;
    DWM_FRAME_COUNT cRefreshStarted;
    ULONGLONG cPixelsReceived;
    ULONGLONG cPixelsDrawn;
    DWM_FRAME_COUNT cBuffersEmpty;
    } DWM_TIMING_INFO;
#pragma pack(pop)

typedef enum CR_DISPLAY_SYNC_API_STATUS
{
	CR_DISPLAY_SYNC_API_SUCCESS					= 0,
	CR_DISPLAY_SYNC_API_FAILURE					= 1,  //general failure
	CR_DISPLAY_SYNC_DLL_FAILURE					= 2	  //failed to load functions dynamically through system dlls

}CR_DISPLAY_SYNC_API_STATUS;

typedef struct cr_display_sync_event_params
{
	int threadPriority;			//allow the blocking display sync loop to update priority of current thread for the duration and return priority to its starting state when finished (e.g. 
	HANDLE syncEvent;			//pass your own event created with CreateEvent, so that this handle may be passed on to be cr_init_display_sync_event inherited and used within a child process if desired
								//otherwise, if you only want a threaded event from the context of your applicaiton process, just pass NULL for the event and observe cr_get_display_sync_event_handle to get handle to the internal event

}cr_display_sync_event_params;

HANDLE cr_get_display_sync_event_handle();
unsigned int __stdcall cr_start_display_sync_listener(void * params);

unsigned int cr_wait_for_display_sync( unsigned long dwMilliseconds );

//CR_LARGE_INTEGER vsyncAddPerformanceStats( CR_LARGE_INTEGER now, CR_LARGE_INTEGER cpu );
//bool vsyncSleepWithVsyncInterrupt(DWORD);
//bool vsyncGetNewTimingInfo( CR_LARGE_INTEGER*pvblank, CR_LARGE_INTEGER*pperiod );
void vsyncStartService();
void vsyncStopService();


#ifdef __cplusplus
}
#endif


#endif //CR_VSYNC_EXT_H

