#ifndef WGL_UTILS_H
#define WGL_UTILS_H

// openGL headers (default opengl 1.1 headers provided by Microsoft with the OS)
// used for initial context creation

//#include <gl/gl.h>
//include gl func and extension loader
#include "crgc_gl_ext.h" //replaces glew
#include <gl/GLU.h> //for cross-platform capability best not to use GLU if it can be helped

#define VAR_TO_STRING(x) #x

// TO DO:  MOVE WINDOW NAME STRING APPLICATION SPECIFIC ITEMS elsewhere
// references to these items must be removed from the routines in this header first
//WINDOW NAME DEFINITIONS
#define WNDCLASSNAME	"AVPWindowClass"				// window class name
#define WNDNAME			"Async Video Player"			// string that will appear in the title bar


typedef enum glProfile
{
	CORE = WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
	COMPATIBILITY = WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB

}glProfile;

typedef enum glScreenModes
{
	WINDOWED = 0,
	BORDERLESS = 1
	//FULLSCREEN = 2

}glScreenModes;


typedef struct glWindowContext
{
	//allow client to tag views with thread identifiers, useful for messaging between threads
	unsigned int processThreadID;
	unsigned int loadThreadID;
	unsigned int renderThreadID;   
	unsigned int controlThreadID;

	int renderState;  //a var for enumerating what the window should render
	bool resize;
	bool shouldClose;
	bool shouldIdle;
	bool renderText;
	int textID;

	//abstract window properties
	int originx;		//the screen position of the window top left corner
	int originy;		//the screen position of the window top left corner
	int width;			//the actual width of the window at current
	int height;			//the actual height of the window at current

	int windowedWidth;	//store the desired width of the window when in minimized windowed state
	int windowedHeight; //store the desired height of the window when in minimzed windowed state

	int screenmode;		//options are WINDOWED and BORDERLESS
	bool fullscreen;
	long borderWidth;
	bool vsync;

	bool offscreenBuffer;
	bool doubleBuffer;

	int bitdepth;			//generally, 24
	int depthBufferDepth;	//16, 24, or 32
	int colorBufferDepth;	//generally, 32 for rgba
	int stencilBufferDepth; //depth + stencil of 24 + 8 is typical

	unsigned int sampleFactor;

	//OS specific window context handles ...
	HDC hdc;
	HWND hwnd;
	HGLRC hglrc;
	HINSTANCE hinstance;
	WNDCLASSEX wndclassex;
	HMENU rootMenu;
	WNDPROC eventQueue;

	//graphics library context properties
	unsigned int gcMajorVersion;
	unsigned int gcMinorVersion;
	unsigned char gcProfile;		//for opengl, options are CORE or COMPATIBILITY

	//graphics buffer window properties
	int pixelFormatIndex;

	//abstract window identifier properties
	char * title;
	char * className;


}glWindowContext;

//global window singleton
glWindowContext glWindow;
glWindowContext gpgpuWindow;


void PostShutdownMessage(void)
{
	//wglMakeCurrent(hdc, NULL);		// release device context in use by rc
	//wglDeleteContext(hglrc);		// delete rendering context

	PostQuitMessage(0);				// make sure the window will be destroyed

	//if (screenmode == FULLSCREEN)	// if FULLSCREEN, change back to original resolution
	//	SysRecoverDisplayMode();
}

void destroy_window_context(glWindowContext * window)
{
	//make sure the context we are releasing isn't current on any threads
	wglMakeCurrent(window->hdc, NULL);		// release device context in use by rc
	wglDeleteContext(window->hglrc);			// delete rendering context
	window->hglrc = NULL;
}

void destroy_window(glWindowContext * window)
{
	destroy_window_context(window);
	DestroyWindow(window->hwnd);			//send the destroy window message to the wndproc behind the hwnd platform window
	
	if( window->className )
		UnregisterClass(window->className, window->hinstance);
	else
		UnregisterClass(WNDCLASSNAME, window->hinstance);
	
}

//OpenGL Context Creation for MS Windows

