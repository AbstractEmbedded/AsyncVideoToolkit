
///////////////////////////////////////////////////////////////////////////////
//  SYSTEM DEFINITIONS/INCLUDES

#define NTDDI_VERSION NTDDI_WIN7 
#define _WIN32_WINNT _WIN32_WINNT_WIN7 

// standard definitions
#define STRICT                                                  // enable strict type-checking of Windows handles
#define WIN32_LEAN_AND_MEAN                                     // allow the exclusion of uncommon features
#define WINVER                                          _WIN32_WINNT_WIN7  // allow the use of Windows XP specific features
#define _WIN32_WINNT                                    _WIN32_WINNT_WIN7  // allow the use of Windows XP specific features
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES         1       // use the new secure functions in the CRT
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT   1       // use the new secure functions in the CRT


//windows header (for MS WINDOWS API Window Creation)
#include <Windows.h>
#include <windowsx.h>           // useful Windows programming extensions
#include <ShellAPI.h>

#include <Commdlg.h>			//Open File Dialog


//DEBUGGING INCLUDES
//Visual Leak Detector
#ifdef _DEBUG
//#include <vld.h> 
//#include <crtdbg.h> 
#endif

//C Library Includes
//#include "ntdef.h" 
//#include "ntstatus.h"

#include <fcntl.h>  
#include <sys/types.h>  
#include <sys/stat.h>  
#include <io.h>  
#include <stdio.h>  
  

#include <stdlib.h>  
#include <tchar.h>              // generic text character mapping
#include <string.h>             // includes string manipulation routines
//#include <stdlib.h>            // includes standard libraries
#include <stdio.h>              // includes standard input/output routines
#include <process.h>            // threading routines for the CRT
#include <tlhelp32.h>
#include <malloc.h>




//CUSTOM INCLUDES
//Win32 Window Creation and OpenGL Context Creation Utility API
#include "wglUtils.h"

//Core Render Engine Win32 system level display and timing functionality functionality
#include "cr_display.h"
#include "cr_display_sync.h"

//GLSL Shader Attirbute and Uniform Definitions
//and compile shader functionality
#include "AsyncVideoPlayerShader.h"

//image loading routines
#include "sgi_image.h"
#include "avt_image.h"


//miscellanous utilities
#include "stringUtils.h"
#include "cr_time_utils.h"

//a cross-platform platform dependent circular or "ring" buffer implementation
//useful for synchronizing texture handles or the texture data itself between two threads in a consumer/producer fashion
//currently supports Win32 and Darwin architectures (and any unix flavor that provides mmap + virtual memory functionality
#include "avt_circular_buffer.h" 

//User Interface Layer Definitions
#include "AsyncVideoPlayerGUI.h"


//CUSTOM DEFINITIONS

//define application, process and render states

//The OpenGL render thread backed by _glWindow Win32 OpenGL Window
//will define itself through enumerated render states
typedef enum AVPVideoRenderState
{
	VIDEO_RENDER_STATE_IDLE = 0,		//when the application is not playing back video the render thread should idle and listen for wake
	VIDEO_RENDER_STATE_PLAYING = 1,		//
	VIDEO_RENDER_STATE_PAUSED = 2

}AVP_VIDEO_RENDER_STATE;

//The OpenGL GPGPU processing thread backed by _gpgupuWindow Win32 OpenGL Window
//will define its operation through enumerated event states
typedef enum AVPVideoEventState
{
	VIDEO_EVENT_STATE_IDLE		= 0,			//when the application is not loading or playing back video the processing thread should idle and listen for wake
	VIDEO_EVENT_STATE_BUFFERING = 1,
	VIDEO_EVENT_STATE_PAUSED	= 2,
	VIDEO_EVENT_STATE_PLAYING   = 3,
	VIDEO_EVENT_STATE_SEEKING   = 4
}AVP_VIDEO_EVENT_STATE;

// ...and those enumerated event states will be managed through control events
typedef enum AVPVideoControlEvent
{
	AVP_VIDEO_CONTROL_IDLE	= 0,
	AVP_VIDEO_CONTROL_LOAD	= 1,
	AVP_VIDEO_CONTROL_PAUSE	= 2,
	AVP_VIDEO_CONTROL_PLAY	= 3,
	AVP_VIDEO_CONTROL_SEEK  = 4

}AVPVideoControlEvent;

//The GPGPU processing thread will always asynchronously upload frames to GPU while managing a texture cache and inform the render thread about them
//but caching/streaming can be done in several different ways, though all latency paths are subject to the time it takes to asynchronously load
//startup latency is defined as time to render the first frame of a video
typedef enum AVPVideoCacheScheme
{
	CACHE_FRAMES_TO_GPU,	//load all frames to GPU before playback -- video size limited by available GPU memory, startup latency determined by Disk read speed, framerate chokes when moving non-resident textures into GPU resident memory 
	CACHE_FRAMES_TO_CPU,	//load all frames to CPU before playback -- video size limited by available RAM, startup latency determined by Disk speed/video size, framerate determined by speed of path from RAM -> PCIE -> GPU
	BUFFER_FRAMES_TO_CPU,	//load large chunks of frames from disk while playing back previously loaded chunk(s) -- video size limited by disk size, startup latency determined by Disk speed/chunk size, framerate determined by speed of path from RAM -> PCIE -> GPU
	STREAM_FROM_DISK		//load frames directly from disk to GPU with no intermediate caching or buffering -- video size limited by disk size startup latency determined by time to load single frame from disk, framerate determined by speed of path DISK->RAM->PCIE->GPU
}AVP_VIDEO_CACHE_SCHEME;


typedef enum avp_app_state
{
	INVALID_NUM_ARGS = -2,
	INVALID_ARG = -1,
	AVP_IMPORT_SOURCE_VIDEO = 0
	//TG_EXPORT_TO_TDF = 1,
	//TG_EXPORT_TEMPERATURES_TO_SOURCE = 2,

}AVP_APP_STATE;


// user defined messages for passing between window control and asynchronous threads to communicate
//#define AVP_WINDOW_PAUSE   (WM_APP + 1)
#define AVP_WINDOW_RESIZE  (WM_APP + 1)
#define AVP_WINDOW_SHOW	  (WM_APP + 2)
#define AVP_WINDOW_RECREATE (WM_APP + 3)
#define AVP_WINDOW_CLOSE   (WM_APP + 4)


