#include "AsyncVideoPlayer.h"


void setThreadPriority(DWORD priorityLevel)
{
	HANDLE thread = GetCurrentThread();

	SetThreadPriority(thread, priorityLevel);
}

void setProcessPriority(DWORD priorityClass)
{
	HANDLE process = GetCurrentProcess();
	//DWORD mainThreadProcessID = GetCurrentProcessId();
	
	SetPriorityClass(process, priorityClass);
}

void ResizeWin32Window(HWND& hMain, int clientAreaWidth, int clientAreaHeight, bool hasTitleBar, bool hasMenu)
{
  RECT winRect;
  RECT clRect;
  GetWindowRect(hMain, &winRect);
  GetClientRect(hMain, &clRect);

  long borderWidth = (winRect.right - winRect.left) - clRect.right;

  RECT adjustedWindowRect ={0, 0, clientAreaWidth, clientAreaHeight};

  //get the window style
  DWORD dwStyle = GetWindowLong(glWindow.hwnd, GWL_STYLE);
					//dwStyle &= ~WS_CAPTION;
					//dwStyle &= ~WS_OVERLAPPEDWINDOW;
					//dwStyle |= WS_POPUP;
					//SetWindowLong(glWindow.hwnd, GWL_STYLE, dwStyle);

  //must set menu bool to true to account for that height
  if( hasTitleBar || hasMenu )
	  AdjustWindowRectEx(&adjustedWindowRect, dwStyle, hasMenu, NULL);

  MoveWindow(hMain, winRect.left, winRect.top, adjustedWindowRect.right-adjustedWindowRect.left, adjustedWindowRect.bottom-adjustedWindowRect.top, TRUE);
  return;
}

void MoveWin32Window(HWND &hMain, int x, int y)
{
	RECT winRect;
	RECT clRect;
	GetWindowRect(hMain, &winRect);
	GetClientRect(hMain, &clRect);


	//modify the window style to add back the title bar and switch back to a WS_OVERLAPPEDWINDOW style instead of WS_POPUP (BORDERLESS)
	DWORD dwStyle = GetWindowLong(glWindow.hwnd, GWL_STYLE);
					  
	//calculate the border width if the window is a WINDOWED style
	long borderWidth = 0;  
					
	if( dwStyle & WS_OVERLAPPEDWINDOW )
	{
		borderWidth = (winRect.right - winRect.left) - clRect.right;
		glWindow.borderWidth = borderWidth;
	}

	SetWindowPos(glWindow.hwnd, NULL, x - borderWidth/2, y, winRect.right - winRect.left, winRect.bottom - winRect.top, SWP_FRAMECHANGED | SWP_NOZORDER);

}


static void __processLoadTextureThreadMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch (uMsg)
	{
		//if the GPUGPU thread receives this message, then the render thread is telling
		//us that it finished initialzing its opengl context
	case AVP_THREAD_CLOSE:
	{
		fprintf(stdout, "\nasyncLoadTextureThread AVP_THREAD_CLOSE");
		
		// notification that we should stop the render thread from executing and close the view
		//_textureThreadShoudClose = true;
		// kill this thread and its resources (CRT allocates them)
		_endthreadex(0);
		break;
	}

	/*
	case AVP_VIDEO_CONTROL_EVENT:
	{
		printf("\nloadTextureThread AVP_CONTROL_EVENT\n");
		switch (wParam)
		{
			case AVP_VIDEO_CONTROL_LOAD:
			{
				
			}
		}

		break;
	}
	*/

	default:
		break;

	}

	return;
}

static bool _videoPaused = true;

unsigned int __stdcall asyncLoadTextureCallback(void * view)
{
	//this thread will only exist for the duration that a video is loaded for playback

	//for retrieving messages sent to this thread using PeekMessage
	MSG msg;
	
	//for loading uncompressed video file data from mapped memory to pbo persistent mapped buffer memory 
	unsigned int loadPBOIndex = 0;
	static unsigned long frameIndex = 0;
	//TO DO:  move this to an apllication managed property
	size_t FRAME_SIZE = (size_t)g_sourceVideo.width * (size_t)g_sourceVideo.height * 4;

	//escalate thread priority as high as we can without using REALTIME thread priority as an admin because it could lock up other system processes
	//though we could if use it if we wanted to and there are some legal use cases when strictly making kernel mode system calls
	setThreadPriority(THREAD_PRIORITY_TIME_CRITICAL);

	//create an event that we can idle threads on when the video control state is paused
	_videoIdleEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	DWORD dwRet;
	//start the loop that reads frames from mapped video file memory buffer and copies into pbo buffer memory
	//we can do this on a thread without and OpenGL Context because we are using persistent mapped buffer storage (OpenGL 4.4) for the PBOs
	while (1)
	{
		int32_t availableReadBytes = 0;
		avt_circ_buffer_tail(&_loadTextureSync, &availableReadBytes);
		if (availableReadBytes / 4 < NUM_CACHE_FRAMES)
		{
			//advance to the next pixel buffer in our array of pbos
			//so we can load the video image data from mapped memory to the pbo buffer
			loadPBOIndex = (loadPBOIndex + 1) % _numPBOs;

			//this is the call that copies video mapped memory buffer 
			//the peristent mapped pbo memory buffer, one frame at a time
			loadTextureToPBO(loadPBOIndex, frameIndex, FRAME_SIZE);

			//put the pbo index in the buffer, so the OpenGL upload pbo to texture thread (ie asyncGPGPUCallback) can grab it and use it
			//to upload the pbo buffer to an OpenGL texture resident in GPU memory for access/use by the vertex/fragment shaders when rendering
			avt_circ_buffer_produce_bytes(&_loadTextureSync, &loadPBOIndex, 4);

			//advance the frame index so the offset to the next frame into the mapped memory buffer
			//can be correctly calculated on the next pass
			frameIndex++;
			frameIndex = (frameIndex) % g_sourceVideo.numFrames;
		}

		//if the video control state is paused, we should idle the thread to stop producing
		//while we can wake up to listen for thread messages that change the video control state
		while (g_videoEventState == VIDEO_EVENT_STATE_PAUSED)
		{
			//MsgWaitForMultipleObjectsEx... is handy, it will wait for multiple objects such as events to be signaled
			//but will also stop idling when any messages are sent to the thread so that we can wake up
			//and process them
			//idle until we receive a message, then process the message...
			dwRet = MsgWaitForMultipleObjectsEx(1, &_videoIdleEvent, INFINITE, QS_ALLINPUT, 0);

			/*
			if (dwRet == (WAIT_OBJECT_0 + 1))
			{
				while (PeekMessage(&msg, NULL, AVP_THREAD_MESSAGE_FIRST, AVP_THREAD_MESSAGE_LAST, PM_REMOVE))
				{
					__processLoadTextureThreadMessage(msg.message, msg.wParam, msg.lParam);
					memset(&msg, 0, sizeof(MSG));
				}

			}
			*/
		}


		// NOTE: it is imperative that PeekMessage() is used rather than GetMessage() when on background render thread
		// listen to the queue in case this thread received a message from the event queue on another thread
		//listen after command buffer has been passed to opengl for the current frame, if we read before this causes dropped frames on resize
		//but it also means events won't be processed until the following frame, which in most cases is generally ok
		//passing NULL for second parameter ensures that messages for the platform window AND non-window thread messages will be processed
		//maximum of 10000 messages per queue
		while (PeekMessage(&msg, NULL, AVP_THREAD_MESSAGE_FIRST, AVP_THREAD_MESSAGE_LAST, PM_REMOVE))
		{
			__processLoadTextureThreadMessage(msg.message, msg.wParam, msg.lParam);
			memset(&msg, 0, sizeof(MSG));
		}

	}

	// kill this thread and its resources (CRT allocates them)
	_endthreadex(0);
	return 0;
}


void setVideoEventState(AVP_VIDEO_EVENT_STATE state)
{
	//only notify threads of state update when the state has changed
	if (state != g_videoEventState)
	{
		g_videoEventState = state;

		//SendMessage(gpgpuWindow.hwnd, AVP_WINDOW_CLOSE, 0, 0);
		//send an async message to the asyncLoadTextureCallback thread to wake it up if its paused or idling
		WPARAM controlEvent = AVP_VIDEO_CONTROL_PLAY;
		PostThreadMessage(gpgpuWindow.loadThreadID, AVP_VIDEO_CONTROL_EVENT, controlEvent, NULL);
	}
}

static void __processGPGPUThreadMessage (UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int w,h;
	int cursorArea;				//the cursor area returned by WM_NCHITTEST and used by WM_SETCURSOR
	HCURSOR hCursor;			//a custom cursor loaded for the view
	POINT mouse_screen_coord;	//a POINT struct for mouse coordinates captured in screen coordinates
	POINT mouse_client_coord;	//a POINT struct for mouse coordinates converted to client rect coordinates from screen coordinates
	RECT clientRect;			//a RECT struct for window coordinates/size
	RECT windowRect;			//a RECT struct describing the size of the window client area and the outer non-client area
	int borderWidth;			//the width of the border in pixels:  TO DO: determine how DPI affects this
	HDWP hdwp;

	switch(uMsg)
    {
        //case UWM_PAUSE:
            // notification that we should either pause or resume rendering
        //    _bPaused = (bool)wParam;
        //    break;

		//if the GPUGPU thread receives this message, then the render thread is telling
		//us that it finished initialzing its opengl context
        case AVP_WINDOW_CLOSE:
		{
			fprintf(stdout, "\nrgpgpu ender thread AVP_WINDOW_CLOSE\n");
            // notification that we should stop the render thread from executing and close the view
			gpgpuWindow.shouldClose = true;
			break;
		}
		case AVP_VIDEO_CONTROL_EVENT:
		{
			printf("\nAVP_CONTROL_EVENT\n");
			switch(wParam)
			{	
				case AVP_VIDEO_CONTROL_LOAD:
					{
						//if a video is already playing back, then the asyncLoadTextureCallback needs
						//to be killed and the memory mapped file needs to be cleaned up
						if( gpgpuWindow.loadThreadID )
						{
							//message the gpgpu thread to load the video using the metadata
							//WPARAM controlEvent = AVP_VIDEO_CONTROL_LOAD;
							PostThreadMessage(gpgpuWindow.loadThreadID, AVP_THREAD_CLOSE, NULL, NULL);
							//PostThreadMessage(gpgpuWindow.loadThreadID, )

							//unmap the video file memory
							if (_videoMapPtr)
								UnmapViewOfFile(_videoMapPtr);
							_videoMapPtr = NULL;

							//clear the circular synchronization buffers
							avt_circ_buffer_clear(&_loadTextureSync);
							avt_circ_buffer_clear(&_renderTextureSync);
						
							//set thread id to null so we can recreate it
							gpgpuWindow.loadThreadID = 0;
						}

						/***  LOAD THE VIDEO FILE FOR STREAMING PLAYBACK ***/

						//set the video state to buffering to indicate we are loading but not ready for playback
						setVideoEventState(VIDEO_EVENT_STATE_BUFFERING);
						_videoFrame = 0; //destroy the render threads reference to the last frame that it will use to render when in paused state, since we are loading a new video file 
						
						//we have received the message to load the on the gpgpu processing thread
						//1)  Open the file as memory mapped store so we can read the file with speed
						//as if it were a buffer in ram
						size_t textureSize = (size_t)g_sourceVideo.width * (size_t)g_sourceVideo.height * 4;
						createMappedFile((char*)g_sourceVideo.sourcepath, textureSize);
						//2) however, the data from disk must be read into virtual memory page stores first
						//so we read over the buffer to "touch" each virtual memory page before
						touchMappedData();

						//setting the video event state to paused
						//will start the asyncLoadTextureCallback to load a single frame to be rendered
						//to signify to the user buffering has ended and then idle the thread
						setVideoEventState(VIDEO_EVENT_STATE_PAUSED);

						gpgpuWindow.loadThreadID = 0;
						_beginthreadex(NULL, 0, asyncLoadTextureCallback, 0, 0, &(gpgpuWindow.loadThreadID));	

						//tell this thread not to idle anymore, so it can pull pbo indices from the circ buffer
						//and upload those pbo buffers as resident textures to gpu memory
						gpgpuWindow.shouldIdle = false;
					}
			}

			break;
		}
		/*
		case AVP_WINDOW_RESIZE:

			//if( glWindow.resize )
			//{
				RECT rcClient;
				RECT rcWindow;
				// set-up the perspective screen to be the size of the client area
				// to avoid clipping, use the client area size, not the window size
				GetClientRect(gpgpuWindow.hwnd, &rcClient);
				GetWindowRect(gpgpuWindow.hwnd, &rcWindow);

				gpgpuWindow.width = rcClient.right - rcClient.left;// w;
				gpgpuWindow.height = rcClient.bottom - rcClient.top;// h;
				break;
		case AVP_WINDOW_GRAPHICS_CONTEXT_INITIALIZED:
			break;
		case AVP_WINDOW_SET_PROCESS_STATE:
		{
			
			
			break;
		}
		*/
		default:
			break;
            
    }
}