//choose the closest available pixel format to the requested window definition
int SetGLPixelFormat (HDC & hdc)//glWindowContext &glWindow)
{
	/*
	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,
			PFD_TYPE_RGBA,
			glWindow.bitdepth,
			0,0,0,0,0,0,0,0,0,0,0,0,0, // useles parameters
			16,
			0,0,PFD_MAIN_PLANE,0,0,0,0
	};
	*/

	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
		PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
		32,                        //Colordepth of the framebuffer.
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24,                       //Number of bits for the depthbuffer
		8,                        //Number of bits for the stencilbuffer
		0,                        //Number of Aux buffers in the framebuffer.
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	//HDC hdc = GetDC(glWindow.hwnd);
	int indexPixelFormat = -1;
	// Choose the closest pixel format available
	if (!(indexPixelFormat = ChoosePixelFormat(hdc, &pfd)))
	{
		MessageBox(glWindow.hwnd, "Failed to find pixel format", "Pixel Format Error", MB_OK);
		PostShutdownMessage();
	}
	
	// Set the pixel format for the provided window DC
	if ( !SetPixelFormat(hdc, indexPixelFormat, &pfd) )
	{
		MessageBox(glWindow.hwnd, "Failed to Set Pixel Format", "Pixel Format Error", MB_OK);
		PostShutdownMessage();
	}

	return indexPixelFormat;
}

int SetGLPixelFormat2(HDC &hdc)
{
	const int attribList[] =
	{
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB, 32,
		WGL_DEPTH_BITS_ARB, 24,
		WGL_STENCIL_BITS_ARB, 8,
		0,        //End
	};

	// There are a number of extensions that have added new attributes for this function. The important ones that you might want to use are:
	// * WGL_ARB_pixel_format_float: Allows for floating-point framebuffers.
	// * WGL_ARB_framebuffer_sRGB: Allows for color buffers to be in sRGB format.
	// * WGL_ARB_multisample: Allows for multisampled framebuffers.
	// Once you have a pixel format number, you can set it just like any pixel format with SetPixelFormat.

	int pixelFormat = -1;
	UINT numFormats;

	BOOL success = wglChoosePixelFormatARB(hdc, attribList, NULL, 1, &pixelFormat, &numFormats);

	if( !success )
	{
		printf("\nwglChoosePixelFomratARB failed\n");
	}

	return pixelFormat;
}


//Call Resize to initialize the view and perspective
void glResize (int width, int height)
{

	//wglSwapIntervalEXT(1)

	if (height <= 0)
		height = 1;

	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	
	glLoadIdentity();

	gluPerspective(45.0f, (float)width/(float)height, 1.0f, 100000.0f);

	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();
}

void initGLWindow()
{
	glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
	glClearDepth(1.0f);
}


// WGLisExtensionSupported: This Is A Form Of The Extension For WGL
bool WGLisExtensionSupported(const char *extension)
{
	const char * p;
	const size_t extlen = strlen(extension);
	const char *supported = NULL;

	// Try To Use wglGetExtensionStringARB On Current DC, If Possible
	PROC wglGetExtString = wglGetProcAddress("wglGetExtensionsStringARB");

	if (wglGetExtString)
		supported = ((char*(__stdcall*)(HDC))wglGetExtString)(wglGetCurrentDC());

	// If That Failed, Try Standard Opengl Extensions String
	if (supported == NULL)
		supported = (char*)glGetString(GL_EXTENSIONS);

	// If That Failed Too, Must Be No Extensions Supported
	if (supported == NULL)
		return false;

	// Begin Examination At Start Of String, Increment By 1 On False Match
	for (p = supported; ; p++)
	{
		// Advance p Up To The Next Possible Match
		p = strstr(p, extension);

		if (p == NULL)
			return false;															// No Match

		// Make Sure That Match Is At The Start Of The String Or That
		// The Previous Char Is A Space, Or Else We Could Accidentally
		// Match "wglFunkywglExtension" With "wglExtension"

		// Also, Make Sure That The Following Character Is Space Or NULL
		// Or Else "wglExtensionTwo" Might Match "wglExtension"
		if ((p == supported || p[-1] == ' ') && (p[extlen] == '\0' || p[extlen] == ' '))
			return true;															// Match
	}
}