//enumerate options for a bitfield option mask
typedef enum AVP_WINDOW_RESIZE_OPTION
{
	AVP_WINDOW_RESIZE_OPTION_NONE				= 1 << 0,	// 0000 0001, //Standard resize of window
	AVP_WINDOW_RESIZE_OPTION_BORDERLESS			= 1 << 1,	// 0000 0010
	AVP_WINDOW_RESIZE_OPTION_FULLSCREEN			= 1 << 2		// 0000 0100//Fullscreen option will make the win32 window the size of the screen

}AVP_WINDOW_RESIZE_OPTION;

//WINDOW + OPENGL MESSAGES
#define AVP_WINDOW_GRAPHICS_CONTEXT_INITIALIZED  (WM_APP + 5)
#define AVP_SHARE_GRAPHICS_CONTEXT_INITIALIZED  (WM_APP + 6)
#define AVP_WINDOW_TOGGLE_VSYNC (WM_APP + 7)
//#define AVP_WINDOW_RENDER_INSTRUCTION (WM_APP + 7)
//#define AVP_WINDOW_SET_RENDER_STATE   (WM_APP + 8)

//VIDEO CONTROL EVENT MESSAGES
#define AVP_VIDEO_CONTROL_EVENT (WM_APP + 8 )

//THREAD CONTROL MESSAGES
#define AVP_THREAD_CLOSE (WM_APP + 9)

//window threads will respond to these messages
#define AVP_WINDOW_MESSAGE_FIRST AVP_WINDOW_RESIZE
#define AVP_WINDOW_MESSAGE_LAST	AVP_VIDEO_CONTROL_EVENT

//non window threads will respond to these messages
#define AVP_THREAD_MESSAGE_FIRST AVP_THREAD_CLOSE
#define AVP_THREAD_MESSAGE_LAST AVP_THREAD_CLOSE

volatile AVP_APP_STATE g_appState = AVP_IMPORT_SOURCE_VIDEO;
volatile AVP_VIDEO_EVENT_STATE g_videoEventState = VIDEO_EVENT_STATE_IDLE; //start the app in video idle rendering state

//define some metadata properties for a display
typedef struct avp_display
{
	cr_ulong2 resolution;
	cr_float2 dimensions;
	HANDLE verticalRetraceEvent;// = NULL;				//a native windows event associated with the display vertical retrace that can be waited on for thread synchronization (use this one)
	//static HANDLE verticalRetraceSemaphore;// = NULL;			//a native windows semaphore associated with the display vertical retrace that can be waited on for limiting thread resources
	//static HANDLE idleEvent;//= NULL;							//a native windows event associated with the display vertical retrace that can be waited on for thread synchronization

	bool vsyncServiceRunning;			//status boolean
	//static CRITICAL_SECTION _vilock;			//lock a critical section of code using a Native windows mechanism
	//static HANDLE _hVFront = NULL;			//a native windows event associated with the front buffer that can be waited on for thread synchronization
	//static HANDLE _hVBack = NULL;				//a native windows event associated with the front buffer that can be waited on for thread synchronization
	HANDLE hVThread;//=NULL;				//handle to (native windows) vsync update thread
	unsigned int hVThreadID;//=0;
}AVP_DISPLAY;

//define some METADATA properties for a video file
typedef struct avp_video
{
	size_t width, height, framerate;
	size_t numFrames;
	char * sourcepath;

}AVP_VIDEO;

static avp_display g_display = {0};
static avp_video g_sourceVideo = {0};

//define a string useful for loading files
char * g_videoSourcePath = NULL;

//define variables need for abstract video texture streaming components only
//(i.e. that means no graphics specific implementation variables -- they go in JRMVideoPlayerShader.h
//define number of video frames for test video for convenience
//static const int NUM_VIDEO_FRAMES = 240;

//memory mapped file
int _videoFileDescriptor;
FILE * _videoFilePtr = NULL;
HANDLE _videoFile = 0;
HANDLE _videoFileMap = 0;
//unsigned char * _videoMapView = NULL;
unsigned char * _videoMapPtr = NULL;


//let's define a size for our circular buffer
//make it the size of the data we need for one scanline per pixel
//multiplied by n frames worth of pixels
//while adhering to the OS virutal memory page size
static const unsigned int NUM_CACHE_FRAMES = 1;
static const unsigned int NUM_FRAMES_TO_BUFFER = 1;

//the id of the last texture video texture that the render thread rendered (used for pausing video to keep rendering the last frame)
GLuint _videoFrame; 
//a pre-allocated resident memory texture cache that pbos will pull from   
/*GLuint*/ unsigned int _videoTextureCache[NUM_CACHE_FRAMES]; //= 0; //must initialize to 0

//define the circular buffer that will provide write and read safe access between a producer and consumer thread respectively
//specifically, in the context of this application for video playback, it will be populated by the asynchronous "GPGPU" texture loading thread
//with OpenGL texture handles and consumed by the asynchronous render thread for display
avt_circ_buffer _loadTextureSync;
avt_circ_buffer _renderTextureSync;

//static const int osv_image_width = 3840;
//static const int osv_image_height = 2160;
//static const int num_pixels_per_frame = osv_image_width * osv_image_height;
//static const int scanline_size = 20 * sizeof(unsigned char); //4 bytes of data per pixel needs to be written to the scanner control hardware output
static const unsigned int k_circ_buffer_size = 65536 * sizeof(unsigned int);  //Circular buffer size needs to be a multiple of the virtual page size (4096 bytes?  or 64k?)
						