void createTextureCache(unsigned int numCacheFrames)
{
	//glGenTextures(numCacheFrames, _videoTextureCache);

	//allocate storage for textures in cache of size numCachedTextures
	for( int cacheFrameIndex= 0; cacheFrameIndex<numCacheFrames; cacheFrameIndex++)
	{
		glGenTextures(1, &(_videoTextureCache[cacheFrameIndex]));
		glBindTexture(GL_TEXTURE_2D, _videoTextureCache[cacheFrameIndex]); 
		glTexStorage2D( GL_TEXTURE_2D, 1, GL_RGBA8, (GLsizei)gpgpuWindow.width, (GLsizei)gpgpuWindow.height);
		glBindTexture(GL_TEXTURE_2D,0);

		//loadVideoTexture(cacheFrameIndex, &img, (char*)videoTexturePath, glWindow.width, glWindow.height);

	}


}


void unmapFile(HANDLE file, HANDLE fileMap, const unsigned char * fileMapBuffer)
{
	UnmapViewOfFile(fileMapBuffer);
	CloseHandle(fileMap);
	CloseHandle(file);

}

	/*
		// lock the buffer:
		if( gpusync[0])
			glDeleteSync(gpusync[0]);
		gpusync[0] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		*/

		//glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
		/*
		if( gpusync[0] )
		{
			GLenum waitReturn = GL_UNSIGNALED;
			while (waitReturn != GL_ALREADY_SIGNALED && waitReturn != GL_CONDITION_SATISFIED)
			{
				waitReturn = glClientWaitSync(gpusync[0], GL_SYNC_FLUSH_COMMANDS_BIT, 10000000);
			}
			//printf("\n wait return \n");

		}
		*/