//choose the closest available pixel format to the requested window definition
int chooseGLPixelFormat(HDC hdc, HWND hwnd, int pixelFormat, int colorBufferDepth, int depthBufferDepth, int stencilBufferDepth, bool renderToBitmap, bool doubleBuffer)//glWindowContext &glWindow)
{
	//the pixel format index returned by the system call
	int pixelFormatIndex;

	PIXELFORMATDESCRIPTOR pfd;//=
	/*
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |  PFD_SUPPORT_COMPOSITION | PFD_DOUBLEBUFFER,    //Flags
		PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
		32,                        //Colordepth of the framebuffer.
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24,                       //Number of bits for the depthbuffer
		8,                        //Number of bits for the stencilbuffer
		0,                        //Number of Aux buffers in the framebuffer.
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};
	*/
	// set the pixel format for the DC, this tells Windows things like
		   // the color depth and so on that we wish to use in the context
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;

	if (!renderToBitmap)
	{
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_COMPOSITION;//|PFD_SUPPORT_OPENGL;//|PFD_DOUBLEBUFFER;
	}
	else
		pfd.dwFlags = PFD_DRAW_TO_BITMAP | PFD_SUPPORT_COMPOSITION;

	pfd.dwFlags = pfd.dwFlags | PFD_SUPPORT_OPENGL;

	if (doubleBuffer)
		pfd.dwFlags = pfd.dwFlags | PFD_DOUBLEBUFFER;

	//if (pixelFormat == CRGC_PIXEL_FORMAT_RGBA)
		pfd.iPixelType = PFD_TYPE_RGBA;
	//else
	//	pfd.iPixelType = PFD_TYPE_COLORINDEX;


	pfd.cColorBits = colorBufferDepth;         // BPP is ignored for windowed mode
	pfd.cDepthBits = depthBufferDepth;			// 16-bit z-buffer
	pfd.iLayerType = PFD_MAIN_PLANE;

	printf("\nchooseGLPixelFormat\n");
	//HDC hdc = GetDC(glWindow.hwnd);
	pixelFormatIndex = -1;
	// Choose the closest pixel format available
	if (!(pixelFormatIndex = ChoosePixelFormat(hdc, &pfd)))
	{
		MessageBox(hwnd, "Failed to find pixel format", "Pixel Format Error", MB_OK);
		PostShutdownMessage();
	}

	// Set the pixel format for the provided window DC
	if (!SetPixelFormat(hdc, pixelFormatIndex, &pfd))
	{
		MessageBox(hwnd, "Failed to Set Pixel Format", "Pixel Format Error", MB_OK);
		PostShutdownMessage();
	}

	return pixelFormatIndex;
}


#include <strsafe.h>