static avt_float4_matrix updateCamera()
{

	//declarations
	avt_float3 eye_pos;
	avt_float3 center;
	avt_float3 up;

	int width, height;
	int y, one, mask, masked_n, signbit;
	float upDirection;
    GLfloat aspect;
    avt_float4_matrix projectionMat = {0};
    
	//initializations
    //get the view matrix so we can pass the inverse view matrix to the shader
    avt_float4_matrix viewInverseMat = AVT_MATRIX_IDENTITY;   //view inverse matrix is actually the camera world transformation matrix caculated by crLookAt
    avt_float4_matrix viewMat = AVT_MATRIX_IDENTITY;          //the view matrix will be derived by taking the inverse of the camera world matrix
	avt_float4_matrix modelMat = AVT_MATRIX_IDENTITY;
	avt_float4_matrix modelViewMat = AVT_MATRIX_IDENTITY;
	avt_float4_matrix rotation_matrix = AVT_MATRIX_IDENTITY;
    
    
    //1 SET THE VIEWPORT for the OPENGL WINDOW!

	width = glWindow.width;
	height = glWindow.height;

	if( !glWindow.fullscreen )
	{
	    width = glWindow.windowedWidth;
		height = glWindow.windowedHeight;
	}

    glViewport(0, 0, width, height);											// set the windowport
    
    aspect = (GLfloat) width / (GLfloat) height;
    
    //2 CREATE A PERSPECTIVE MATRIX !

    //In order to fil the rectangular screen with square quad geometry using a perspective matrix
    //supply an aspect of 1.0f, an fov of 90.0 and place the depth of the quad from the camera at the near frustum (so it doesn't fail depth tests)
    //cr_float4_matrix projectionMat = {0};
    //crMatrixMath eliminates the need for glu, which doesn't exist for iOS (DARWIN)
    //gluPerspective(90, aspect, 1.0f, 1000.0f);
    avt_float4_matrix_perspective( &projectionMat, 90.f, 1.0f, 1.0f, 100.0f);                // set up a perspective projection matrix
    glUniformMatrix4fv( _projectionUniform, 1, 0, projectionMat.vector );                // pass it to the vertex shader as a uniform
	
	//avt_float4_matrix_print(&projectionMat, "Projection Matrix");

    //3 CREATE A VIEW MATRIX !
    
    //1 set the world position of the camera
    //eye_pos = {0.0f, 2.0f, 10.0f};      //push the camera back by 130 down by 40 units, crLookatAt will actually create a matrix that will translate the world and leave camera at origin
    eye_pos.x = 0.0f;
	eye_pos.y = 0.0f;
	eye_pos.z = 1.0f;
	//viewInverseMat.vector[12] = -1*eye_pos.x;
    //viewInverseMat.vector[13] = -1*eye_pos.y;
    //viewInverseMat.vector[14] = -1*eye_pos.z;
 
    //avt_float3 eye_view = {0.0f, 1.0f, -1.0f};
    //2 set the target position to look at
    //avt_float3 center = {0.0, 0.0, 0.0};
    center.x = 0.0f;
	center.y = 0.0f;
	center.z = 0.0f;
	//3 set the camera vertical axis vector relative to world coordinate system (i.e ENU or RFD?)
    
	y = eye_pos.y;
	one = 1;
	
	mask =  1 << 31;
    masked_n = y & mask;
    signbit = masked_n >> 31;
	
	upDirection = (float)(one |= (!signbit) << 31);
	//avt_float3 up = {0.0f, 1.0f, 0.0f};
    up.x = 0.0f; up.y = 1.0f; up.z = 0.0f;

    //4 calculate the view matrix, crPrimitives provides two options:
    //  1) avt_float4_matrix_look is equivalent to gluLookAt
    //  2) crViewMatrixf is an optimized form that takes a transform matrix populated with the negative eye world position instead of passing as a parameter, which avoids the need for several matrix multiplications
    avt_float4_matrix_look(&viewMat, &eye_pos, &center, &up);
    //crViewMatrixf(&viewInverseMat, &center, &up);
    //avt_float4_matrix_print(&viewInverseMat, "Camera Matrix");
    
    //5 calculate the inverse view matrix, not that we'll need it
    avt_float4_matrix_inverse(viewMat.vector, viewInverseMat.vector);
    //avt_float4_matrix_print(&viewMat, "View Matrix");
    
    //6 pass the view inverse matrix to the vertex shader
    //_viewInverseUniform = glGetUniformLocation( g_ShaderCallback->getShaderProgram(), "viewMatrixInverse");
    glUniformMatrix4fv(_viewInverseUniform,1, GL_FALSE,viewInverseMat.vector);
    //7 pass the view matrix to the vertex shader as a uniform
    glUniformMatrix4fv( _viewUniform, 1, GL_FALSE, viewMat.vector );                // pass it to the vertex shader as a uniform


	return viewMat;

}


static void setupQuadVBOs()
{
	glGenBuffers(1, &_quadVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, _quadVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(AVT_QUAD_VBO), AVT_QUAD_VBO, GL_STATIC_DRAW);

	glGenBuffers(1, &_quadIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _quadIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof( AVT_QUAD_VBO_INDICES ), AVT_QUAD_VBO_INDICES, GL_STATIC_DRAW);

}


//update a mesh translation, rotation and scale to create a world space model matrix
//then calculate the modelView matrix by using the camera's viewInverseMatrix and upload the modelView matrix the GPU
static avt_float4_matrix updateQuad(avt_float4_matrix * viewMat)
{
    //cr_float3 rot_axis;
    //float angle;
    //cr_float_quat quatCorrection;
    //cr_float_quat rot_quat;
    
    //int width, height;
    //int y, one, mask, masked_n, signbit;
    //float upDirection;
    //GLfloat aspect;
    //cr_float4_matrix projectionMat = {0};
    
    //initializations
    //cr_float4_matrix rotation_matrix;// = CR_MATRIX_IDENTITY;
    
    //get the view matrix so we can pass the inverse view matrix to the shader
    //cr_float4_matrix viewInverseMat = CR_MATRIX_IDENTITY;   //view inverse matrix is actually the camera world transformation matrix caculated by crLookAt
    //cr_float4_matrix viewMat = CR_MATRIX_IDENTITY;          //the view matrix will be derived by taking the inverse of the camera world matrix
    avt_float4_matrix modelMat = AVT_MATRIX_IDENTITY;
    avt_float4_matrix modelViewMat = AVT_MATRIX_IDENTITY;
    avt_float4_matrix scale_matrix = AVT_MATRIX_IDENTITY;
    avt_float4_matrix rotation_matrix;// = CR_MATRIX_IDENTITY;
    
    //4 CREATE A MODELVIEW MATRIX FOR OUR MESH(ES) !
    
    //1 create a model matrix to specify position, rotation and scale of model in world coordinate space
    //cr_float4_matrix modelMat = CR_MATRIX_IDENTITY;
    modelMat.vector[12] = 0.0;
    modelMat.vector[13] = 0.0;
    modelMat.vector[14] = 0.0;//-1000.0;    //translate mesh into screen
    
    //however, we can leave the model at (0,0,0) and then translate our camera backward (and up) by creating a view matrix instead
    //then calculate the modelView Matrix later after models have upated their positions based on that view matrix
    
    //rot_axis = CR_X_AXIS;
    //angle = -M_PI_2;
    
    //quatCorrection = cr_float_quat_from_axis_angle(&rot_axis, angle);
    //rot_quat = cr_float_quat_multiply(&_arcballRotationQuat, &quatCorrection);
    
    //scale the quad because it is a unit quad that ranges from -0.5 to 0.5 in X and Y components
	//but screen position components after ortho projection range from -1.0 to 1.0
	//and we want to fill teh screen with the texture that maps to the quad
    
    //cr_float4_matrix scale_matrix = CR_MATRIX_IDENTITY;
    scale_matrix.vector[0] = 2.0;
    scale_matrix.vector[5] = 2.0;
    scale_matrix.vector[10] = 2.0;
    avt_float4_matrix_multiply(&modelMat, &scale_matrix, &modelMat);
    
    
    //2 apply tranlsations, rotations, and scale
    //rotation_matrix = cr_float4_matrix_from_quaternion(&rot_quat);
    //cr_float4_matrix_multiply(&modelMat, &rotation_matrix, &modelMat);
    
    
    //3 multiply the model matrix by the view matrix on the CPU so this doesn't have to be done per vertex shader on the GPU
    //cr_float4_matrix modelViewMat = CR_MATRIX_IDENTITY;
    avt_float4_matrix_multiply(viewMat, &modelMat, &modelViewMat);      //multiply viewMatrix * modelViewMatrix;
    
    //4 pass the modelview matrix to the vertex shader as uniform
    glUniformMatrix4fv(_modelViewUniform, 1, 0, modelViewMat.vector );
    
    
    return modelViewMat;
    
    //NOW WE ARE READY TO PASS THE GEOMETRY TO BE RENDERED ACCORDING TO OUR MATRICES
    

    
    
}