unsigned int __stdcall asyncGPGPUCallback (void * view)
{
	//Timing definitions
	CR_LARGE_INTEGER t1,t2;//= tick();

	//GLsync gpusync[_numPBOs];
	GLenum ret;
	
	GLint64 startTime, stopTime;
	unsigned int queryID[2];

	DWORD dwFPSCurrent , dwFPSLast, dwFPSLastFrameSync;  // used to help calc the fps
    unsigned long nFPS;               // current framerate/FPS for the main loop

	
    static double dLastTime, dCurTime;  // used to calc CPU cycles during a render
    static double dElapsed;                 // used to calc CPU cycles during a render
	//End Timing definitions

	//define a single buffer for loading textures
	//AVT_IMAGE img = {0};							//create a local image object to load the image to the cpu
	
	MSG msg;
	DWORD dwRet;

	//escalate thread priority
	setThreadPriority(THREAD_PRIORITY_TIME_CRITICAL);

	//int currentThreadPriority = GetThreadPriority(currentThread);
	//printf("\nGPGPU Thread Priority = %d,\n", currentThreadPriority);

	//we've moved the gpgpu window opengl context created by the glWindow render thread to this thread
	//we've made sure it is not current on that thread, let's make sure it is now current on this thread
	wglMakeCurrent(GetDC(gpgpuWindow.hwnd), gpgpuWindow.hglrc);

	//wglSwapIntervalEXT(1);
	//LPARAM lParam = MAKELPARAM(glWindow.width, glWindow.height);
	//SendMessage(glWindow.hwnd, WM_SIZE, 0, lParam);
	//hack to reset the window with current size because MS Windows does not offset it by the application title bar when initialized
	//RECT windowRect;
	//GetWindowRect(gpgpuWindow.hwnd, &windowRect);
	//SetWindowPos(gpgpuWindow.hwnd, NULL, windowRect.left, windowRect.top, windowRect.right - windowRect.left +1,windowRect.bottom-windowRect.top+1, /*SWP_ASYNCWINDOWPOS |*/ SWP_FRAMECHANGED);// SWP_NOREDRAW);
	//ResizeWin32Window(gpgpuWindow.hwnd, gpgpuWindow.width, gpgpuWindow.height);

	//create a Win32 event that will allow us to idle this thread
	//and wake it only when we send messages to its queue
	_gpgpuIdleEvent = CreateEvent(NULL, TRUE, FALSE, NULL);


	//fence sync for knowing when async pbo uploads have finished
	// to prevent CPU stalls (so the CPU doesn't pass a command buffer to the GPU while it is still operating on one)
	//GLsync gpusync;

	//TO DO: check if PBO ARB extensions are present
	createPBOs(gpgpuWindow.width, gpgpuWindow.height, 4, 8);

	const char * videofilebasepath = "..\\..\\videos\\";

	//initialize timing
	//tickInit();
	//t1 = t2 = tick();

	static int texturePBOIndex = 0;
    //int readPBOIndex = 0;                  // pbo index used for next frame
	//int loadPBOIndex = 0;
	//int loadBufferIndex = 0;

	createTextureCache((unsigned int)NUM_CACHE_FRAMES);
	
	static const unsigned long FRAME_SIZE = gpgpuWindow.width * gpgpuWindow.height * 4;

	//gpusync[0] = 0;

	GLuint videoFrame = 0;
	int textureCacheIndex = -1;

	//vertical sync for this thread should always be turned off to process as fast as possible
	wglSwapIntervalEXT(0);

	//start the display event loop
	while(!gpgpuWindow.shouldClose)
	{

		//idle until we receive the message to load a video for playback
		if( gpgpuWindow.shouldIdle )
		{
			//MsgWaitForMultipleObjectsEx... is handy, it will wait for multiple objects such as events to be signaled
			//but will also stop idling when any messages are sent to the thread so that we can wake up
			//and process them
			//idle until we receive a message
			dwRet = MsgWaitForMultipleObjectsEx(1, &_gpgpuIdleEvent, INFINITE, QS_ALLINPUT, 0);
			if (dwRet == (WAIT_OBJECT_0+1))
			{
				while(PeekMessage(&msg, NULL, AVP_WINDOW_MESSAGE_FIRST, AVP_WINDOW_MESSAGE_LAST, PM_REMOVE))
				{
					__processGPGPUThreadMessage(msg.message, msg.wParam, msg.lParam); 
					memset(&msg, 0, sizeof(MSG));
				}

			}
		}
		else
		{
		
			//check how many bytes are in the buffer
			//avt_circ_buffer_tail(&_renderTextureSync, &availableBytes );

			//check how many bytes are available for reading from the circular
			//buffer populated with pbo indices by the asyncLoadTextureCallback thread
			int32_t availableReadBytes = 0;
			unsigned int * pboIndexPtr = (unsigned int*)avt_circ_buffer_tail(&_renderTextureSync, &availableReadBytes);

			if( availableReadBytes < (NUM_FRAMES_TO_BUFFER * 4))
			{
				
				
				unsigned int * pboIndexPtr = (unsigned int*)avt_circ_buffer_tail(&_loadTextureSync, &availableReadBytes);

				if (availableReadBytes > (NUM_FRAMES_TO_BUFFER * 4) - 1)
				{
					//async transfer pbo to gpu texture memory
					//we will store the index into the texture cache and pass it to the render thread on the next frame
					//to ensure enough time for the pbo to finish async upload to texture
					textureCacheIndex = (textureCacheIndex + 1) % NUM_CACHE_FRAMES;
					readTextureFromPBO(_videoTextureCache[textureCacheIndex], *pboIndexPtr);

					//WaitForSingleObject(g_display.verticalRetraceEvent, 5);

					//must call SwapBuffers to synchronize the pbo texture upload
					//I have been unable to achieve non fragmented playback when removing this call
					//to utilize the GPU at 100%, but performance is almost exactly the same anyway
					SwapBuffers(gpgpuWindow.hdc);

					//drain the pboIndex we just used from the loadTextureSync circ buffer
					avt_circ_buffer_consume(&_loadTextureSync, sizeof(unsigned int));

					//pass the texture loaded from pbo on the last loop to the render thread
					avt_circ_buffer_produce_bytes(&_renderTextureSync, &(_videoTextureCache[(textureCacheIndex)]), 4);
				}
			}
		} //end available bytes in circ buffer
		

		// NOTE: it is imperative that PeekMessage() is used rather than GetMessage() when on background render thread
		// listen to the queue in case this thread received a message from the event queue on another thread
		//listen after command buffer has been passed to opengl for the current frame, if we read before this causes dropped frames on resize
		//but it also means events won't be processed until the following frame, which in most cases is generally ok
		//passing NULL for second parameter ensures that messages for the platform window AND non-window thread messages will be processed
		//maximum of 10000 messages per queue
		while(PeekMessage(&msg, NULL, AVP_WINDOW_MESSAGE_FIRST, AVP_WINDOW_MESSAGE_LAST, PM_REMOVE))
		{
			__processGPGPUThreadMessage(msg.message, msg.wParam, msg.lParam); 
			memset(&msg, 0, sizeof(MSG));
		}
		
		//wait in idle state until this thread is told to do something
		//WaitForSingleObject(_gpgpuIdleEvent, INFINITE);
	}

	//closeFile();
	//unmapFile( _videoFile, _videoFileMap, _videoMapView);

	//clean up the graphics context
	//make sure the context we are releasing isn't current on any threads
	wglMakeCurrent(gpgpuWindow.hdc, NULL);		// release device context in use by rc
	wglDeleteContext(gpgpuWindow.hglrc);		// delete rendering context


	//unmap the video file memory
	if( _videoMapPtr )
		UnmapViewOfFile(_videoMapPtr);
	_videoMapPtr = NULL;

	//free image loading buffer
	//avt_free_image(&img);

	// message back to the platform window control thread (i.e the thread that created the platform window) 
	// that we have cleaned up the graphics context and we are ready to close the platform window
	if( gpgpuWindow.shouldClose )
	{
		/*exitStatus =*/ SendMessage(gpgpuWindow.hwnd, AVP_WINDOW_CLOSE, 0,0);
	}

	// kill this thread and its resources (CRT allocates them)
    _endthreadex(0);

	return 0;

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*/
/ / void
/ /        ThreadProc (UINT uMsg, WPARAM wParam, LPARAM lParam)
/ /
/ /        uMsg = message to process
/ /        wParam = word sized param who's value depends on the message
/ /        lParam = long sized param who's value depends on the message
/ /
/ / PURPOSE:
/ /        Procedure to handle messages received for a view on a render thread from other threads
/*/




static void __processRenderThreadMessage (UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int w,h;
	int cursorArea;				//the cursor area returned by WM_NCHITTEST and used by WM_SETCURSOR
	HCURSOR hCursor;			//a custom cursor loaded for the view
	POINT mouse_screen_coord;	//a POINT struct for mouse coordinates captured in screen coordinates
	POINT mouse_client_coord;	//a POINT struct for mouse coordinates converted to client rect coordinates from screen coordinates
	RECT clientRect;			//a RECT struct for window coordinates/size
	RECT windowRect;			//a RECT struct describing the size of the window client area and the outer non-client area
	int borderWidth;			//the width of the border in pixels:  TO DO: determine how DPI affects this
	HDWP hdwp;
	//unsigned int 
	switch(uMsg)
    {
        //case UWM_PAUSE:
            // notification that we should either pause or resume rendering
        //    _bPaused = (bool)wParam;
        //    break;

		
        case AVP_WINDOW_RESIZE:
		
			//process AVP_WINDOW_RESIZE bitmask options
			if( wParam & AVP_WINDOW_RESIZE_OPTION_BORDERLESS )
			{
				
				printf("\nAVP_WINDOW_RESIZE_OPTION_BORDERLESS \n");
				
				RECT winRect;
				RECT clRect;
				if( !(glWindow.screenmode == BORDERLESS) ) //make the window borderless (ie WS_POPUP)
				{
					  glWindow.screenmode = BORDERLESS;

					  //even though the window was 640 x 480 exactly before removing the title bar and menu
					  //the window rect provided by windows for the window will now be larger than 640x480
					  //so we must get the origin of the current window before removing the title bar and/or menu, 
						//while tracking the the windowed state to determine the size manually
					
					  GetWindowRect(glWindow.hwnd, &winRect);
					  GetClientRect(glWindow.hwnd, &clRect);

					  //calculate the border width assuming we are coming from a WINDOWED style
					  long borderWidth = (winRect.right - winRect.left) - clRect.right;
					  glWindow.borderWidth = borderWidth;

					  //store with border width because we won't be able to calculate it again OTF when toggling back, but we need to undo its applied offset

					//printf("AVP RESIZE borderWIdth = %ld\n", borderWidth);
					//printf("\AVP RESIZE window rect left = %ld top = %ld width = %ld, height=  %ld\n", winRect.left, winRect.top, (winRect.right - winRect.left), (winRect.bottom - winRect.top));
					//printf("\nAVP RESIZE window client left = %ld top = %ld width  = %ld, height=  %ld\n",  clRect.left, clRect.top, (clRect.right - clRect.left), (clRect.bottom - clRect.top));
		

					//remove the title bar
					DWORD dwStyle = GetWindowLong(glWindow.hwnd, GWL_STYLE);
					dwStyle &= ~WS_CAPTION;
					dwStyle &= ~WS_OVERLAPPEDWINDOW;
					dwStyle |= WS_POPUP;
					SetWindowLong(glWindow.hwnd, GWL_STYLE, dwStyle);
					//SetWindowLong(glWindow.hwnd, GWL_EXSTYLE, 0 & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
					
					//remove the menu
					 SetMenu(glWindow.hwnd, NULL);


					  //if we are maintaing client area correctly when the windowed style window has title bar and or menu
					  //we want that to be the size of our window when switching to a borderless style
					  //while staying at the exsiting window origin (ie. the window doesn't move wrt top left corner)
					  //since we know we don't have a title bar or menu we shouldn't need to call AdjustWindowRectEx
					  //RECT adjustedWindowRect = {0,0,glWindow.windowedWidth, glWindow.windowedHeight};
					  //RECT adjustedWindowRect = {0,0,clRect.right - clRect.left, clRect.bottom - clRect.top};
					  
					  if( !(glWindow.fullscreen) )
							SetWindowPos(glWindow.hwnd, NULL, winRect.left + borderWidth/2, winRect.top, clRect.right - clRect.left, clRect.bottom - clRect.top, SWP_FRAMECHANGED | SWP_NOZORDER);
					  else					 
						  	SetWindowPos(glWindow.hwnd, NULL, winRect.left + borderWidth/2, winRect.top, g_display.resolution.x, g_display.resolution.y, SWP_FRAMECHANGED | SWP_NOZORDER);

					  //SetWindowPos(glWindow.hwnd, NULL, NULL, NULL, NULL, NULL, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE );
					
					  
					//the last time the window was minimized it was bordered, but now we need to undo accounting for borders
					//when we stored the window x origin when it was last minimized
					if( glWindow.fullscreen )
					{
						glWindow.originx += glWindow.borderWidth/2;
					}


				}
				else //make the window style WINDOWED (ie WS_OVERLAPPEDWINDOW)
				{
					//add the menu back
					//SetMenu(glWindow.hwnd, glWindow.rootMenu);


					glWindow.screenmode = WINDOWED;

					//get the area of the client window and origin or the total window before making modifications to the window
					// though if coming back from a borderless window style state windowRect should equal clientRect
					
					GetWindowRect(glWindow.hwnd, &winRect);
					GetClientRect(glWindow.hwnd, &clRect);


					//modify the window style to add back the title bar and switch back to a WS_OVERLAPPEDWINDOW style instead of WS_POPUP (BORDERLESS)
					DWORD dwStyle = GetWindowLong(glWindow.hwnd, GWL_STYLE);
					dwStyle |= WS_CAPTION;
					dwStyle &= ~WS_POPUP;
					dwStyle |= WS_OVERLAPPEDWINDOW;
					  
					//show the title bar
					SetWindowLong(glWindow.hwnd, GWL_STYLE, dwStyle);
					//add the menu back
					SetMenu(glWindow.hwnd, glWindow.rootMenu);

					RECT adjustedWindowRect ={clRect.left, clRect.top, clRect.right, clRect.bottom};

					//get the window style
					dwStyle = GetWindowLong(glWindow.hwnd, GWL_STYLE);
					
					bool hasMenu = glWindow.rootMenu;
					//if the window has a title and/or menu we'll need to calculate the ajusted window rect
					if( (dwStyle & WS_CAPTION ) || hasMenu)
						AdjustWindowRectEx(&adjustedWindowRect, dwStyle, hasMenu, NULL);
					//MoveWindow(hMain, winRect.left, winRect.top, adjustedWindowRect.right-adjustedWindowRect.left, adjustedWindowRect.bottom-adjustedWindowRect.top, TRUE);
					
					//if( !(glWindow.fullscreen) )
						SetWindowPos(glWindow.hwnd, NULL, winRect.left - glWindow.borderWidth/2, winRect.top, adjustedWindowRect.right - adjustedWindowRect.left, adjustedWindowRect.bottom - adjustedWindowRect.top, SWP_FRAMECHANGED | SWP_NOZORDER );
					//else					 
					//  	SetWindowPos(glWindow.hwnd, NULL, winRect.left - glWindow.borderWidth/2, winRect.top, g_display.resolution.x, g_display.resolution.y, SWP_FRAMECHANGED | SWP_NOZORDER);
  
					//the last time the window was minimized it was borderless, but now we need to account for borders
					//if we come out of fullscreen
					if( glWindow.fullscreen )
					{
						glWindow.originx -= glWindow.borderWidth/2;
					}

				}


				
				GetWindowRect(glWindow.hwnd, &winRect);
				GetClientRect(glWindow.hwnd, &clRect);

				if( !glWindow.fullscreen )
				{
					//update the window width / height properties for opengl render loop to key off of
					//GetClientRect(glWindow.hwnd, &winRect);
					//GetClientRect(glWindow.hwnd, &clRect);

					glWindow.originx = winRect.left;
					glWindow.originy = winRect.top;
					glWindow.windowedWidth = clRect.right - clRect.left;// w;
					glWindow.windowedHeight = clRect.bottom - clRect.top;// h;
				}
				else //if fullscreen
				{		
					//we may need to check to see if the window extended beyond in the case of a full screen windowed style window with borders
					//because AdjustWindowRectEx will only remove 30 pixels from the client window area for the title bar even though it measures 52 pixels
					//and the menu measures 30 pixels
					if( winRect.bottom > g_display.resolution.y )
					{
						
						GetWindowRect(glWindow.hwnd, &winRect);
						GetClientRect(glWindow.hwnd, &clRect);

						winRect.bottom = g_display.resolution.y;
						SetWindowPos(glWindow.hwnd, NULL, winRect.left, winRect.top, winRect.right - winRect.left, winRect.bottom-winRect.top + glWindow.borderWidth/2, SWP_FRAMECHANGED | SWP_NOZORDER );

					}

				}
			
			}

			if( wParam & AVP_WINDOW_RESIZE_OPTION_FULLSCREEN )
			{
				if( !(glWindow.fullscreen) )
				{
				
					glWindow.fullscreen = true;

					//get/update the display resolution
					g_display.resolution = cr_display_get_resolution();
					
					

					// On expand, if we're given a window_rect, grow to it, otherwise do
					// not resize.
					//if (!for_metro) {
					  //MONITORINFO monitor_info;
					  //monitor_info.cbSize = sizeof(monitor_info);
					  //GetMonitorInfo(MonitorFromWindow(glWindow.hwnd, MONITOR_DEFAULTTONEAREST), &monitor_info);
					  //gfx::Rect window_rect(monitor_info.rcMonitor);
					  //SetWindowPos(glWindow.hwnd, NULL, 0, 0, monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
					  //AdjustWindowRectEx(&(monitor_info.rcMonitor), WS_OVERLAPPED|WS_BORDER|WS_SYSMENU|WS_CAPTION, FALSE, WS_EX_CLIENTEDGE);

					//make the Win32 window the same size as the display
					//the opengl backing buffer was already created at screen resolution so the window only needs to be recreated if the resolution changes

					RECT winRect;
					RECT clRect;
					GetWindowRect(glWindow.hwnd, &winRect);
					GetClientRect(glWindow.hwnd, &clRect);


					//modify the window style to add back the title bar and switch back to a WS_OVERLAPPEDWINDOW style instead of WS_POPUP (BORDERLESS)
					DWORD dwStyle = GetWindowLong(glWindow.hwnd, GWL_STYLE);
					  
					//calculate the border width if the window is a WINDOWED style
					long borderWidth = 0;  
					
					if( dwStyle & WS_OVERLAPPEDWINDOW )
					{
						borderWidth = (winRect.right - winRect.left) - clRect.right;
					    glWindow.borderWidth = borderWidth;
					}


						//save the x and y position so we can return from fullscreen
					//the windowed width and height of the window are already stored
					glWindow.originx = winRect.left;
					glWindow.originy = winRect.top;
					//glWindow.windowedWidth = clRect.right - clRect.left;
					//glWindow.windowedHeight = clRect.bottom - clRect.top;
					
					long width, height;
										
					width = g_display.resolution.x;
					height = g_display.resolution.y;

					RECT adjustedWindowRect ={0, 0, width, height};

					bool hasMenu = glWindow.rootMenu;
					//if the window has a title and/or menu we'll need to calculate the ajusted window rect
					if( (dwStyle & WS_CAPTION ) || hasMenu)
					{
						AdjustWindowRectEx(&adjustedWindowRect, dwStyle, hasMenu, NULL);

						//clip window rect to display resolution
						if( adjustedWindowRect.bottom  > g_display.resolution.y )
							adjustedWindowRect.bottom = g_display.resolution.y;

						width = adjustedWindowRect.right - adjustedWindowRect.left;
						height = adjustedWindowRect.bottom - adjustedWindowRect.top;

					}						
					//else //borderless		

					SetWindowPos(glWindow.hwnd, NULL, -borderWidth/2, 0, (int)width, (int)height, SWP_FRAMECHANGED | SWP_NOZORDER );

					//we may need to check to see if the window extended beyond in the case of a full screen windowed style window with borders
					//because AdjustWindowRectEx will only remove 30 pixels from the client window area for the title bar even though it measures 52 pixels
					//and the menu measures 30 pixels
					GetWindowRect(glWindow.hwnd, &winRect);
					GetClientRect(glWindow.hwnd, &clRect);


					if( winRect.bottom > g_display.resolution.y )
					{
						winRect.bottom = g_display.resolution.y;
						SetWindowPos(glWindow.hwnd, NULL, winRect.left, winRect.top, winRect.right - winRect.left, winRect.bottom-winRect.top + borderWidth/2, SWP_FRAMECHANGED | SWP_NOZORDER );

					}

				}
				else
				{

					glWindow.fullscreen = false;
					
					//revert the Win32 window to its previous size before being fullscreened
					//the opengl backing buffer was already created at screen resolution so the window only needs to be recreated if the resolution changes
					
					RECT winRect;
					RECT clRect;
					GetWindowRect(glWindow.hwnd, &winRect);
					GetClientRect(glWindow.hwnd, &clRect);



					//modify the window style to add back the title bar and switch back to a WS_OVERLAPPEDWINDOW style instead of WS_POPUP (BORDERLESS)
					DWORD dwStyle = GetWindowLong(glWindow.hwnd, GWL_STYLE);
					  
					//calculate the border width if the window is a WINDOWED style
					long borderWidth = 0;  
					
					if( dwStyle & WS_OVERLAPPEDWINDOW )
					{
						borderWidth = (winRect.right - winRect.left) - clRect.right;
					    glWindow.borderWidth = borderWidth;
					}

					
					long x, y, width, height;
					
					x= glWindow.originx;
					y = glWindow.originy;
					width = glWindow.windowedWidth;
					height = glWindow.windowedHeight;

					RECT adjustedWindowRect = {0, 0, width, height};


					bool hasMenu = glWindow.rootMenu;
					//if the window has a title and/or menu we'll need to calculate the ajusted window rect
					if( (dwStyle & WS_CAPTION ) || hasMenu)
					{
						AdjustWindowRectEx(&adjustedWindowRect, dwStyle, hasMenu, NULL);

						width = adjustedWindowRect.right - adjustedWindowRect.left;
						height = adjustedWindowRect.bottom - adjustedWindowRect.top;

						//x-=borderWidth/2;
						

					}
					//else //borderless
					//{
						SetWindowPos(glWindow.hwnd, NULL, x, y, (int)width, (int)height, SWP_FRAMECHANGED | SWP_NOZORDER );



					//SetWindowPos(glWindow.hwnd, NULL, glWindow.originx, glWindow.originy, glWindow.windowedWidth, glWindow.windowedHeight, SWP_FRAMECHANGED | SWP_NOZORDER );
				}

			}

			//when a win32 window is resized (say as the result of a use dragging the window to enlarge it)
			//we received the message from the window control thread here, and we set this flag to process 
			//the resize for opengl related views and buffers on the next pass of the gl render loop
			//glWindow.resize = true;
            break;
			
        case AVP_WINDOW_CLOSE:
			fprintf(stdout, "\nrender thread AVP_WINDOW_CLOSE\n");
            // notification that we should stop the render thread from executing and close the view
			glWindow.shouldClose = true;
			break;
		case AVP_WINDOW_TOGGLE_VSYNC:
		{
			glWindow.vsync = wParam;
			wglSwapIntervalEXT(glWindow.vsync);
			break;
		}
		/*
		case AVP_WINDOW_GRAPHICS_CONTEXT_INITIALIZED:
			break;
		case AVP_WINDOW_RENDER_INSTRUCTION:

			glWindow.renderText = (bool)lParam;
			glWindow.textID = (int)wParam;
			break;

		case AVP_WINDOW_SET_RENDER_STATE:

			fprintf(stdout, "\nrender thread AVP_WINDOW_SET_RENDER_STATE: %u\n", (int)wParam);
			break;
		*/
            
    }
}