int chooseGLPixelFormatARB(HDC hdc, HWND hwnd, int pixelFormat, int colorBufferDepth, int depthBufferDepth, int stencilBufferDepth, int sampleFactor, bool renderToBitmap, bool doubleBuffer)
{
	//DWORD dwErr;
	PIXELFORMATDESCRIPTOR pfd;
	int wglPixelFormat;
	int pixelFormatIndex = -1;
	UINT numFormats;
	int numSampleBuffers;
	BOOL success = false;

	if (sampleFactor <= 1)
	{
		sampleFactor = 1;
		numSampleBuffers = 1;
	}
	else
		numSampleBuffers = 1;


	//if (pixelFormat == CRGC_PIXEL_FORMAT_COLOR_INDEX)
	//{
	//	wglPixelFormat = WGL_TYPE_COLORINDEX_ARB;
	//}
	//else
	//{
		wglPixelFormat = WGL_TYPE_RGBA_ARB;
	//}

	printf("\nchooseGLPixelFormatARB\n");

	if (sampleFactor > 1)
	{
		const int attribList[] =
		{
			WGL_DRAW_TO_WINDOW_ARB, (int)!renderToBitmap,
			WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
			//WGL_TRANSPARENT_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB, (int)doubleBuffer,
			WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
			WGL_COLOR_BITS_ARB, colorBufferDepth,
			WGL_DEPTH_BITS_ARB, depthBufferDepth,
			WGL_STENCIL_BITS_ARB, stencilBufferDepth,
			WGL_SAMPLE_BUFFERS_ARB, 1, //Number of buffers (must be 1 at time of writing)
			WGL_SAMPLES_ARB, sampleFactor,			  //Number of samples
			0,       //End
		};

		printf("\npixelFormat = CRGC_PIXEL_FORMAT_RGBA\n");

		//since this requires hdc it is probably necessary to call this on same thread that HWND was created on
		success = wglChoosePixelFormatARB(hdc, attribList, NULL, 1, &pixelFormatIndex, &numFormats);

	}
	else
	{
		const int attribList[] =
		{
			WGL_DRAW_TO_WINDOW_ARB, (int)!renderToBitmap,
			WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
			//WGL_TRANSPARENT_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB, (int)doubleBuffer,
			WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
			WGL_COLOR_BITS_ARB, colorBufferDepth,
			WGL_DEPTH_BITS_ARB, depthBufferDepth,
			WGL_STENCIL_BITS_ARB, stencilBufferDepth,
			//WGL_SAMPLE_BUFFERS_ARB, numSampleBuffers, //Number of buffers (must be 1 at time of writing)
			//WGL_SAMPLES_ARB, sampleFactor,			  //Number of samples
			0,        //End
		};
		//since this requires hdc it is probably necessary to call this on same thread that HWND was created on
		success = wglChoosePixelFormatARB(hdc, attribList, NULL, 1, &pixelFormatIndex, &numFormats);

	}



	//success = wglChoosePixelFormatARB(hdc, attribList, NULL, 1, &pixelFormatIndex, &numFormats);
// There are a number of extensions that have added new attributes for this function. The important ones that you might want to use are:
// * WGL_ARB_pixel_format_float: Allows for floating-point framebuffers.
// * WGL_ARB_framebuffer_sRGB: Allows for color buffers to be in sRGB format.
// * WGL_ARB_multisample: Allows for multisampled framebuffers.
// Once you have a pixel format number, you can set it just like any pixel format with SetPixelFormat.



	if (!success)
	{
		//DWORD dwErr;
		char tmp[256];

		LPVOID lpMsgBuf;
		LPVOID lpDisplayBuf;
		DWORD dw = GetLastError();
		DWORD dw2;

		dw2 = FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL);


		if (dw2 == 0)
			printf("\nFormat Message failed\n");
		// Display the error message and exit the process

		dw2 = GetLastError();
		lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
			(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)"wglChoosePixelFormatARB") + 40) * sizeof(TCHAR));
		StringCchPrintf((LPTSTR)lpDisplayBuf,
			LocalSize(lpDisplayBuf) / sizeof(TCHAR),
			TEXT("%s failed with error %d: %s"),
			"wglChoosePixelFormatARB", dw2, lpMsgBuf);
		MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

		LocalFree(lpMsgBuf);
		LocalFree(lpDisplayBuf);

		//dwErr = GetLastError();// != ERROR_CLASS_ALREADY_EXISTS
		//sprintf(tmp, "Failed to find valid pixel format.\n\nError = %u", dwErr);
		//MessageBox(hwnd, tmp, "wglChoosePixelFormatARB Failed", MB_OK);


		PostShutdownMessage();
		//printf("\nwglChoosePixelFomratARB failed\n");
	}

	/* Is this the best solution? */
	DescribePixelFormat(hdc, pixelFormatIndex, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

	// Set the pixel format for the provided window DC
	if (!SetPixelFormat(hdc, pixelFormatIndex, &pfd))
	{
		MessageBox(hwnd, "Failed to Set Pixel Format", "Pixel Format Error", MB_OK);
		PostShutdownMessage();
	}

	return pixelFormatIndex;
}

 
HGLRC createOpenGLContext(glWindowContext * view, int majorVersion, int minorVersion)
{

	HGLRC hglrc;		//the windows render context handle (and also the opengl context handle )
	int numExtensions, i;
	int major; int minor, samples, numTextureUnits;
	const GLubyte * glVersion;
	const GLubyte * glExtensions;
	const GLubyte * wglExtensions;

	hglrc = NULL;
	numExtensions = 0;


	if (view->hglrc)
	{
		destroy_window_context(view);
	}

	//Note:  On windows, a OpenGL 1.1 context must first be created to get the wgl extensions
	//       So that a higher OpenGL version (> 1.1) context may be created if desired 
	//		(History:  that is just the way that Microsoft implemented wgl and then they decided 
	//		not to ship updated versions of opengl past version 1.1 because they have proprietary interest in Direct3D instead)

	if( majorVersion == 0 && minorVersion == 0 ) //the only opengl version shipped by windows
	{
		// select pixel format using Windows provided ChoosePixelFormat call, needed before wglCreateContext call
		view->pixelFormatIndex = chooseGLPixelFormat(view->hdc, view->hwnd, view->pixelFormatIndex, view->colorBufferDepth, view->depthBufferDepth, view->stencilBufferDepth, view->offscreenBuffer, view->doubleBuffer);


		// create the opengl 1.1 rendering context using the system provided wgl api
				//hglrc = wglCreateContext(hdc);

		if ((hglrc = wglCreateContext(view->hdc)) == NULL)
		{
			MessageBox(view->hwnd, "Failed to Create the OpenGL Rendering Context", "OpenGL Rendering Context Error", MB_OK);
			PostShutdownMessage();
			return NULL;
		}


		// make hglrc current rc
		if ((wglMakeCurrent(view->hdc, hglrc)) == false)
		{
			MessageBox(view->hwnd, "Failed to Make OpenGL Rendering Context Current", "OpenGL Rendering Context Error", MB_OK);
			PostShutdownMessage();
			return NULL;
		}

		//query the glVersion as a parsable string using opengl 1.1 api functions
		/*const GLubyte * */glVersion = glGetString(GL_VERSION);
		printf("\nGL_VERSION:\t%s\n", glVersion);

		//query the gl version numbers direclty as integers
		glGetIntegerv(GL_MAJOR_VERSION, &major);
		glGetIntegerv(GL_MINOR_VERSION, &minor);

		printf("\nGL_VERSION:  %d.%d\n", major, minor);

		//query the available extensions as a parsable string using the opengl 1.1. api functions
		/*const GLubyte **/ glExtensions = glGetString(GL_EXTENSIONS);//wglGetProcAddress("wglGetExtensionsStringARB");
		printf("\nGL_EXTENSIONS:\n\n%s\n", glExtensions);

		//query the max sample rate
		glGetIntegerv(GL_MAX_SAMPLES, &samples);
		printf("\nGL_MAX_SAMPLES:	%d\n", samples);
		
		//glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &samples);
		//printf("\nGL_MAX_SAMPLES:	%d\n", samples);

		glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &numTextureUnits);
		printf("\nGL_MAX_TEXTURE_IMAGE_UNITS:	%d\n", numTextureUnits);

		//load opengl extensions for functions beyond opengl 1.1
		//but only if our desired version is beyond 1.1
		//if( !(majorVersion == 1 && minorVersion == 1 ) )
		//{
			//load desired Win32 OpenGL (WGL "wiggle") functions and extensions
			//this only needs to be done once on the context creation thread after the dummy context has been created, 
			//if using opengl > 1.1 and desired pixel format or other feature is not exposed by standard wglCreateContext function
		if (!crgc_wgl_ext_init())
			printf("\ncrgc_wgl_ext_init failed!\n");
		//}
	
		
	}
	else //must load contexts for versions greater than opengl 1.1 manually using WGL OpenGL Architecture Review Board (ARB) extensions, after loading a "dummy" context from the version shipped by windows
	{
		//First, assuming extension functions have already been loaded, such as with GLEW, we will check the wgl extensions
		//that opengl tell us are available, since our context creation will depend on having certain extensions available
		//[ i.e. if the extension isn't present, then we know our function loader failed to load that opengl function and we shouldn't use that function :) ]
		wglExtensions = NULL;

		//list the available wgl Architecture Review Board Extensions, requires a valid current wgl arb created context
		//must wrap this call in wglGetProcAddress because cannot rely on wglGetExetensionsStringEXT or glGetString to account for it properly
		//check the result of this one, since it usually returns NULL
		//FYI, calling opengl extension functions before loading them causes crash	
		if (wglGetProcAddress("wglGetExtensionsStringARB"))
		{
			/*const GLubyte * */wglExtensions = wglGetExtensionsStringARB();
		}

		if (!wglExtensions)
		{
			//list the available extensions provided by the hardware manufacturer (i.e. exposed by the driver of the graphics card for Windows OS rather than by Microsoft or Khronos ARB extension)
			wglExtensions = wglGetExtensionsStringEXT();
		}

		printf("\nWGL_EXTENSIONS:\n\n%s\n", wglExtensions);

		//query the extensions iteratively, (fyi, glGetStringi is a glext extension function)
		numExtensions = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

		for (i = 0; i < numExtensions; i++)
		{
			printf("\n%s", glGetStringi(GL_EXTENSIONS, i));

		}

		if (!WGLisExtensionSupported("WGL_ARB_multisample"))
			printf("\nWGL_ARB_multisample failure to load\n");
		else if (WGLisExtensionSupported("WGL_ARB_multisample"))
			printf("\nWGL_ARB_multisample is supported\n");
		else
			printf("\n Hello ? WTF?!\n");



		// select pixel format using wglChoosePixelFormatARB call, needed before wglCreateContextARB call
		//do this separately from the create context function instead, so it is guaranteed to happen on the same thread the platform window (HWND) was created on
		view->pixelFormatIndex = chooseGLPixelFormatARB(view->hdc, view->hwnd, view->pixelFormatIndex, view->colorBufferDepth, view->depthBufferDepth, view->stencilBufferDepth, view->sampleFactor, view->offscreenBuffer, view->doubleBuffer);

		//To query WGL extensions, you must first create an OpenGL context. Yes, that seems backwards, but that's how it is. 
		//To get the list of WGL extensions, you must create a context, make it current, and call wglGetProcAddress("wglGetExtensionsStringARB"). 
		//If this function does not return a valid pointer (see above for how to check), then the list of WGL extension strings is not available. 
		//If this pointer is invalid, then the OpenGL extension string may contain some WGL extensions. However, this is very unlikely, as this is an old and widely-implemented extension.
		//This function returns a string like glGetString(GL_EXTENSIONS): a space-separated list of extensions. So make sure to parse it correctly and don't copy it into a fixed-length buffer.
		//WGL function retrieval does require an active, current context. However, the use of WGL functions do not. Therefore, you can destroy the context after querying all of the WGL extension functions.
		//Function typedefs for WGL extensions are available in wglext.h.

		// Request an OpenGL context version + profile
		//if maximum version is desired, simply do not specify version
		//if attribs is NULL, wglCreateContextAttribsARB will have same effect as wglCreateContext(hdc)
		//which will give the maximum opengl version available in compatibility mode

		//printf("CR_VAR_TO_STRING(GL_ARB_create_context) = %s", CR_VAR_TO_STRING(WGL_ARB_create_context));

		//check that the context lists WGL_ARB_create_context ext as present
		//if so, we can specify attributes when creating our context
		if (strstr((char*)wglExtensions, VAR_TO_STRING(WGL_ARB_create_context)) != NULL)
		{
			int attribs[] =
			{
				// Ask for OpenGL Version
				WGL_CONTEXT_MAJOR_VERSION_ARB, majorVersion,
				WGL_CONTEXT_MINOR_VERSION_ARB, minorVersion,//glWindow.gcMinorVersion,
				// Uncomment this for forward compatibility mode
				//WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
				// Uncomment this for Compatibility profile
				//WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
				// We are using Core profile here
				WGL_CONTEXT_PROFILE_MASK_ARB, view->gcProfile,
				0
			};

			// create the opengl 1.1 rendering context using the system provided wgl api
			//the second parameter can be used to specify this wgl window should share context wtih another previously created window
			if ((hglrc = wglCreateContextAttribsARB(view->hdc, 0, attribs)) == NULL)
			{

				fprintf(stderr, "\nFailed to Create the OpenGL Rendering Context using ARB Extension.  Falling back to wglCreateContext.\n");
				MessageBox(view->hwnd, "Failed to Create the OpenGL Rendering Context using ARB Extension.  Falling back to wglCreateContext.", "OpenGL Rendering Context Error", MB_OK);
				//PostShutdownMessage();
				//return NULL;

				//fallback to wglCreateContext
				if ((hglrc = wglCreateContext(view->hdc)) == false)
				{
					fprintf(stderr, "\nFailed to Create the OpenGL Rendering Context after falling back to wglCreateContext. Posting Shutdown Message.\n");
					MessageBox(view->hwnd, "Failed to Create the OpenGL Rendering Context after falling back to wglCreateContext. Posting Shutdown Message.", "OpenGL Rendering Context Error", MB_OK);
					PostShutdownMessage();
					return NULL;
				}
			}

		}
		else
		{
			printf("\nWGL_ARB_create_context extension unavailable.  Choosing OpenGL Version is not possible.\n");

		}

		// make hglrc current rc
		if ((wglMakeCurrent(view->hdc, hglrc)) == false)
		{
			MessageBox(view->hwnd, "Failed to Make OpenGL Rendering ARB Context Current", "OpenGL Rendering Context Error", MB_OK);
			PostShutdownMessage();
			return NULL;
		}

		//query the gl version numbers direclty as integers
		glGetIntegerv(GL_MAJOR_VERSION, &major);
		glGetIntegerv(GL_MINOR_VERSION, &minor);

		printf("\nGL_VERSION:  %d.%d\n", major, minor);

		//query the max sample rate
		glGetIntegerv(GL_MAX_SAMPLES, &samples);
		printf("\nGL_MAX_SAMPLES_ARB:	%d\n", samples);


	
	}

	return hglrc;
}