static void renderTexturedQuad(GLuint * textureHandle)
{
    //bind the quad buffer for the font glyphs that we will modify per glyph
    //for one draw call per glyph texture
    //glBindBuffer(GL_ARRAY_BUFFER, (atlas->quadVertexBuffer));
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (atlas->quadIndexBuffer));
    glBindBuffer(GL_ARRAY_BUFFER, _quadVertexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _quadIndexBuffer);
    
    //glVertexAttribPointer(_texturePositionSlot, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    //glVertexAttribPointer(_textureColorSlot, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) (sizeof(float) * 3));
    
	//tell gpu how vbo attribute uniforms are packed
	glVertexAttribPointer( _vertexAttribute, 4, GL_FLOAT, GL_FALSE, sizeof(avt_vertex_mesh_vbo), 0);
	glVertexAttribPointer( _colorAttribute, 4, GL_FLOAT, GL_FALSE, sizeof(avt_vertex_mesh_vbo), (GLvoid*)(sizeof(avt_float4_packed)));
	glVertexAttribPointer( _normalAttribute, 4, GL_FLOAT, GL_FALSE, sizeof(avt_vertex_mesh_vbo), (GLvoid*)(sizeof(avt_float4_packed) * 2));
	glVertexAttribPointer( _texelAttribute, 2, GL_FLOAT, GL_FALSE, sizeof(avt_vertex_mesh_vbo), (GLvoid*)(sizeof(avt_float4_packed) * 3));
    
    //set the active texture in slot 0 to be the texture we use for rendering glyphs
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, *textureHandle);
    //identify the texture unit slot to the shader?
    glUniform1i(_textureUniform, 0);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     
    
    glDrawElements(GL_TRIANGLES, sizeof(AVT_QUAD_VBO_INDICES)/sizeof(AVT_QUAD_VBO_INDICES[0]), GL_UNSIGNED_BYTE, 0);
    
}

static GLuint loadGLTextureFromImage(AVT_IMAGE * img, const char * filepath, GLuint textureFormat, GLuint *textureHandle, GLuint textureLayer, GLuint totalLayers, unsigned int &imgWidth, unsigned int &imgHeight)
{
	int newTexture;
	//GLuint textureHandle;	
	//AVT_IMAGE img = {0};							//create a local image object to load the image to the cpu
	

	const char * imgExtension = findFileExtension(filepath);


	//send the cpu texture memory to the gpu
	newTexture = 0;
	if (*textureHandle == 0)
	{
		glGenTextures(1, textureHandle);
		newTexture = 1;
		//printf("\nafter gen textures\n");
	}
	
	//glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipLevelCount, GL_RGBA8, width, height, 2);

	/*
	if( strcmp( imgExtension, "dds") == 0 )
	{
		img = avt_load_image_compressed(filepath);				//populate the local image object by loading an image file in compressed format from disk

		glBindTexture(textureFormat, *textureHandle);
		if (textureFormat == GL_TEXTURE_2D)
		{
			glCompressedTexImage2D(textureFormat,0,img.pixelInternalFormat,img.width,img.height,0,img.size,img.pixelData);
		}
		else
		{
			if (newTexture)
				glCompressedTexImage3D(textureFormat,0,img.pixelInternalFormat,img.width,img.height,totalLayers,0,img.size*totalLayers,NULL);
			glCompressedTexSubImage3D(textureFormat,0,0,0,textureLayer,img.width,img.height,1,img.pixelInternalFormat,img.size,img.pixelData);
		}
		glTexParameteri( textureFormat, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( textureFormat, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glGenerateMipmap(textureFormat);
		glTexParameteri(textureFormat, GL_GENERATE_MIPMAP, GL_TRUE);
		glBindTexture(textureFormat, 0);
	}
	else
	*/

	//printf("\nReading Texture File: %s\n", filepath);

	
	//create an opengl texture with the image buffer to copy the image to the gpu in the appropriate format the framebuffer and/or shader(s) expect
	//glGenTextures( 1, textureHandle );
	glBindTexture(GL_TEXTURE_2D, *textureHandle);

	//printf("\nafter gen bind \n");

	// Apply texture sampling/filtering preferences
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glGenerateMipmap(GL_TEXTURE_2D);

	//printf("\nafter gen mipmap \n");
	

	if( strcmp(imgExtension, "rgb") == 0 ) // load sgi image
	{
		//printf("\nReading Texture File: %s\n", filepath);
		
		
		//The backing .mcm image file format is SGI Iris (.rgb), let's load it as such
		unsigned int width, height, numChannels = 0;
		float fWidth, fHeight;
		unsigned char * imageBytes = NULL;

		numChannels = 4;
		

		imageBytes = loadSGIImage(filepath, &width, &height, &numChannels);
	
		
		if( imageBytes )
		{
			printf("\nMCM width = %d", width);
			printf("\nMCM height = %d", height);
			printf("\nMCM numChannels: %d\n", numChannels);
	
			imgWidth = width; 
			imgHeight = height;

			fWidth = (float)width;
			fHeight = (float)height;
		}
		
	

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, imageBytes);

		
		printf("\nafter texImage2D\n");

		free(imageBytes);
	}
	else if( strcmp(imgExtension, "rgba") == 0  || strcmp(imgExtension, "bgra") == 0 )
	{
		/*
		img.numChannels = 4;
		//use the pass by reference input to tell us the image dimensions
		//since our custom raw byte format has no header for performance
		img.width = imgWidth; 
		img.height = imgHeight;
		img.depth = 8;

		img.filepath = (char*)malloc( strlen(filepath)+1 );
		strcpy( img.filepath, filepath);
		*/

		//this will allocate data for the pixel buffer and load image data from file into it
		//AVT_IMAGE.pixelData memory must be freed with avt_free_image(img);
		avt_image_buffer_import_bytes( &(img->pixelData), img->width, img->height, img->numChannels, img->depth, filepath);

		//TO DO: if texture size or format changes must recreate the glTexture
		if( newTexture )
		{
			glTexStorage2D( GL_TEXTURE_2D, 1, GL_RGBA8, (GLsizei)img->width, (GLsizei)img->height);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)img->width, (GLsizei)img->height, GL_BGRA, GL_UNSIGNED_BYTE, img->pixelData ); 
			//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)img->width, (GLsizei)img->height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, img->pixelData ); 
			//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)img->width, (GLsizei)img->height, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, img->pixelData);
		}
		else
		{
			//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)img->width, (GLsizei)img->height/4, GL_BGRA, GL_UNSIGNED_BYTE, img->pixelData ); 
			//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, (GLsizei)img->height/4, (GLsizei)img->width, (GLsizei)img->height/4, GL_BGRA, GL_UNSIGNED_BYTE, img->pixelData + (img->width * img->numChannels * (GLsizei)img->height/4) ); 
			//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, (GLsizei)img->height/2, (GLsizei)img->width, (GLsizei)img->height/4, GL_BGRA, GL_UNSIGNED_BYTE, img->pixelData + (img->width * img->numChannels * (GLsizei)img->height/2)  ); 
			//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, (GLsizei)img->height/4*3, (GLsizei)img->width, (GLsizei)img->height/4, GL_BGRA, GL_UNSIGNED_BYTE, img->pixelData + (img->width * img->numChannels * ( (GLsizei)img->height/4 * 3 ) ) ); 
			//glTexStorage2D( GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)img->width, (GLsizei)img->height);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)img->width, (GLsizei)img->height, GL_BGRA, GL_UNSIGNED_BYTE, img->pixelData ); 
		}
	}
	
	avt_print_image_details( img); //print the image details to make sure everything loaded appropriately
    
    //explicitly free image buffer and image object after load, because the image object would be freed by going out of scope but buffer will not
    //avt_free_image( img );
    

	//explicitly free image buffer and image object after load, because the image object would be freed by going out of scope but buffer will not
	//free(imageBytes);

	//return the handle to the texture in gpu memory
	return *textureHandle;
}