void renderVideoFrame(GLuint * videoFrame)
{


		//the call to updateCamera will updat ethe viewport
		//glViewport(0, 0, glWindow.windowedWidth, glWindow.windowedHeight);

		//if we aren't using depth testing and we are using double buffering, we don't
		//need to clear the framebuffer
		//glClearColor(0.0, 1.0, 0.0,1.0);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		glDisable(GL_BLEND);
		//glDisable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//if we enable depth test we must clear the buffer with glClear or we will get black screen
		//however, if we don't enable depth testing there is no need to clear the color buffer with glClear
		//since we know we are repainting all pixels in the buffer when we fill our screen sized textuloadTred quad
		glDisable(GL_DEPTH_TEST);

		//we can cull back faces for performance
		//not that it matters in the case of a single quad
		glCullFace(GL_BACK);
		glEnable(GL_CULL_FACE);

		//glShadeModel( GL_SMOOTH );
		//glClearDepth( 1.0f );
		//glEnable( GL_DEPTH_TEST );
		//glDepthFunc( GL_LEQUAL );
		//glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
		//glEnable(GL_CULL_FACE);
		//glCullFace(GL_BACK);


		//update camera and calculate projection and view matrices
		//
		//Note:  though projection matrix only needs to be calculated once
		//and technically for video rendering view matrix only needs to be calcualted once
		//but is not a performance concern so we'll leave it here for clarity
		//updateCamera();

		//create a model to world matrix for the quad
		//and multiply this with the view matrix input to create the modelView matrix CPU side
		updateQuad(&(updateCamera()));
		//printf("\n after update quad\n");

		//render quad vbo geometry with specified texture input
		renderTexturedQuad(videoFrame);


		//WaitForSingleObject(g_display.verticalRetraceEvent, 5);	
		//swap the back buffer to screen for double buffered contexts
		SwapBuffers(glWindow.hdc);

		//measure time between SwapBuffers calls if desired
		//t2 = tick();
		//dElapsed = tick2sec(t2 - t1);
		//t1 = t2; //set t1 to current time

		//print time elapsed
		//printf("GPGPU render time elapsed:  %g\n", dElapsed);

		//debug to check for opengl errors
		//getOpenGLErrors();

}

void renderClearScreen( avt_float4 * rgba )
{
	glClearColor(rgba->r, rgba->g, rgba->b, rgba->a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	SwapBuffers(glWindow.hdc);
}



unsigned int __stdcall asyncRenderCallback (void * view)
{
	
	//resize defintiions
	RECT rcWindow;
	RECT rcClient;

	//Timing definitions
	CR_LARGE_INTEGER t1,t2;//= tick();

	GLsync gpusync[_numPBOs];
	GLenum ret;
	
	GLint64 startTime, stopTime;
	unsigned int queryID[2];

	DWORD dwFPSCurrent , dwFPSLast, dwFPSLastFrameSync;  // used to help calc the fps
    unsigned long nFPS;               // current framerate/FPS for the main loop

	
    static double dLastTime, dCurTime;  // used to calc CPU cycles during a render
    static double dElapsed;                 // used to calc CPU cycles during a render
	//End Timing definitions

	MSG msg;
	
	SetProcessDPIAware();

	setThreadPriority(THREAD_PRIORITY_TIME_CRITICAL);

	//if there is already a context associated with the view, and there will be for the first view loaded
	//because ms requires an opengl context created with wglCreateContext to load the wglCreateContextARB function
	if (glWindow.hglrc)
	{
		//crgc_view_destroy_context(renderView);
		destroy_window_context(&glWindow);
	}


	if (glWindow.hdc == NULL)
	{
		printf("\nhdc == NULL\n");
		MessageBox(glWindow.hwnd, "Failed to Get the Window Device Context", "Device Context Error", MB_OK);
		PostQuitMessage(0);
		//break;
		//exitStatus = CR_PLATFORM_API_CONTEXT_FAILURE;
		return 0;
	}

	//we may have platform window event loop on the main thread, but always create 
	//the opengl graphics context associated with the window on the thread we want to render on (i.e. call opengl calls and SwapBuffers)
	//createWindowGraphicsContext(glWindow.hwnd, glWindow.hglrc);
	glWindow.hglrc = createOpenGLContext(&glWindow, 0, 0);

	//destroy the existing dummy context, but stay on the same thread so we can use the wgl extensions that it loaded
	//to recreate the context
	//crgc_view_destroy_context(renderView);
	//make sure the context we are releasing isn't current on any threads
	destroy_window_context(&glWindow);

	//Send a synchronous message to destroy the Win32 window
	SendMessage(glWindow.hwnd, AVP_WINDOW_RECREATE, 0, 0);

	//wait for the crgc_view_event_loop thread to notify us that the win32 window has been recreated

	while (1)//PeekMessage(&msg, NULL, CRGC_VIEW_PAUSE, CRGC_VIEW_CLOSE, PM_REMOVE))
	{

		PeekMessage(&msg, NULL, AVP_WINDOW_RESIZE, AVP_WINDOW_CLOSE, PM_REMOVE);
		if (msg.message == AVP_WINDOW_RECREATE)
		{
			//__processRenderThreadMessage(renderView, msg.message, msg.wParam, msg.lParam); 
			memset(&msg, 0, sizeof(MSG));
			break;
		}
		memset(&msg, 0, sizeof(MSG));
	}


	glWindow.hglrc = createOpenGLContext(&glWindow, glWindow.gcMajorVersion, glWindow.gcMinorVersion);
	wglMakeCurrent(GetDC(glWindow.hwnd), glWindow.hglrc);

	//load desired OpenGL functions and extensions
	//gl_lite_init();
	crgc_gl_ext_init();

	//if (!success)
	//	printf("\ncrgc_gl_ext_init failed!\n");

	wglMakeCurrent(0,0);

	//now that we can create a context greater than opengl version 1.1 fixed function pipeline with wgl ARB extensions, 
	//create OpenGL Context for GPGPU window/thread.  we create it on this render thread because we want to use wglShareLists
	gpgpuWindow.hglrc = createOpenGLContext(&gpgpuWindow, gpgpuWindow.gcMajorVersion, gpgpuWindow.gcMinorVersion);

	wglMakeCurrent(0,0);
	wglMakeCurrent(GetDC(glWindow.hwnd), glWindow.hglrc);
	

	//create gl shared context for loading textures on a seperate thread
	//printf("\nbefore wglShareLists\n");
	
	if( !wglShareLists(glWindow.hglrc, gpgpuWindow.hglrc) )
	{
		char buf[256];
		DWORD dwErr = GetLastError();
		sprintf(buf, "wglShareLists Failed with error: %lu", dwErr);
		//MessageBox(NULL, buf, "wglShareLists Error", MB_OK);
		printf("\n%s\n", buf);
	}

	//printf("\nafter wglShareLists\n");
			
	//start the asynchronous gpgpu window render thread
	gpgpuWindow.renderThreadID = 0;
	_beginthreadex(NULL, 0, asyncGPGPUCallback, 0, 0, &gpgpuWindow.renderThreadID);
	
	//wglMakeCurrent(GetDC(glWindow.hwnd), glWindow.hglrc);

	int samples, numTextureUnits;
	//query the max sample rate
	glGetIntegerv(GL_MAX_SAMPLES, &samples);
	printf("\nGL_MAX_SAMPLES:	%d\n", samples);

	//glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &samples);
	//printf("\nGL_MAX_SAMPLES:	%d\n", samples);

	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &numTextureUnits);
	printf("\nGL_MAX_TEXTURE_IMAGE_UNITS:	%d\n", numTextureUnits);

	//tell the win32 window control thread that opengl is finished initializing on the render thread
	//at current, this is only a roundabout way of updating the text to be rendered by this thread
	//SendMessage(glWindow.hwnd, AVP_WINDOW_GRAPHICS_CONTEXT_INTIALIZED, 0,0); //synchronous
	//PostMessage(glWindow.hwnd, AVP_WINDOW_GRAPHICS_CONTEXT_INITIALIZED, 0, 0); //asynchronous
	glWindow.shouldClose = (bool)!SendMessage(glWindow.hwnd, AVP_WINDOW_GRAPHICS_CONTEXT_INITIALIZED, 0, 0);//pArgList->bZoomed, 0);

	glClearColor(0.75,0.75,0.75,1.0);

	//LPARAM lParam = MAKELPARAM(glWindow.width, glWindow.height);
	//SendMessage(glWindow.hwnd, WM_SIZE, 0, lParam);
	//hack to reset the window with current size because MS Windows does not offset it by the application title bar when initialized
	//RECT windowRect;
	//GetWindowRect(glWindow.hwnd, &windowRect);
	//SetWindowPos(glWindow.hwnd, NULL, windowRect.left, windowRect.top, windowRect.right - windowRect.left +1,windowRect.bottom-windowRect.top+1, /*SWP_ASYNCWINDOWPOS |*/ SWP_FRAMECHANGED);// SWP_NOREDRAW);
		
	//ResizeWin32Window(glWindow.hwnd, glWindow.width, glWindow.height);
	
	//ShowWindow(glWindow.hwnd, SW_SHOW);
	//UpdateWindow(glWindow.hwnd);

	compileShaders();

	setupQuadVBOs();

	/*
	int32_t numAvailableBytes = 0;
	GLuint * textureHandlePtr = (GLuint*)avt_circ_buffer_tail(&_renderTextureSync, &numAvailableBytes);


	while( numAvailableBytes > 15 ) //ensure that 4 frames are buffered
	{
		printf("\n avialable bytes = %d\n", numAvailableBytes);
		//videoFrame = *textureHandlePtr;
		//free up 20 bytes in the circular buffer
		//avt_circ_buffer_consume(&_renderTextureSync, 4);
	}
	*/

	wglSwapIntervalEXT(glWindow.vsync);

	//do some last minute initializations before starting the render loop
	//gpusync[0] = gpusync[1] = 0;// {0,0};
	//glViewport(0, 0, glWindow.width, glWindow.height);
	
	getOpenGLErrors();
			
	//initialize timing
	tickInit();
	t1 = t2 = tick();

	

	avt_float4 blackColor = { 0.0f, 0.0f, 0.0f, 1.0f };
	avt_float4 backgroundColor = { 0.75f, 0.75f, 0.75f, 1.0f };

	//start the display event loop
	while(!glWindow.shouldClose)
	{

		
		
		switch( g_videoEventState )
		{
			
			case VIDEO_EVENT_STATE_PLAYING:
			{
				//t2 = tick();
				//printf("\nRender time elapsed = %g", tick2sec(t2 - t1));
				//t1 = t2;

				//if the video state is playing, only render video frames
				//when they are available in the circ buffer
				//GLuint videoFrame = 0;
				int32_t numAvailableBytes = 0;
				GLuint * textureHandlePtr = (GLuint*)avt_circ_buffer_tail(&_renderTextureSync, &numAvailableBytes);

				if (numAvailableBytes > (NUM_FRAMES_TO_BUFFER) * 4 - 1) //ensure that 1 frames are buffered
				{
					//printf("\n avialable bytes = %d\n", numAvailableBytes);
					_videoFrame = textureHandlePtr[0];
					//remove the reference to the opengl texture unit from the circular buffer
					avt_circ_buffer_consume(&_renderTextureSync, 4);

					renderVideoFrame(&_videoFrame);
					


				}

				

				break;
			}
			case VIDEO_EVENT_STATE_PAUSED:
			{
				/*
				//if the video state is paused
				//we will keep reading frames until the circular buffer is empty
				//but reissue the draw calls with the last valid frame
				int32_t numAvailableBytes = 0;
				GLuint * textureHandlePtr = (GLuint*)avt_circ_buffer_tail(&_renderTextureSync, &numAvailableBytes);

				if (numAvailableBytes > (NUM_FRAMES_TO_BUFFER) * 4 - 1) //ensure that 1 frames are buffered
				{
					//printf("\n avialable bytes = %d\n", numAvailableBytes);
					_videoFrame = textureHandlePtr[0];

					//remove the reference to the opengl texture unit from the circular buffer
					avt_circ_buffer_consume(&_renderTextureSync, 4);

				}

				renderVideoFrame(&_videoFrame);
				*/

				if (_videoFrame)
					renderVideoFrame(&_videoFrame);
				else //set the screen color to black to indicate the video is finished buffering and is ready for playback
					renderClearScreen(&blackColor);

				break;


				break;
			}
			case VIDEO_EVENT_STATE_IDLE:
			default:
			{
				renderClearScreen(&backgroundColor);
				break;
			}
		}
		


		// NOTE: it is imperative that PeekMessage() is used rather than GetMessage() when on background render thread
		// listen to the queue in case this thread received a message from the event queue on another thread
		//listen after command buffer has been passed to opengl for the current frame, if we read before this causes dropped frames on resize
		//but it also means events won't be processed until the following frame, which in most cases is generally ok
		//passing NULL for second parameter ensures that messages for the platform window AND non-window thread messages will be processed
		//maximum of 10000 messages per queue
				
		//getOpenGLErrors();
				
		if(PeekMessage(&msg, NULL, AVP_WINDOW_MESSAGE_FIRST, AVP_WINDOW_MESSAGE_LAST, PM_REMOVE))
		{
			__processRenderThreadMessage(msg.message, msg.wParam, msg.lParam); 
			memset(&msg, 0, sizeof(MSG));
		}

		//wait for vertical retrace !!!
		//since we are waiting for event objects to be flipped so that one becomes signaled
		//we can't use a timeout of zero, since the event we are watching only pulses and may not be signaled at the time this is called
		//must use a timeout greater than that of the framerate
		//cr_wait_for_display_sync(INFINITE);
		//if( renderView->index %2 == 0 )
		//WaitForSingleObject(g_display.verticalRetraceEvent, 5);	

	}


	//clean up the graphics context
	//make sure the context we are releasing isn't current on any threads
	wglMakeCurrent(glWindow.hdc, NULL);		// release device context in use by rc
	wglDeleteContext(glWindow.hglrc);		// delete rendering context

	// message back to the platform window control thread (i.e the thread that created the platform window) 
	// that we have cleaned up the graphics context and we are ready to close the platform window
	if( glWindow.shouldClose )
	{
		/*exitStatus =*/ SendMessage(glWindow.hwnd, AVP_WINDOW_CLOSE, 0,0);
	}

	// kill this thread and its resources (CRT allocates them)
    _endthreadex(0);

	//return the status of the view received from the control thread
	return 0;//exitStatus;
}