void destroyOpenGLContext()
{
	//cleanup the wgl opengl context
	wglMakeCurrent(glWindow.hdc, NULL);		// make sure the context being released is NOT the current context
	wglDeleteContext(glWindow.hglrc);		// delete rendering context
}

/*
//create an opengl window using a Windows API HWND after a MS window has been created
void createWindowGraphicsContext(HWND &hwnd, HGLRC &hglrc)
{
	// get device context using the Window Process provided Window Handle (HWND)
	HDC hdc;
	if ((hdc = GetDC(hwnd)) == NULL)
	{
		MessageBox(hwnd, "Failed to Get the Window Device Context", "Device Context Error", MB_OK);
		PostShutdownMessage();
		//break;
		return;
	}

	//all opengl contexts must create a wgl OpenGL 1.1 context to begin with
	//and then create another context after loading extensions if needed
	//for nvidia this will create a compatibility profile with the latest opengl version (e.g. 4.5.0)
	createOpenGLContext(hdc, hwnd, hglrc, 1, 1);


	//load opengl extensions for functions beyond opengl 1.1
	//but only if our desired version is beyond 1.1
	if( !(glWindow.gcMajorVersion == 1 && glWindow.gcMinorVersion == 1 ) )
	{
		//load desired OpenGL functions and extensions
		bool success = gl_lite_init();

		if (!success )
		printf("\ngl_lite_init failed!\n");

		//if opengl version >= 3 (or may just equal to 3) and we don't want backward compatibility
		//we can specify this with the extension wgl_arb_create_context
		//if opengl version desired is < 3 there is no need to recreate a context
		//the functionality we need is already loaded through extension functions
		if( (glWindow.gcMajorVersion >= 3) )
		{
			//assume that a previous opengl 1.1 context was loaded and we need to destroy it, so we can create a new context using the extension functions
			wglMakeCurrent(glWindow.hdc, NULL);		// must make the wgl context NOT the current context before releasing
			wglDeleteContext(glWindow.hglrc);		// delete rendering context
			createOpenGLContext(hdc, hwnd, hglrc, glWindow.gcMajorVersion, glWindow.gcMinorVersion);
		}

	}


	//glResize(glWindow.width, glWindow.height);

	// initialize OpenGL before showing the window
	//glViewport(0, 0, glWindow.width, glWindow.height);
	//initGLWindow();

	// everything went OK, show the window
	//ShowWindow(hwnd, SW_SHOW);
	//UpdateWindow(hwnd);
}
*/