static void loadVideoTexture(unsigned int textureIndex, AVT_IMAGE * img, char * videoTexturePath, unsigned int textureWidth, unsigned int textureHeight)
{
	unsigned int pathLength;
	unsigned int textureNameLength;
	unsigned int texturePathLength;
	unsigned int textureExtensionLength;

	const char * filename;
	const char * textureExtension;
	char * texturePath;
	const char * textureName;
	GLuint textureHandle;
	//unsigned int textureIndex;


	//const char * videoTexturePath = "..\\..\\textures\\t72a-mbt_camo.rgb";
	//const char * videoTexturePath = "..\\..\\textures\\Frame_0.bgra";
	//_videoTexture = 0;

	unsigned int width, height;
	//width = height = 0;

	//for raw buffer formats, we must specify the size of the image
	//because there is no header to read that specifies this information
	width = textureWidth;
	height = textureHeight;

	_videoTextureCache[textureIndex] = (unsigned int)loadGLTextureFromImage(img, (const char*) videoTexturePath, GL_TEXTURE_2D, &(_videoTextureCache[textureIndex]), 0, 0, width, height);

}


void createPBOs(unsigned int width, unsigned int height, unsigned int numChannels, unsigned int bitDepth)
{
	 //if(pboSupported)
    //{
		printf("\ncreate PBOs\n");
		unsigned int pboSize = width * height * numChannels;
        // create 2 pixel buffer objects, you need to delete them when program exits.
        // glBufferDataARB with NULL pointer reserves only memory space.
        glGenBuffers(_numPBOs, _pboIds);
        
		for( int pboIndex = 0; pboIndex < _numPBOs; pboIndex++)
		{
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pboIds[pboIndex]);
			//unsigned int flags =   GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
			glBufferStorage(GL_PIXEL_UNPACK_BUFFER, pboSize, 0,   GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
			//glBufferData(GL_PIXEL_UNPACK_BUFFER, pboSize, 0, GL_STREAM_DRAW);
			_pboMaps[pboIndex] = (GLubyte*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, pboSize,  GL_MAP_WRITE_BIT  | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT/*| GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_FLUSH_EXPLICIT_BIT | GL_MAP_INVALIDATE_RANGE_BIT*/ );
		}
        
		//Must unbind or this won't work even with a single PBO!!! Why?!
		//It just gets re-bound below...
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		
		getOpenGLErrors();
}

void readTextureFromPBO( GLuint textureHandle, unsigned int pboIndex )
{
		 // bind the texture and the PBO populated on the previous frame to copy data from pbo to GPU texture
		//Confirmed that this gives zero-copy latency, the real problem is the time it takes to map/unmap the pbo buffer when populating the pbo below
		glBindTexture(GL_TEXTURE_2D, textureHandle);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pboIds[pboIndex]);

		// copy pixels from PBO to texture object	
		// Use offset instead of pointer.
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, g_sourceVideo.width, g_sourceVideo.height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, 0);
		
		//unbind buffer
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		//unbind texture
		glBindTexture(GL_TEXTURE_2D, 0);

}


void openFile(char * filepath)
{
	//fopen fails for large files on x64 platforms :(
	//use _open instead
	//fopen_s(&_videoFilePtr, filepath, "rb");
	
	//int fd;
  //__int64 n;

  /* Open file */
  _videoFileDescriptor = _open( filepath, _O_BINARY );


  /* Close file */
  //_close(fh);

 //return n / sizeof(short);
	
	//assert(_videoFilePtr != NULL);

	//seek to get file size
		//fseek(_videoF	ilePtr, 0, SEEK_END);
		//struct stat finfo;
		//fstat(fileno(_videoFilePtr), &finfo);// (long)ftell(_videoFilePtr);

		  /* Find end of file */
		//unsigned long long fileSize = _lseeki64(fd, 0, SEEK_END);

		//printf("\n  path to file: %s\n", filepath);
		//printf("\n file size in bytes: %lld\n", fileSize);//finfo.st_size);
		//system("pause");
		//seek back to the beginning of the ifle
		//fseek(file, 0L, SEEK_SET);
		//rewind(_videoFilePtr);
}