static INT_PTR CALLBACK videoDialogCallback(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
{
	WORD commandID;
 switch (wm) {
 case WM_INITDIALOG: 
	 {
			setDialogPosition(glWindow.hwnd, hwnd);
			//setDialogFont(hwnd);
             break;
		 return TRUE;
	 }
 case WM_COMMAND:
	commandID = GET_WM_COMMAND_ID(wParam, lParam);  
  if (commandID == IDCANCEL) 
	  EndDialog(hwnd, 0);
  else if ( commandID == IDOK ) 
  {
	  //store the video resolution and commence loading video 
	  //by sending a message to the GPGPU "render" thread

	  char vWidthStr[1024];
	  char vHeightStr[1024];
	  GetDlgItemText(hwnd, ID_EDIT_WIDTH, vWidthStr, 1024);
	  GetDlgItemText(hwnd, ID_EDIT_HEIGHT, vHeightStr, 1024);

	  //update the avt_video meteadata object
	  g_sourceVideo.width = atoi(vWidthStr);
	  g_sourceVideo.height = atoi(vHeightStr);
	  g_sourceVideo.sourcepath = (char*)malloc(strlen(g_videoSourcePath)+1);
	  strcpy(g_sourceVideo.sourcepath, g_videoSourcePath);


	  //message the gpgpu thread to load the video using the metadata
	  WPARAM controlEvent = AVP_VIDEO_CONTROL_LOAD;
	  PostThreadMessage(gpgpuWindow.renderThreadID, AVP_VIDEO_CONTROL_EVENT, controlEvent, NULL);

	  //printf("\nEdit Width: %u, Height: %u\n", g_sourceVideo.width, g_sourceVideo.height);
	  
	  EndDialog(hwnd, 0);
  }

  break;
 }
 return FALSE;
}


void processLoadVideoFile(char * videofilePath)
{
	//TO DO:  compare video file strings to see if it is already loaded
	//probably not that important 

	if( videofilePath != g_videoSourcePath )
	{
		if( g_videoSourcePath )
			free(g_videoSourcePath);
		else
			g_videoSourcePath = NULL;

		g_videoSourcePath = (char*)malloc( (strlen( videofilePath)+1) * sizeof(char));

		strcpy(g_videoSourcePath, videofilePath);
	}


	/*
	//create wide char strings from c string memory
	wchar_t w_videoSourcePath[4096];//( strlen(g_videoSourcePath), L'#' );
	mbstowcs( &w_videoSourcePath[0], g_videoSourcePath, strlen(g_videoSourcePath) );

	System::String^ const s_videoSourcePath = gcnew System::String(w_videoSourcePath);
	//TO DO: sourcepath needs to be freed
	//_beginthreadex(NULL, 0, asyncLoadSourceGeometryCallback, g_sourcepath, 0, &(glWindow.loadThreadID));

	//tell the gpgpuWindow thread to load the video or prepare to load the video
	//based on streaming configuration

	//but first get some information about the video file from the user if needed
	string input = Microsoft::VisualBasic::Interaction::InputBox(s_videoSourcePath , L"Load Uncompressed Format", L"DefResp & vbNewLine & \"a.\"", 500, 500);
	*/

	createVideoPropertiesDialog(glWindow.hwnd, L"Specify Raw Format Properties", videoDialogCallback, glWindow.width, glWindow.height);

}

bool GoFullscreen()
{
	// turn off window region without redraw
	SetWindowRgn(glWindow.hwnd, 0, false);

	DEVMODE newSettings;	

	// request current screen settings
	EnumDisplaySettings(0, 0, &newSettings);

	//  set desired screen size/res	
 	newSettings.dmPelsWidth  = glWindow.width;//GetWidth();		
	newSettings.dmPelsHeight = glWindow.height;//GetHeight();		
	newSettings.dmBitsPerPel = 32;		

	//specify which aspects of the screen settings we wish to change 
 	newSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

	// attempt to apply the new settings 
	long result = ChangeDisplaySettings(&newSettings, CDS_FULLSCREEN);

	// exit if failure, else set datamember to fullscreen and return true
	if ( result != DISP_CHANGE_SUCCESSFUL )	return false;
	else 
	{
		// store the location of the window
		//m_oldLoc = GetLocation();

		// switch off the title bar
	    DWORD dwstyle = GetWindowLong(glWindow.hwnd, GWL_STYLE);
	    dwstyle &= ~WS_CAPTION;
	    SetWindowLong(glWindow.hwnd, GWL_STYLE, dwstyle);

		// move the window to (0,0)
		SetWindowPos(glWindow.hwnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		InvalidateRect(glWindow.hwnd, 0, true);		

		return true;
	}
}


void toggleVsync()
{
	HMENU playbackMenu = GetSubMenu(glWindow.rootMenu, 1);
	MENUITEMINFOA menuitem = { sizeof(MENUITEMINFOA) };

	if (glWindow.vsync)
	{
			GetMenuItemInfoA(playbackMenu, IDM_PLAYBACK_TOGGLE_VSYNC, false, &menuitem);
			menuitem.dwTypeData = "&Enable Vsync\tV";
			menuitem.fMask = MIIM_TYPE | MIIM_DATA;
			SetMenuItemInfoA(playbackMenu, IDM_PLAYBACK_TOGGLE_VSYNC, false, &menuitem);		
	}
	else
	{
			GetMenuItemInfoA(playbackMenu, IDM_PLAYBACK_TOGGLE_VSYNC, false, &menuitem);
			menuitem.dwTypeData = "&Disable Vsync\tV";
			menuitem.fMask = MIIM_TYPE | MIIM_DATA;
			SetMenuItemInfoA(playbackMenu, IDM_PLAYBACK_TOGGLE_VSYNC, false, &menuitem);
	}

	//send the message to the glWindow render thread to tell it to toggle vsync using wglSwapInterval
	PostThreadMessage(glWindow.renderThreadID, AVP_WINDOW_TOGGLE_VSYNC, !glWindow.vsync, NULL);
}


void togglePausePlay()
{
	if (g_videoEventState == VIDEO_EVENT_STATE_PLAYING)
	{

		//HMENU rootMenu = GetMenu(glWindow.hwnd);
		if (glWindow.rootMenu) {

			HMENU playbackMenu = GetSubMenu(glWindow.rootMenu, 1);
			MENUITEMINFOA menuitem = { sizeof(MENUITEMINFOA) };
			GetMenuItemInfoA(playbackMenu, IDM_PLAYBACK_PLAY_PAUSE, false, &menuitem);
			menuitem.dwTypeData = "&Play\t[Space]";
			menuitem.fMask = MIIM_TYPE | MIIM_DATA;
			SetMenuItemInfoA(playbackMenu, IDM_PLAYBACK_PLAY_PAUSE, false, &menuitem);
			//hMenuTrackPopup = GetSubMenu(hMenu, 0);
			//TrackPopupMenu(hMenuTrackPopup, 0, point.x, point.y, 0, hWnd, NULL);
			//DestroyMenu(hMenu);
		}

		setVideoEventState(VIDEO_EVENT_STATE_PAUSED);
	}
	else if (g_videoEventState == VIDEO_EVENT_STATE_PAUSED)
	{
		//HMENU rootMenu = GetMenu(glWindow.hwnd);
		if (glWindow.rootMenu) {

			HMENU playbackMenu = GetSubMenu(glWindow.rootMenu, 1);
			MENUITEMINFOA menuitem = { sizeof(MENUITEMINFOA) };
			GetMenuItemInfoA(playbackMenu, IDM_PLAYBACK_PLAY_PAUSE, false, &menuitem);
			menuitem.dwTypeData = "&Pause\t[Space]";
			menuitem.fMask = MIIM_TYPE | MIIM_DATA;
			SetMenuItemInfoA(playbackMenu, IDM_PLAYBACK_PLAY_PAUSE, false, &menuitem);
			//hMenuTrackPopup = GetSubMenu(hMenu, 0);
			//TrackPopupMenu(hMenuTrackPopup, 0, point.x, point.y, 0, hWnd, NULL);
			//DestroyMenu(hMenu);
		}
		setVideoEventState(VIDEO_EVENT_STATE_PLAYING);
	}
	else
	{
		printf("\nError:  Spacebar key has no effect for current video event state: %d", g_videoEventState);
	}

}

/*
void SendThreadMessage(DWORD threadId, unsigned long message, WPARAM param)
{
	HANDLE h = CreateEvent(NULL, 0, 0, NULL);
	::PostThreadMessage(threadId, message, param, h);
	WaitForSingleObject(h, INFINITE);
	CloseHandle(h);
}
*/

//*=====================
//  The MS Windows API window event handler
//*===================== 

LRESULT CALLBACK processWin32WindowMessageCallback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	//printf("\nWinProc msg = %u\n", msg);
	
	glWindowContext * windowContext;

	if( hwnd == gpgpuWindow.hwnd)
		windowContext = &gpgpuWindow;
	else
		windowContext = &glWindow;

	switch(msg)
	{
		case WM_CREATE:		//when the WINDOWS API window has been created we will use this to create the opengl context for the window
		{	
				//use wgl to initialize opengl for a WINDOWS API Window
				printf("\nWM_CREATE\n");
				//createOpenGLWindow(hwnd);

				//accept drag and drop files
				DragAcceptFiles(hwnd,TRUE);


				// everything went OK, show the window
				if( windowContext == &glWindow )
				{
					//ShowWindow(hwnd, SW_SHOW);
					//UpdateWindow(hwnd);
				}
				break;
		}
		case WM_DROPFILES:
        {
			static const int SIZE = 4096;

			char *buffer;
			char* outText;
			HDROP hDropInfo = NULL;
			UINT buffsize= SIZE;
		    char draggedFilepath[SIZE];
	
            hDropInfo = (HDROP) wparam;
 
            DragQueryFile (hDropInfo, 0, draggedFilepath, buffsize);
 
			printf("\nReceived Drag and Drop: %s\n", draggedFilepath);
	
			char pwd[4096];
	
			GetCurrentDirectory(4096, pwd);

			printf("current directory: %s\n", pwd);

			if( draggedFilepath )
			{

				//TO DO: check that source file is valid based on the application state
				if( g_appState == AVP_IMPORT_SOURCE_VIDEO )
				{

					 //_beginthreadex(NULL, 0, asyncLoadSourceGeometryCallback, (void*)draggedFilepath, 0, &(glWindow.loadThreadID));

					//asyncLoadSourceGeometryCallback(sourcepath); //will free sourcepath
					//loadSourceGeometry(sourcepath);
					
					processLoadVideoFile(draggedFilepath);


					 //printf("\nsourcepath = %s\n", g_sourcepath);
					 //MessageBeep(MB_ICONINFORMATION);
					 break;
				}

			}

 
            break;
        }

		
		case WM_CLOSE:		//when user clicks 'close' button, this is the first method that will handle it
			
			printf("\nWM_CLOSE\n");

			if (MessageBox(hwnd,"Are you sure you want to quit?","Exit Application", MB_OKCANCEL) == IDOK)
			{
				//DestroyWindow(hwnd);
				//PostShutdownMessage();
							
				//forward AVP_WINDOW_CLOSE message asynchronously to our glWindow rendering on a separate thread
				//to tell it to shutdown graphics context rendering and cleanup buffers (e.g. opengl)
				PostThreadMessage(glWindow.renderThreadID, AVP_WINDOW_CLOSE, NULL, NULL);
				PostThreadMessage(gpgpuWindow.renderThreadID, AVP_WINDOW_CLOSE, NULL, NULL);
				//PostThreadMessage(gpgpuWindow.renderThreadID, AVP_WINDOW_CLOSE, NULL, NULL);
				//PulseEvent(_gpgpuIdleEvent);
				// Indicate to Win32 that we processed this messasge ourselves
				return 0;

			}
			
			// Else: User canceled. Do nothing.
			return 0;
		case AVP_WINDOW_RECREATE:
		{
			ShowWindow(glWindow.hwnd, SW_HIDE);
			destroy_window(&glWindow);

			fprintf(stdout, "\nAVP_WINDOW_RECREATE \n");
			//DestroyWindow(view->hwnd);			//send the destroy window message to the wndproc behind the hwnd platform window
			return 0;
		}
		case AVP_WINDOW_CLOSE:
			fprintf(stdout, "\nAVP_WINDOW_CLOSE\n");

			if (glWindow.hwnd == hwnd)
				destroy_window(&glWindow);
			else
				destroy_window(&gpgpuWindow);

			return 0;

		case WM_DESTROY:	//when DestroyWindow(hwnd) is called, this is the first method that will handle it
         	printf("\nWM_DESTROY\n");
			PostQuitMessage(0);
			//PostShutdownMessage();
			//return 0;
		case WM_SIZE:
		{	

			
			//glViewport(0, 0, glWindow.width, glWindow.height);
			int w = LOWORD(lparam);
            int h = HIWORD(lparam);

			printf("\n Lparam w: %d, h: %d\n", w, h);
			//glWindow.width = w;
			//glWindow.height = h;

			
			//glResize(w, h);

			//glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
			//glEnable(GL_DEPTH_TEST);
			//render
			//glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
			//glClearDepth(1.0f);
		
			//renderQuad();
			//SwapBuffers(GetDC(glWindow.hwnd));
			

			//send a message to the opengl render thread to let it know the window has resized
			PostThreadMessage(glWindow.renderThreadID, AVP_WINDOW_RESIZE, NULL, lparam);

			//PostThreadMessage(gpgpuWindow.renderThreadID, AVP_WINDOW_RESIZE, NULL, lparam);

			break;
			
		}
		case AVP_WINDOW_GRAPHICS_CONTEXT_INITIALIZED:
		{

			//message the render thread to tell it what instructions to draw on the screen
			//WPARAM tg_instruction_id = TG_IMPORT_SOURCE_GEOMETRY;
			//PostThreadMessage(glWindow.renderThreadID, AVP_WINDOW_RENDER_INSTRUCTION, tg_instruction_id, 1);

			//show window
			ShowWindow(hwnd, SW_SHOW);
			UpdateWindow(hwnd);

			//bring window to front
			SetForegroundWindow(hwnd);

			//give keyboard focus to the window
			SetFocus(hwnd);

			return 1; //success
			//break;
		}
		case WM_COMMAND:
		{
		  //char draggedFilepath[SIZE];
		  //char * sourcepath = NULL;      
          switch(LOWORD(wparam)) {
          

              case IDM_FILE_QUIT:
              
                  //SendMessage(hwnd, WM_CLOSE, 0, 0);
                  PostThreadMessage(glWindow.renderThreadID, AVP_WINDOW_CLOSE, NULL, NULL);
				  PostThreadMessage(gpgpuWindow.renderThreadID, AVP_WINDOW_CLOSE, NULL, NULL);
				  break;
				case IDM_FILE_IMPORT_VIDEO:
				{
				//case IDM_FILE_OPEN:
              

					openFileDialog(hwnd, &g_videoSourcePath, "BGRA (.bgra)\0*.bgra\0"); //will allocate memory for sourcepath

					if( g_videoSourcePath )
					{

						processLoadVideoFile(g_videoSourcePath);

					}



					break;
				}
				case IDM_PLAYBACK_PLAY_PAUSE:
				{
					togglePausePlay();
					break;
				}
				case IDM_PLAYBACK_TOGGLE_VSYNC:
				{
					toggleVsync();
					break;
				}
				case IDM_WINDOW_TOGGLE_BORDER:
				{
					PostThreadMessage(glWindow.renderThreadID, AVP_WINDOW_RESIZE, AVP_WINDOW_RESIZE_OPTION_BORDERLESS, MAKELPARAM(glWindow.width, glWindow.height));
					break;
				}
				case IDM_WINDOW_TOGGLE_FULLSCREEN:
				{
					PostThreadMessage(glWindow.renderThreadID, AVP_WINDOW_RESIZE, AVP_WINDOW_RESIZE_OPTION_FULLSCREEN, MAKELPARAM(glWindow.width, glWindow.height));
					break;
				}
			}
           
           break;
		}
		case WM_KEYDOWN:
		{
            switch(wparam)
            {
				case 'F':
				{

					//if( !(glWindow.fullscreen) )
						//pass the resize command to the render window thread, tell it to go at the display resolution 
						PostThreadMessage(glWindow.renderThreadID, AVP_WINDOW_RESIZE, AVP_WINDOW_RESIZE_OPTION_FULLSCREEN, MAKELPARAM( glWindow.width, glWindow.height));	
					//else
					//	PostThreadMessage(glWindow.renderThreadID, AVP_WINDOW_RESIZE, AVP_WINDOW_RESIZE_OPTION_FULLSCREEN, MAKELPARAM( glWindow.width, glWindow.height));	


					return 0;
				}
				case 'B':
				{
					PostThreadMessage(glWindow.renderThreadID, AVP_WINDOW_RESIZE, AVP_WINDOW_RESIZE_OPTION_BORDERLESS, MAKELPARAM( glWindow.width, glWindow.height));	
					return 0;
				}
				case 'V':
				{
					toggleVsync();
					return 0;
				}
                case VK_ESCAPE:
				{
					PostThreadMessage(glWindow.renderThreadID, AVP_WINDOW_CLOSE, NULL, NULL);
					PostThreadMessage(gpgpuWindow.renderThreadID, AVP_WINDOW_CLOSE, NULL, NULL);
					return 0;
				}
				case VK_SPACE:
				{
					togglePausePlay();
					return 0;
				}

			}
		}

		default:			//we haven't explicity handled this message type, call DefWindowProc for unhandled types
			return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	
	return 0;//DefWindowProc(hwnd, msg, wparam, lparam);
}

void createPlatformWindow(glWindowContext * window)
{
	//for non-WinMain applications, the hinstance is not provided as an application parameter, get the hinstance as follows:	

	if (!window->hinstance)
		window->hinstance = (HINSTANCE)GetModuleHandle(NULL);

	//printf("\nafter getModuleHandle\n");

	//Create/Register Microsoft Windows API Window Class
	//CreateWndClass( glWindow.wndclassex, glWindow.hinstance );
	memset(&(window->wndclassex), 0, sizeof(WNDCLASSEX));
	RegisterWndClass(window->className, window->wndclassex, window->hinstance, window->eventQueue);

	//printf("\nafter registerWndClass\n");

	//Create the Win32 Window and obtain handles to the window (HWND) and Device Context (HDC)
	bool hasTitleBar = true;
	bool hasMenu = true;
	CreateWnd(window->title, window->className, window->hwnd, window->hinstance, window->hdc, (window->originx), (window->originy), (window->width), (window->height), window->bitdepth, window->screenmode, hasTitleBar, hasMenu);

	window->hdc = GetDC(window->hwnd);
	//printf("\nafter CreateWnd\n");

	//record the thread id of the current thread the win32 window event loop is running on so it can be used for interthread messaging 
	window->controlThreadID = GetCurrentThreadId();
	//createRenderWindow();

}

void setWindowClassName(glWindowContext * window, int viewIndex)
{

#ifdef UNICODE
	const wchar_t * viewStrBase;
	const wchar_t * classStrSuffix;
	wchar_t indexStr[256];

#else
	const char * viewStrBase;
	const char * classStrSuffix;
	char indexStr[256];

#endif 

	//fill structs defining desired window properties
//glWindowContext glWindow;
#ifdef UNICODE
	viewStrBase = L"CoreRenderOpenGLView\0";
	classStrSuffix = L"CoreRenderWindowClass\0";
#else
	viewStrBase = "AsyncVideoPlayerWindow";
	classStrSuffix = "Class";
#endif

#ifdef UNICODE
	_itow(viewIndex + 1, indexStr, 10);

	//create Title and Class Name for the Platform Window
	glView[viewIndex].title = (wchar_t*)malloc((wcslen(viewStrBase) + 1) * sizeof(wchar_t));

	//swprintf(
	memcpy(glView[viewIndex].title, viewStrBase, wcslen(viewStrBase) * sizeof(wchar_t));
	glView[viewIndex].title[wcslen(viewStrBase)] = L'\0';


	//itoa(viewIndex+1, indexStr, 10);
	wcscat(glView[viewIndex].title, indexStr);
	glView[viewIndex].title[wcslen(viewStrBase) + wcslen(indexStr)] = L'\0';

	glView[viewIndex].className = (wchar_t*)malloc(wcslen(glView[viewIndex].title) + wcslen(classStrSuffix) + 1);
	memcpy(glView[viewIndex].className, glView[viewIndex].title, wcslen(glView[viewIndex].title) * sizeof(wchar_t));
	glView[viewIndex].className[wcslen(glView[viewIndex].title)] = L'\0';

	wcsncat(glView[viewIndex].className, classStrSuffix, wcslen(classStrSuffix));
	glView[viewIndex].className[wcslen(glView[viewIndex].title) + wcslen(classStrSuffix)] = L'\0';

	//printf("\nview index = %s\n", indexStr);
	printf("\nview title = %S\n", glView[viewIndex].title);

	//printf("\nsize = %d\n", strlen(classStrSuffix));
	printf("\nclassName = %S\n", glView[viewIndex].className);


#else
	itoa(viewIndex + 1, indexStr, 10);

	//create Title and Class Name for the Platform Window
	window->title = (char*)malloc(strlen(viewStrBase) + strlen(indexStr) + 1);
	memcpy(window->title, viewStrBase, strlen(viewStrBase));
	window->title[strlen(viewStrBase)] = '\0';

	//itoa(viewIndex+1, indexStr, 10);
	strcat(window->title, indexStr);
	window->title[strlen(viewStrBase) + strlen(indexStr)] = '\0';

	window->className = (char*)malloc(strlen(window->title) + strlen(classStrSuffix) + 1);
	memcpy(window->className, window->title, strlen(window->title));
	window->className[strlen(window->title)] = '\0';

	strncat(window->className, classStrSuffix, strlen(classStrSuffix));
	window->className[strlen(window->title) + strlen(classStrSuffix)] = '\0';

	//printf("\nview index = %s\n", indexStr);
	printf("\nview title = %s\n", window->title);

	//printf("\nsize = %d\n", strlen(classStrSuffix));
	printf("\nclassName = %s\n", window->className);

#endif 

}

void createWin32RenderWindow()
{


	//ask ms windows what the graphics refresh rate is
	DEVMODE dmScreenSettings;					// device mode
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dmScreenSettings);
	int framesPerSecond = dmScreenSettings.dmDisplayFrequency;
	printf("framesPerSecond = %d\n", framesPerSecond);
	int frameCount = 0;

	//RGBA color1 = {1.0f,1.0f,1.0f,1.0f};	//white
	//RGBA color2 = {0.0f,0.0f,0.0f,0.0f};	//black

	//RGBA * color = &color1;

	//ZeroMemory( glWindow, sizeof(WNDCLASSEX) );

	memset(&glWindow, 0, sizeof(glWindowContext));

	//fill a struct defining desired window properties
	//glWindowContext glWindow;
	glWindow.originx = glWindow.originy = 0;

	glWindow.rootMenu = 0;
	glWindow.width = g_display.resolution.x;
	glWindow.height = g_display.resolution.y;


	glWindow.windowedWidth = 1920;
	glWindow.windowedHeight = 1080;
	glWindow.fullscreen = false;
	glWindow.vsync = true;


	glWindow.bitdepth = 24;
	glWindow.screenmode = WINDOWED;
	glWindow.colorBufferDepth = 32;
	glWindow.depthBufferDepth = 24;	//16, 24, or 32
	glWindow.stencilBufferDepth = 8;
	glWindow.doubleBuffer = true;
	glWindow.offscreenBuffer = false;

	glWindow.sampleFactor = 1;

	glWindow.eventQueue = processWin32WindowMessageCallback;
	//if( glWindow.screenmode = WINDOWED )
	//{

	//}

	glWindow.pixelFormatIndex = 0;	//the pixel format that opengl has chosen for the window context
	glWindow.gcMajorVersion = 1;
	glWindow.gcMinorVersion = 2;
	glWindow.gcProfile = CORE;

	glWindow.shouldIdle = false;

	//createa a unique window class name and window title before registering the window class
	setWindowClassName(&glWindow, 0);

	//OpenGL profile has no meaning until version 3.0, and COMPATIBILITY profile has no meaning until 3.2
	//choosing compatibility profile will always drive major versions 3 & 4 to the lastest version 4 minor version for NVIDIA GTX gpu
	//if( glWindow.gcMajorVersion >= 3 )
	//	glWindow.gcProfile = COMPATIBILITY;		

	createPlatformWindow(&glWindow);
}




void createWin32WindowMenu()
{
	//root menu
	glWindow.rootMenu = CreateMenu();

	//sub-menus
	HMENU fileMenu = CreateMenu();
	HMENU playbackMenu = CreateMenu();
	HMENU windowMenu = CreateMenu();
	//HMENU envMenu = CreateMenu(); 
	//HMENU temperaturesMenu = CreateMenu(); 

	AppendMenuW(fileMenu, MF_STRING, IDM_FILE_IMPORT_VIDEO, L"&Import Video...");
	//AppendMenuW(fileMenu, MF_STRING, IDM_FILE_EXPORT_TEMPERATURES_TO_SOURCE_FORMAT, L"&Export Temperatures to Source Geometry");
	//AppendMenuW(fileMenu, MF_STRING, IDM_FILE_EXPORT_TO_TDF, L"&Export to TDF");
	AppendMenuW(fileMenu, MF_SEPARATOR, 0, NULL);
	AppendMenuW(fileMenu, MF_STRING, IDM_FILE_QUIT, L"&Quit\tEsc");
	AppendMenuW(glWindow.rootMenu, MF_POPUP, (UINT_PTR)fileMenu, L"&File");


	AppendMenuW(playbackMenu, MF_STRING, IDM_PLAYBACK_PLAY_PAUSE, L"&Play\t[Space]");
	AppendMenuW(playbackMenu, MF_STRING, IDM_PLAYBACK_TOGGLE_VSYNC, L"&Disable Vsync\tV");
	AppendMenuW(glWindow.rootMenu, MF_POPUP, (UINT_PTR)playbackMenu, L"&Playback");

	AppendMenuW(windowMenu, MF_STRING, IDM_WINDOW_TOGGLE_BORDER, L"&Toggle Border\tB");
	AppendMenuW(windowMenu, MF_STRING, IDM_WINDOW_TOGGLE_FULLSCREEN, L"&Toggle Fullscreen\tF");
	AppendMenuW(glWindow.rootMenu, MF_POPUP, (UINT_PTR)windowMenu, L"&Window");

	//AppendMenuW(temperaturesMenu, MF_STRING, IDM_TEMPERATURES_SET_CSV_FILE_PATH, L"&Set CSV file...");
	//AppendMenuW(glWindow.rootMenu, MF_POPUP, (UINT_PTR)temperaturesMenu, L"&Temperatures");

	SetMenu(glWindow.hwnd, glWindow.rootMenu);
}

unsigned int __stdcall win32RenderWindowEventLoop()
{
	char indexStr[256];
	BOOL bRet = true;
	MSG msg;
	memset(&msg, 0, sizeof(MSG));

	//start the hwnd message loop, which is event driven based on platform messages regarding interaction for input and the active window
	//while (bRet)
	//{
		//pass NULL to GetMessage listen to both thread messages and window messages associated with this thread
		//if the msg is WM_QUIT, the return value of GetMessage will be zero
	while (1)//(bRet = GetMessage(&msg, NULL, 0, 0)))
	{
		bRet = GetMessage(&msg, NULL, 0, 0);

		//if (bRet == -1)
		//{
			// handle the error and possibly exit
		//}

		//bRet = GetMessage(&msg, view->hwnd, 0, 0);
		//printf("\nbRet = %d\n", bRet);


		//if a PostQuitMessage is posted from our wndProc callback,
		//we will handle it here and break out of the view event loop and not forward bakc to the wndproc callback

		if (bRet == 0 || msg.message == WM_QUIT)
		{
			//if the view wasn't destroyed we want to maintain
			if (glWindow.shouldClose)
				break;
			else
				printf("\nview did not clsoe\n");

			ReleaseDC(glWindow.hwnd, glWindow.hdc);	//release the device context backing the hwnd platform window
			glWindow.hwnd = NULL;
			glWindow.hdc = NULL;

			//if( view->className )
			//	free(view->className);

			//view->className = NULL;

			//check if we just closed the only remaining only remaining view 
			//if so, post a message to the simulation thread (

			//itoa(rand(), indexStr, 10);

			//view->className = (char*)malloc(strlen("WTFWindow") + strlen(indexStr) + 1);
			//memcpy(view->className,"WTFWindow", strlen("WTFWindow"));
			//view->className[strlen("WTFWindow")] = '\0';//currently on the main thread)

			//strcat(view->className, indexStr);
			//view->className[strlen("WTFWindow") + strlen(indexStr) ] = '\0';

			//printf("view->className = %s", view->className);
			//createa a unique window class name and window title before registering the window class
			//setWindowClassName(&glWindow, 0);

			//recreate the OS specific platform window
			//crgc_view_create_platform_window(view);
			//createWin32RenderWindow();
			createPlatformWindow(&glWindow);

			//set the window title or update the text as desired
			SetWindowText(glWindow.hwnd, "Async Video Player");

			PostThreadMessage(glWindow.renderThreadID, AVP_WINDOW_RESIZE, AVP_WINDOW_RESIZE_OPTION_BORDERLESS, MAKELPARAM( glWindow.width, glWindow.height));

			//the menu was already created the first time we created the window
			//we just need to set it on the new window now -- actually this doesn't work we have to recreate the menu
			//SetMenu(glWindow.hwnd, glWindow.rootMenu);
			createWin32WindowMenu();

			bool hasMenu = glWindow.rootMenu;
			bool hasTitleBar = true;
			ResizeWin32Window(glWindow.hwnd, glWindow.windowedWidth, glWindow.windowedHeight, hasTitleBar, hasMenu);


			//notify the render thread the the platform window has been recreated
			PostThreadMessage(glWindow.renderThreadID, AVP_WINDOW_RECREATE, 0, 0);

		}


		//else if( msg.message == WM_ENTERSIZEMOVE || msg.message == WM_EXITSIZEMOVE)
		//	break;
		//else if( msg.message == WM_SIZE || msg.message == WM_SIZING)
		//	break;
		//else if( msg.message == WM_NCCALCSIZE )
		//	break;
		//else if( msg.message == WM_NCPAINT )
		//	break;

		//pass on the messages to the __main_event_queue for processing
		//TranslateMessage(&msg);
		DispatchMessage(&msg);

		memset(&msg, 0, sizeof(MSG));

	}

	
	fprintf(stdout, "\nEnd AsyncVideoPlayer::win32WindowEventLoop\n");
	//if we have allocated the view ourselves we can expect the graphics context and platform window have been deallocated
	//and we can destroy the memory associatedk with the view structure
	//TO DO:  clean up view struct memory
	//ReleaseDC(glWindow.hwnd, view->hdc);	//release the device context backing the hwnd platform window
	
	//when drawing we must explitly release the DC handle
	//fyi, hwnd and hdc were assigned to the glWindowContext in CreateWnd
	ReleaseDC(glWindow.hwnd, glWindow.hdc);

	if( glWindow.title )
		free(glWindow.title);
	glWindow.title = NULL;

	if( glWindow.className )
		free(glWindow.className);
	glWindow.className = NULL;


	//Since our glWindow and gpgpuWindow windows are managed by separate
	//instances of WndProc callback on separate threads,
	//but they are both closed at the same time
	//there is a race condition between when the windows get destroyed
	//and thus break out of their above GetMessage pump loops

	//we will let the gpgpuWindow control thread set the _gpgpuIdleEvent
	//to always be signaled when it is finished closing its thread
	//and have this glWindow control thread wait until that event is signaled
	//as open to ensure both get closed before the application closes
	WaitForSingleObject(_gpgpuIdleEvent, INFINITE);

	// let's play nice and return any message sent by windows
    return (int)msg.wParam;
}




unsigned int __stdcall win32GPGPUWindowEventLoop()
{
	BOOL bRet = true;
	MSG msg;
	memset(&msg, 0, sizeof(MSG)); 

	//pump the win32 window event loop messages
	//that is, pass them on to WndProc for processing
	while( (bRet = GetMessage( &msg, NULL, 0, 0 )) != 0)
	{ 
		if (bRet == -1)
		{
			// handle the error and possibly exit
			break;
		}
		else
		{
			TranslateMessage(&msg); 
			DispatchMessage(&msg); 
		}
	}
	
	fprintf(stdout, "\nEnd AsyncVideoPlayer::win32GPGPUWindowEventLoop\n");
	//if we have allocated the view ourselves we can expect the graphics context and platform window have been deallocated
	//and we can destroy the memory associatedk with the view structure
	//TO DO:  clean up view struct memory
	//ReleaseDC(glWindow.hwnd, view->hdc);	//release the device context backing the hwnd platform window
	
	//when drawing we must explitly release the DC handle
	//fyi, hwnd and hdc were assigned to the glWindowContext in CreateWnd
	ReleaseDC(gpgpuWindow.hwnd, gpgpuWindow.hdc);

	if( gpgpuWindow.title )
		free(gpgpuWindow.title);
	gpgpuWindow.title = NULL;

	if( gpgpuWindow.className )
		free(gpgpuWindow.className);
	gpgpuWindow.className = NULL;

	//set the gpgpuIdle event to always be signaled
	//to notify the glWindow control thread that its ok to stop waiting and close the application
	SetEvent(_gpgpuIdleEvent);

	// let's play nice and return any message sent by windows
    return (int)msg.wParam;
}


void createWin32GPGPUWindow()
{
	//ask ms windows what the graphics refresh rate is
	DEVMODE dmScreenSettings;					// device mode
    EnumDisplaySettings (NULL, ENUM_CURRENT_SETTINGS, &dmScreenSettings);
	int framesPerSecond = dmScreenSettings.dmDisplayFrequency;
	printf("framesPerSecond = %d\n", framesPerSecond);
	int frameCount = 0;

	//RGBA color1 = {1.0f,1.0f,1.0f,1.0f};	//white
	//RGBA color2 = {0.0f,0.0f,0.0f,0.0f};	//black

	//RGBA * color = &color1;

	//ZeroMemory( glWindow, sizeof(WNDCLASSEX) );

	memset(&gpgpuWindow, 0, sizeof(glWindowContext));

	//fill a struct defining desired window properties
	//glWindowContext glWindow;
	gpgpuWindow.originx = glWindow.originy = 0;
	gpgpuWindow.width = g_display.resolution.x;
	gpgpuWindow.height = g_display.resolution.y;


	gpgpuWindow.windowedWidth = g_display.resolution.x;
	gpgpuWindow.windowedHeight = g_display.resolution.y;
	gpgpuWindow.screenmode = WINDOWED;
	gpgpuWindow.vsync = false;

	gpgpuWindow.bitdepth = 24;
	gpgpuWindow.colorBufferDepth = 32;
	gpgpuWindow.depthBufferDepth = 24;	//16, 24, or 32
	gpgpuWindow.stencilBufferDepth = 8;
	gpgpuWindow.doubleBuffer = true;
	gpgpuWindow.offscreenBuffer = false;

	gpgpuWindow.sampleFactor = 1;
	gpgpuWindow.eventQueue = processWin32WindowMessageCallback;

	gpgpuWindow.pixelFormatIndex = 0;	//the supported pixel format that the OS has chosen for the window context
										//we'll initialize it to zero, though it has no meaning until the OS changes the value
	gpgpuWindow.gcMajorVersion = 1;	
	gpgpuWindow.gcMinorVersion = 2;
	gpgpuWindow.gcProfile = CORE;

	gpgpuWindow.shouldIdle = true; //initiate the thread to idle after startup

	//OpenGL profile has no meaning until version 3.0, and COMPATIBILITY profile has no meaning until 3.2
	//choosing compatibility profile will always drive major versions 3 & 4 to the lastest version 4 minor version for NVIDIA GTX gpu
	//if( glWindow.gcMajorVersion >= 3 )
	//	glWindow.gcProfile = COMPATIBILITY;		

	//for non-WinMain applications, the hinstance is not provided as an application parameter, get the hinstance as follows:	
	
	//gpgpuWindow.hinstance = (HINSTANCE)GetModuleHandle(NULL);


	//createa a unique window class name and window title before registering the window class
	setWindowClassName(&gpgpuWindow, 1);

	//OpenGL profile has no meaning until version 3.0, and COMPATIBILITY profile has no meaning until 3.2
	//choosing compatibility profile will always drive major versions 3 & 4 to the lastest version 4 minor version for NVIDIA GTX gpu
	//if( glWindow.gcMajorVersion >= 3 )
	//	glWindow.gcProfile = COMPATIBILITY;		

	createPlatformWindow(&gpgpuWindow);

}


unsigned int __stdcall createWin32GPGPUWindowThread(void * params)
{
	createWin32GPGPUWindow();

	//because we need to create contexts on the same thread when calling wglSHareLists
	//we will comment this out and move starting the async gpgpu callback to the glWindow render thread
	//gpgpuWindow.renderThreadID = 0;
	//_beginthreadex(NULL, 0, asyncGPGPUCallback, 0, 0, &gpgpuWindow.renderThreadID);

	win32GPGPUWindowEventLoop();


	return 0;
}


//this must persist throughout the entire applciation in order for the 
static cr_display_sync_event_params dispSyncEventParams;

//----------------------------------------------------------------------
// not multi-thread safe (intended to be called at app startup / WinMain)
void startDisplaySyncThread(avp_display * display, int threadPriority) 
{
	if (!display->vsyncServiceRunning) 
	{
		
		//TO DO:  error checking on threadPriority param

		//set a status boolean indicating that vsync "service" is running
		display->vsyncServiceRunning = true;

		// protects such a small section of code, spining will always be much faster than actual waiting in the OS
        //InitializeCriticalSectionAndSpinCount(&_vilock,8);

		//create Windows events associated with the front and back of a window buffer respectively that can be waited on for thread synchronization
		//for idling a thread and waking up when vertical retrace is issued

        //_hVFront = CreateEvent(NULL, TRUE, FALSE, NULL);
        //_hVBack = CreateEvent(NULL, TRUE, FALSE, NULL);
		
		//create a semaphore that counts to one and is currently in an unsignaled state
		//_verticalRetraceSemaphore = CreateSemaphore(NULL, 0, 1, NULL);

		//pass thread priority as param to __stdcall void * parameter

		display->verticalRetraceEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		dispSyncEventParams.threadPriority = threadPriority;
		dispSyncEventParams.syncEvent = display->verticalRetraceEvent;
        display->hVThread = (HANDLE)_beginthreadex(NULL, 0, cr_start_display_sync_listener, &dispSyncEventParams, 0, &display->hVThreadID);
	}
}

void initializeDisplay()
{
	//The following is supposed to give the display resolution, 
	//but it actually just returns the maximum number of virtual pixels the OS thinks the primary display has based on the dpi aware setting
	//E.g. a display with a maximum resolution of 3840 x 2160, but set to a resolution of 2160 x 1440 will still return values of 3840 x 2160
	//screenWidth = GetSystemMetrics(SM_CXSCREEN);
	//screenHeight = GetSystemMetrics(SM_CYSCREEN);

	//get the display resolution using CoreRender cr_display api, we'll need it for custom view resizing
	g_display.resolution = cr_display_get_resolution();
	g_display.dimensions = cr_display_get_dimensions();

	printf("\n Display Resolution:  %lu x %lu\n", g_display.resolution.x, g_display.resolution.y);

	//start the high priority, highly accurate asynchronous display vertical retrace update thread
	startDisplaySyncThread(&g_display, THREAD_PRIORITY_TIME_CRITICAL);
	//OR start it as a process, which can be elevated to REALTIME_PRIORITY_CLASS if this app is run as administrator!!!
	//start the vsync callback as its own real-time process
	//startDisplaySyncProcess(&disp_sync_proc_info, REALTIME_PRIORITY_CLASS);
}

void cleanupDisplay()
{


}


void initializeApplicationState()
{

}

void initializeMemory()
{
	//initialize a circular buffer for thread safe sharing of buffer data between threads
	avt_circ_buffer_init(&_renderTextureSync, k_circ_buffer_size);
	avt_circ_buffer_init(&_loadTextureSync, k_circ_buffer_size);
}

void cleanupMemory()
{
	//cleanup circular buffer
	avt_circ_buffer_cleanup(&_renderTextureSync);

	if( g_videoSourcePath )
		free(g_videoSourcePath);
	g_videoSourcePath = NULL;


	//cleanup avp_video memory
	if(  g_sourceVideo.sourcepath )
		 free(g_sourceVideo.sourcepath);
}

//#include <iostream>


/// \fn void main()
/// \brief main
///
/// Description: Main for the application
///
int main(int argc, char **argv) 
{

	//std::cout << "Foo returns " << foo() << "\n";
	//system("pause");

	//IMPORTANT:  Must set DPI aware to use High Resolution displays of UHD quality or greater
	//SetProcessDpiAwareness( PROCESS_PER_MONITOR_DPI_AWARE );
	SetProcessDPIAware();

	//set the process priority class
	setProcessPriority(HIGH_PRIORITY_CLASS);

	initializeDisplay();
	initializeMemory();
	//intializeApplicationState();
	
	//2 create the platform specific OS window, needed to create a hardware accelerated graphics context for GPGPU rendering
	createWin32RenderWindow();

	//3 create menu for window
	createWin32WindowMenu();
	
	//the window will need to be resized to the desired windowedWidth
	bool hasMenu = glWindow.rootMenu;
	bool hasTitleBar = true;
	ResizeWin32Window(glWindow.hwnd, glWindow.windowedWidth, glWindow.windowedHeight, hasTitleBar, hasMenu);

	//center the window in the display
	//MoveWin32Window(glWindow.hwnd, g_display.resolution.x/2 - glWindow.windowedWidth/2, g_display.resolution.y/2 -  glWindow.windowedHeight/2);

	//launch a thread that will create a win32 window and control loop for a gpgpu window
	//
	gpgpuWindow.renderThreadID = 0;
	_beginthreadex(NULL, 0, createWin32GPGPUWindowThread, 0, 0, NULL);//&(gpgpuWindow.renderThreadID));

	
	//launch a thread that will create an opengl context for the window 
	//and run a display loop that will render to the window using opengl
	//the view will store a reference to its thread id assigned by the OS
	glWindow.renderThreadID = 0;
	_beginthreadex(NULL, 0, asyncRenderCallback, 0, 0, &(glWindow.renderThreadID));

	
	//run the render window control event loop on the main thread
	win32RenderWindowEventLoop();

	cleanupMemory();



	//printf("\nafter create window\n");
	//Patch CRT error on exit in debug build with this call
	//TerminateProcess(GetCurrentProcess(), EXIT_SUCCESS);

	return 0;
}