char * openFileDialog(HWND hwnd, char ** filepathBuffer, const char * filterExtensions)
{
	OPENFILENAME ofn;       // common dialog box structure
	char szFile[1024];       // buffer for file name
	//HWND hwnd;              // owner window
	HANDLE hf;              // file handle

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
	// use the contents of szFile to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = filterExtensions;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	// Display the Open dialog box. 

	if (GetOpenFileName(&ofn)==TRUE) 
	{
		if( *filepathBuffer )
			*filepathBuffer = (char*)realloc(*filepathBuffer, (strlen(ofn.lpstrFile)+1) * sizeof(char));
		else
			*filepathBuffer = (char*)malloc((strlen(ofn.lpstrFile)+1) * sizeof(char));
		
		strcpy(*filepathBuffer, ofn.lpstrFile);

		(*filepathBuffer)[strlen(ofn.lpstrFile)] = '\0';

		printf("\nOpening File:  %s\n", ofn.lpstrFile); 
		
		return *filepathBuffer;
		
		//return std::string((char*)(ofn.lpstrFile));
	}

	if( *filepathBuffer )
	{
		free(*filepathBuffer);
		*filepathBuffer = NULL;
	}

	return *filepathBuffer;

}

char * saveFileDialog(HWND hwnd, char ** filepathBuffer, const char * filterExtensions)
{
	OPENFILENAME ofn;       // common dialog box structure
	char szFile[1024];       // buffer for file name
	//HWND hwnd;              // owner window
	HANDLE hf;              // file handle

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
	// use the contents of szFile to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = filterExtensions;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR ;

	// Display the Open dialog box. 

	if (GetSaveFileName(&ofn)==TRUE) 
	{
		if( *filepathBuffer )
			*filepathBuffer = (char*)realloc(*filepathBuffer, strlen(ofn.lpstrFile) * sizeof(char));
		else
			*filepathBuffer = (char*)malloc(strlen(ofn.lpstrFile) * sizeof(char));
		
		strcpy(*filepathBuffer, ofn.lpstrFile);

		printf("\nSave File Path:  %s\n", ofn.lpstrFile); 
		
		return *filepathBuffer;
		
		//return std::string((char*)(ofn.lpstrFile));
	}

	if( *filepathBuffer )
		free(*filepathBuffer);
	*filepathBuffer = NULL;

	return *filepathBuffer;

}