void closeFile( )
{
	//fclose(_videoFilePtr);
	_close(_videoFileDescriptor);
}

DWORD getpagesize()
{
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	return system_info.dwPageSize;
}

TCHAR szName[] = TEXT("LARGEPAGE");
typedef int(*GETLARGEPAGEMINIMUM)(void);

void DisplayError(TCHAR* pszAPI, DWORD dwError)
{
	LPVOID lpvMessageBuffer;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dwError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpvMessageBuffer, 0, NULL);

	//... now display this string
	_tprintf(TEXT("ERROR: API        = %s\n"), pszAPI);
	_tprintf(TEXT("       error code = %d\n"), dwError);
	_tprintf(TEXT("       message    = %s\n"), lpvMessageBuffer);

	// Free the buffer allocated by the system
	LocalFree(lpvMessageBuffer);

	ExitProcess(GetLastError());
}

void Privilege(TCHAR* pszPrivilege, BOOL bEnable)
{
	HANDLE           hToken;
	TOKEN_PRIVILEGES tp;
	BOOL             status;
	DWORD            error;

	// open process token
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		DisplayError(TEXT("OpenProcessToken"), GetLastError());

	// get the luid
	if (!LookupPrivilegeValue(NULL, pszPrivilege, &tp.Privileges[0].Luid))
		DisplayError(TEXT("LookupPrivilegeValue"), GetLastError());

	tp.PrivilegeCount = 1;

	// enable or disable privilege
	if (bEnable)
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	else
		tp.Privileges[0].Attributes = 0;

	// enable or disable privilege
	status = AdjustTokenPrivileges(hToken, FALSE, &tp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

	// It is possible for AdjustTokenPrivileges to return TRUE and still not succeed.
	// So always check for the last error value.
	error = GetLastError();
	if (!status || (error != ERROR_SUCCESS))
		DisplayError(TEXT("AdjustTokenPrivileges"), GetLastError());

	// close the handle
	if (!CloseHandle(hToken))
		DisplayError(TEXT("CloseHandle"), GetLastError());
}

HANDLE createMappedFile(char * filepath, size_t textureSize)
{

	//HANDLE hMapFile;
	LPCTSTR pBuf;
	//Needed for large pages
	//GETLARGEPAGEMINIMUM pGetLargePageMinimum;
	//HINSTANCE  hDll;
	//HANDLE hMapFile;
	//LPCTSTR pBuf;
	//DWORD largePageMinimum;


	_videoFile = INVALID_HANDLE_VALUE;

	//_videoFile = CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);
	_videoFile = CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);

	if(_videoFile == INVALID_HANDLE_VALUE) { printf("couldn't open %s\n", filepath); return 0; };

	//DWORD fileSize  = GetFileSize(_videoFile, 0);
	
	//filesize needs to be 64 bit integer type and a multiple of the page size 
	//TO DO: get the filesize and calculate the number of frames
	
	LARGE_INTEGER size;
	GetFileSizeEx(_videoFile, &size);
	size_t mappedFileSize = (size_t) size.QuadPart;

	g_sourceVideo.numFrames = mappedFileSize / textureSize;

	// call succeeds only on Windows Server 2003 SP1 or later
	//hDll = LoadLibrary(TEXT("kernel32.dll"));
	//if (hDll == NULL)
	//	DisplayError(TEXT("LoadLibrary"), GetLastError());

	//retrieve GetLargePageMinimum from Kernel32 dll
	//pGetLargePageMinimum = (GETLARGEPAGEMINIMUM)GetProcAddress(hDll,"GetLargePageMinimum");
	//if (pGetLargePageMinimum == NULL)
	//	DisplayError(TEXT("GetProcAddress"), GetLastError());

	//largePageMinimum = (*pGetLargePageMinimum)();
	//FreeLibrary(hDll);

	//_tprintf(TEXT("Large Page Size: %u\n"), largePageMinimum);

	//must lock memory privelege before mapping to use SEC_LARGE_PAGES
	//Privilege(TEXT("SeLockMemoryPrivilege"), TRUE);


	//fileSize += (size_t)largePageMinimum - ((size_t)fileSize%(size_t)largePageMinimum);
	printf("\n fileSize = %llu", mappedFileSize);
	printf("\n numFrames = %llu", g_sourceVideo.numFrames);

	//_videoFileMap = CreateFileMapping(_videoFile, 0, PAGE_READONLY, 0, 0, 0);
	//if(_videoFileMap == 0) { printf("couldn't map %s\n", filepath); return 0; }

	_videoFileMap = CreateFileMapping(
		//INVALID_HANDLE_VALUE,    // use paging file, Creating Named Shared Memory
		_videoFile,
		NULL,                    // default security
		PAGE_READONLY,		     // read/write access
		0,                       // max. object size
		0,						 // buffer size, passing zero for these two params will allocate to size of file
		"Global\MappedFile");                 // name of mapping object

	//Privilege(TEXT("SeLockMemoryPrivilege"), FALSE);

	if (_videoFileMap == NULL || _videoFileMap == INVALID_HANDLE_VALUE)
	{
		printf("Could not create file mapping object (%d).\n",
			GetLastError());
		return NULL;
	}

	_videoMapPtr = (unsigned char*)MapViewOfFile(_videoFileMap,   // handle to map object
		FILE_MAP_READ, // read/write permission
		0,
		0,
		mappedFileSize);


	
	/*
	if(_videoMapPtr)
	{
		printf("prefetching %s... ", filepath);

		// need volatile or need to use result - compiler will otherwise optimize out whole loop
		volatile unsigned long long touch = 0;

		DWORD page_size = getpagesize();
		for(unsigned long long i = 0; i < fileSize; i += page_size)
			touch += _videoMapPtr[i];
	}
	else
		printf("couldn't create view of %s\n", filepath);
	*/

	//UnmapViewOfFile(data);
	//CloseHandle(mapping);
	
	//it is safe to close the file handle after mapping has occurred
	CloseHandle(_videoFile);

	return _videoFileMap;

}



void touchMappedData()
{

	if (_videoMapPtr)
	{
		//printf("prefetching %s... ", filepath);

		DWORD page_size = getpagesize();
		size_t fileSize = (size_t)(g_sourceVideo.width) * (size_t)(g_sourceVideo.height) * 4 * (size_t)(g_sourceVideo.numFrames);

		// need volatile or need to use result - compiler will otherwise optimize out whole loop
		volatile unsigned long long touch = 0;

		for (unsigned long long i = 0; i < fileSize; i += page_size)
			touch += _videoMapPtr[i];
	}
	else
		printf("couldn't create view of file\n");


	// kill this thread and its resources (CRT allocates them)
	//_endthreadex(0);
	return;
}