//pass an empty WNDCLASSEX, and an existing HINSTANCE to initialize WNDCLASSEX
//needed for WINDOWS API Window Creation
/*
bool CreateWndClass (WNDCLASSEX &ex, HINSTANCE hinstance )
{
	printf("\nCreate Window Class\n");
	ex.cbSize = sizeof(WNDCLASSEX);
	ex.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
	ex.lpfnWndProc = WinProc;
	ex.cbClsExtra = 0;
	ex.cbWndExtra = 0;
	ex.hInstance = hinstance;
	ex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	ex.hCursor = LoadCursor(NULL, IDC_ARROW);
	ex.hbrBackground = NULL;//(HBRUSH)GetStockObject(BLACK_BRUSH); //FYI, if there is a brush, the system will clear the window right after each redraw step, then send the WM_PAINT, which is bad for resizing opengl window
	ex.lpszMenuName = NULL;
	ex.lpszClassName = WNDCLASSNAME;
	ex.hIconSm = NULL;

		printf("\nCreate Window Class 2\n");

	if (!RegisterClassEx(&ex))
	{
		MessageBox(NULL, "Failed to register the window class", "Window Reg Error", MB_OK);
		return false;
	}

	return true;
}
*/

//pass an empty WNDCLASSEX, and an existing HINSTANCE to initialize WNDCLASSEX
//needed for WINDOWS API Window Creation
bool RegisterWndClass (char * className, WNDCLASSEX &ex, HINSTANCE &hinstance, WNDPROC eventQueue)
{
	DWORD dwErr;
	//if( eventQueue == NULL )
	//	eventQueue = WinProc;//__crgc_view_platform_window_event_queue;


	//WNDCLASSEX classEx;
	//WNDCLASSEX wc;
	//ZeroMemory( &classEx, sizeof(WNDCLASSEX) );

	ex.cbSize = sizeof(WNDCLASSEX);
	//CS_HREDRAW specifies that a horizontal size change causes redraw of entire window. 
	//otherwise horizontal size change causes redraw only of newly exposed area. ditto for CS_VREDRAW
	ex.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
	ex.lpfnWndProc = eventQueue;
	ex.cbClsExtra = 0;
	ex.cbWndExtra = 0;
	ex.hInstance = hinstance;
	ex.hIcon = NULL;// LoadIcon(NULL, IDI_APPLICATION);
	ex.hCursor = LoadCursor(NULL, IDC_ARROW);
	ex.hbrBackground = NULL;//(HBRUSH)GetStockObject(HOLLOW_BRUSH); //FYI, if there is a brush, the system will clear the window right after each redraw step, then send the WM_PAINT, which is bad for resizing opengl window
	ex.lpszMenuName = NULL;
	
	if( className )
		ex.lpszClassName = (const char*)className;
	else
		ex.lpszClassName = WNDCLASSNAME;//TEXT(WNDCLASSNAME);
	
	printf("Attempting to register window with name %s\n", ex.lpszClassName);
	ex.hIconSm = NULL;

	if (!RegisterClassEx(&ex) /*&& (dwErr = GetLastError() != ERROR_CLASS_ALREADY_EXISTS)*/)
	{
		char buf[256];
		dwErr = GetLastError();
		sprintf(buf, "Failed to register the window class with error: %lu", dwErr);
		MessageBox(NULL, buf, "Window Reg Error", MB_OK);
		return false;
	}

	return true;
}