/*
unsigned char * mapViewOfFile(unsigned int pboIndex, void* copyToPtr)
{

	//long BUFFSIZE = 1024;//FRAME_SIZE * 240;
	//unsigned int frameSize = gpgpuWindow.width * gpgpuWindow.height * 4;
	HANDLE hMapFile;      // handle for the file's memory-mapped region
  HANDLE hFile;         // the file handle
  BOOL bFlag;           // a result holder
  DWORD dBytesWritten;  // number of bytes written
  DWORD dwFileSize;     // temporary storage for file sizes
  DWORD dwFileMapSize;  // size of the file mapping
  DWORD dwMapViewSize;  // the size of the view
  DWORD dwFileMapStart; // where to start the file map view
  DWORD dwSysGran;      // system allocation granularity
  SYSTEM_INFO SysInfo;  // system information; used to get granularity
  LPVOID lpMapAddress;  // pointer to the base address of the
                        // memory-mapped region
  char * pData;         // pointer to the data
  int i;                // loop counter
  int iData;            // on success contains the first int of data
  int iViewDelta;       // the offset into the view where the data
                        //shows up

	long int FRAME_SIZE =   gpgpuWindow.width * gpgpuWindow.height * 4;
	long FILE_MAP_START = pboIndex * FRAME_SIZE;

	// Get the system allocation granularity.
  GetSystemInfo(&SysInfo);
  dwSysGran = SysInfo.dwAllocationGranularity;

  // Now calculate a few variables. Calculate the file offsets as
  // 64-bit values, and then get the low-order 32 bits for the
  // function calls.

  // To calculate where to start the file mapping, round down the
  // offset of the data into the file to the nearest multiple of the
  // system allocation granularity.
  dwFileMapStart = (FILE_MAP_START / dwSysGran) * dwSysGran;
 // _tprintf (TEXT("The file map view starts at %ld bytes into the file.\n"),
    //      dwFileMapStart);

  // Calculate the size of the file mapping view.
  dwMapViewSize = (FILE_MAP_START % dwSysGran) + FRAME_SIZE;
 // _tprintf (TEXT("The file map view is %ld bytes large.\n"),
    //        dwMapViewSize);

  // How large will the file mapping object be?
  dwFileMapSize = FILE_MAP_START + FRAME_SIZE;
  //_tprintf (TEXT("The file mapping object is %ld bytes large.\n"),
     //     dwFileMapSize);

  // The data of interest isn't at the beginning of the
  // view, so determine how far into the view to set the pointer.
  iViewDelta = FILE_MAP_START - dwFileMapStart;
 // _tprintf (TEXT("The data is %d bytes into the view.\n"),
        //   iViewDelta);

  	UnmapViewOfFile(_videoMapView);
  	_videoMapView = (unsigned char*) MapViewOfFileEx(_videoFileMap, FILE_MAP_READ, 0, dwFileMapStart, dwMapViewSize, copyToPtr);
	_videoMapPtr = _videoMapView + iViewDelta; 

	return _videoMapPtr;
}
*/


void ReportResult(DWORD dwBytesTransferred, BYTE* bBuffer, LPSTR pszType)
{
	printf("%d bytes were %s: %s\n", dwBytesTransferred, pszType, bBuffer);
}

void ReportError(DWORD dwError)
{
	// Note: if dwError is 0xc0000011, it means STATUS_END_OF_FILE
	_tprintf(_T("AsyncIOWaitForEventObject failed w/err 0x%08lx\n"), dwError);
}


BOOL RemoveFromEventArray(HANDLE hEvents[], int nSize, int nIndex) 
{
	if (nIndex < 0 || nSize <= nIndex)
		return FALSE;

	for (int i = nIndex + 1; i < nSize; i++)
	{
		hEvents[i - 1] = hEvents[i];
	}
	return TRUE;
}


void asyncReadToBuffer(void * readBuffer, unsigned long bytesToRead)
{
		/////////////////////////////////////////////////////////////////////////
	// Perform multiple asynchronous device I/O requests simultaneously.
	// 

	// A queue of handles for events that are attached to the I/O requests
	HANDLE hReadWriteEvents[10] = { 0 };
	
	int nEvents = 0;
	//HANDLE hFile;
	//TCHAR szFileName[] = _T("MSDN.tmp");
	DWORD dwError;

		// 
	// Issue an asynchronous Read command
	// 

	//create a null overlapped object for async io with ReadFile
	OVERLAPPED oRead = { 0 };
	oRead.OffsetHigh = 0;
	oRead.Offset = 0;
	oRead.hEvent = CreateEvent(NULL, TRUE, FALSE, _T("ReadEvent"));

	//BYTE bReadBuffer[100];
	DWORD dwBytesRead;

	// Issue the Read command
	BOOL bReadDone = ReadFile(_videoFile, readBuffer, bytesToRead, &dwBytesRead, &oRead);
	dwError = GetLastError();

	if (bReadDone)
	{
		// If ReadFile returns TRUE, it indicates that the I/O request was 
		// performed synchronously. At this moment, dwBytesWritten is 
		// meaningful. See http://support.microsoft.com/kb/156932

		//ReportResult(dwBytesRead, (BYTE*)readBuffer, "read");
	}
	else if (dwError != ERROR_IO_PENDING)
	{
		// Error occurred when issuing the asynchronous IO request.
		ReportError(dwError);
		assert(1 == 0);
	}
	else
	{
		// If the I/O request was NOT performed synchronously (ReadFile  
		// returns FALSE), and GetLastError() == ERROR_IO_PENDING, the I/O is 
		// being performed asynchronously. At this moment, dwBytesWritten is 
		// meaningless.

		hReadWriteEvents[nEvents] = oRead.hEvent;
		nEvents++;
		//assert(1 == 0);
	}

	//assert(1==0);

	// 
	// Wait for and handle the completion of any asynchronous read/write 
	// commands in a loop. Typically, a real-life application has a loop that 
	// waits for I/O requests to complete. As each request completes, the 
	// thread performs the desired task, queues another asynchronous I/O 
	// request, and loops back around, waiting for more I/O requests to 
	// complete.
	// 

	while (nEvents > 0)
	{
		DWORD dwReturn = WaitForMultipleObjects(nEvents, hReadWriteEvents, 
			FALSE, INFINITE);
		int nEventId = dwReturn - WAIT_OBJECT_0;

		// Handle the result for the signalled asynchronous IO
		/*
		if (hReadWriteEvents[nEventId] == oWrite.hEvent)
		{
			// The asynchronous write operation is done

			dwError = oWrite.Internal;
			dwBytesWritten = oWrite.InternalHigh;

			if (SUCCEEDED(dwError))
			{
				bWriteDone = TRUE;
				ReportResult(dwBytesWritten, bWriteBuffer, "written");
			}
			else
			{
				SetLastError(dwError);
				ReportError(dwError);
			}
		}
		else */if (hReadWriteEvents[nEventId] == oRead.hEvent)
		{
			// The asynchronous read operation is done

			dwError = oRead.Internal;
			dwBytesRead = oRead.InternalHigh;

			if (SUCCEEDED(dwError))
			{
				bReadDone = TRUE;
				//ReportResult(dwBytesRead, (BYTE*)readBuffer, "read");
			}
			else
			{
				SetLastError(dwError);
				ReportError(dwError);
				assert(1);
			}
		}

		// Remove the signaled event object
		if (RemoveFromEventArray(hReadWriteEvents, nEvents, nEventId))
			nEvents--;
	}
}

//SIMD datatypes are processor/platform specific includes
#ifndef __APPLE__
#if defined(_WIN32) || defined(LINUX)
#define CR_MATH_USE_SIMD
#include <intrin.h>		//__m256 types
#include <mmintrin.h>
#include <xmmintrin.h>  // SSE
#include <pmmintrin.h>  // SSE2
#include <emmintrin.h>  // SSE3
//#include <tmmintrin.h>	// SSSE3
#endif
#endif

/*
static __inline void X_aligned_memcpy_sse2(void* dest, const void* src, const unsigned long size)
{

  __asm
  {
    mov esi, src;    //src pointer
    mov edi, dest;   //dest pointer

    mov ebx, size;   //ebx is our counter 
    shr ebx, 7;      //divide by 128 (8 * 128bit registers)


    loop_copy:
      prefetchnta 128[ESI]; //SSE2 prefetch
      prefetchnta 160[ESI];
      prefetchnta 192[ESI];
      prefetchnta 224[ESI];

      movdqa xmm0, 0[ESI]; //move data from src to registers
      movdqa xmm1, 16[ESI];
      movdqa xmm2, 32[ESI];
      movdqa xmm3, 48[ESI];
      movdqa xmm4, 64[ESI];
      movdqa xmm5, 80[ESI];
      movdqa xmm6, 96[ESI];
      movdqa xmm7, 112[ESI];

      movntdq 0[EDI], xmm0; //move data from registers to dest
      movntdq 16[EDI], xmm1;
      movntdq 32[EDI], xmm2;
      movntdq 48[EDI], xmm3;
      movntdq 64[EDI], xmm4;
      movntdq 80[EDI], xmm5;
      movntdq 96[EDI], xmm6;
      movntdq 112[EDI], xmm7;

      add esi, 128;
      add edi, 128;
      dec ebx;

      jnz loop_copy; //loop please
    loop_copy_end:
  }
}
*/

#include "apex_memmove.h"

//extern "C" void memcpy_asm(unsigned char* dest, unsigned char *src, unsigned long size); // written in assembly!

void loadTextureToPBO(unsigned int pboIndex, unsigned long frameIndex, size_t textureSize)
{
	//decode to pbo from mapped file memory...
		
	// bind PBO to update pixel values -- we don't need to bind for a persistent mapped PBO though :)
	//glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pboIds[pboIndex]);

	// map the buffer object into client's memory
	// Note that glMapBufferARB() causes sync issue.
	// If GPU is working with this buffer, glMapBufferARB() will wait(stall)
	// for GPU to finish its job. To avoid waiting (stall), you can call
	// first glBufferDataARB() with NULL pointer before glMapBufferARB().
	// If you do that, the previous data in PBO will be discarded and
	// glMapBufferARB() returns a new allocated pointer immediately
	// even if GPU is still working with the previous data.

	// if we weren't using peristent mapping we would used the commented out code to:
	// 1) invalidate the buffer (glBufferData)
	// 2) map the pbo into memory (glMapBuffer/glMapBufferRange)
	// 3) and finally use that mapped memory to put data in the pbo (memcpy)

	//invalidate the buffer to tell OpenGL we don't care about the contents of the buffer it provides
	//and we as the client promise to fill the whole buffer
	//glBufferData(GL_PIXEL_UNPACK_BUFFER, gpgpuWindow.width * gpgpuWindow.height * 4, 0, GL_STREAM_DRAW);
	
	//use this to remap the entire buffer
	//GLubyte* ptr = (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY_ARB);
	//use this to remap only part of the buffer
	//(GLubyte*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, glWindow.width * glWindow.height * 4, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_COHERENT_BIT );
	
	//However, since we are using peristent mapped buffer storage (OpenGL 4.4) behind the PBOs
	//we can read directly into the buffer that we've already mapped previously on the asyncGPGPUCallback thread
	GLubyte * ptr = _pboMaps[pboIndex]; 
	if(ptr) //this check is not strictly necessary, we should know that pbo buffer pointers are valid at this point
	{
		//this is the line that copies from the memory mapped file buffer into the pbo 		
		memcpy(ptr, _videoMapPtr + (size_t)frameIndex * textureSize, textureSize);	

		//this is an optimized copy function useful for VS2010 before the compiler updates produced better optimization
		//apex_memcpy(ptr, buffer, textureSize);

		//for debugging you can clear the pbo texture data to a uniform grayscale value 
		//memset(ptr, 255, textureSize);

		//this is extremely legacy code that demonstrates opening a file and reading a frame's worth of data
		//directly into the pbo
		//avt_image_buffer_import_bytes( &ptr, gpgpuWindow.width, gpgpuWindow.height, 4, 8, (char*)videoTexturePath);
	
		//unmap the buffer if not using peristen mapped storage
		//glFlushMappedBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, glWindow.width * glWindow.height * 4);
		//glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release pointer to mapping buffer

		//Note:  If you're still curious what happens with this data at this point,
		//the answer is that the asyncLoadTextureCallback thread puts the corresponding pboIndex in a circular buffer.
		//the async then reads these pboIndices from the thread safe fifo circular buffer queue and uploads them
		//to resident texture memory as glTextures before passing them along to the aysncRenderCallback OpenGL Render thread for rendering
	}

	// unbind PBO -- we don't need to unbind for a persistent mapped PBO though :)
	//glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

}