//Create a Microsoft Windows API Window
//pass an empty HWND, and an existing HINSTANCE to initialize HWND (WINDOWS API Window Handle)
void CreateWnd (char * title, char * className, HWND &hwnd, HINSTANCE &hinst, HDC &hdc, int &x, int &y, int &width, int &height, int depth, int desiredScreenMode, bool hasTitleBar, bool hasMenu)
{	
	char * wndClassName;

	//tell windows to use "retina scaling" for graphics output
	::SetProcessDPIAware();

	// center position of the window
	//int posx = x;//(GetSystemMetrics(SM_CXSCREEN) / 2) - (width / 2);
	//int posy = y;//(GetSystemMetrics(SM_CYSCREEN) / 2) - (height / 2);
 
	if (!className)
		wndClassName = WNDCLASSNAME;
	else
		wndClassName = className;


	//always set up the window for a windowed application by default
	DWORD wndStyle = WS_OVERLAPPEDWINDOW;
	//screenmode = WINDOWED;
 
	if( desiredScreenMode == BORDERLESS )
	{
		wndStyle = WS_POPUP;
	}


	RECT adjustedWindowRect={x, y, glWindow.width, glWindow.height};

	//populate the window style
	wndStyle = (wndStyle|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_CAPTION|WS_SYSMENU) /*& ~WS_BORDER*/;

	//if we desired to have a title bar or menu
	//calculate the adjusted window size with AdjustWindowRectEx

	//must set menu bool to true to account for that height
	if( hasTitleBar || hasMenu )
		AdjustWindowRectEx(&adjustedWindowRect, wndStyle, hasMenu, NULL);
	
	printf("\nBefore CreateWndEX\n");
	// create the window
	hwnd = CreateWindowEx(NULL,
				  wndClassName,
			      title,
			      wndStyle,
			      adjustedWindowRect.left, y,
			      adjustedWindowRect.right - adjustedWindowRect.left, adjustedWindowRect.bottom - adjustedWindowRect.top,
			      NULL,
			      NULL,
			      hinst,
			      NULL);
 	printf("\After CreateWndEX\n");

	//when drawing to WINDOWS API window we must explictitly get the DC handle, so we can release it
	//hdc = GetDC(hwnd);
	
	// at this point WM_CREATE message is sent/received
	// the WM_CREATE branch inside WinProc function will execute here
}



#endif