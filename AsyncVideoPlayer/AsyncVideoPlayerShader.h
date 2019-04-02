#ifndef ASYNC_VIDEO_PLAYER_SHADER_H
#define ASYNC_VIDEO_PLAYER_SHADER_H

//this shouldn't really be included in a header
//that is meant solely for shader defines, but since the vbo
//is defined here and depends on the data types defined in avt_matrix_math.h
//we define it here for now
#include "avt_quad_vbo.h" //a subset of avt_vertex_mesh that rips out vbo and quad definitions that also contains avt_matrix_math include (sneaky :/)
#include <assert.h>

//Shader attribute and uniform definitions for passing from CPU to vertex/fragment shader pipeline

//define some static strings for passing avt_vertex_mesh vbo attributes
static const char * AVT_VERTEX_POSITION_SHADER_ATTRIBUTE = "avtVertexPosition";
static const char * AVT_VERTEX_COLOR_SHADER_ATTRIBUTE = "avtVertexColor";
static const char * AVT_VERTEX_NORMAL_SHADER_ATTRIBUTE = "avtVertexNormal";
static const char * AVT_VERTEX_TEXTURE_UV_SHADER_ATTRIBUTE = "avtVertexTextureUV";

//establish uniforms to pass matrices and textures to shaders
static const char * AVT_PROJECTION_MATRIX_UNIFORM = "avtProjectionMatrix";
static const char * AVT_VIEW_MATRIX_UNIFORM = "avtViewMatrix";
static const char * AVT_INVERSE_VIEW_MATRIX_UNIFORM = "avtInverseViewMatrix";
static const char * AVT_MODEL_VIEW_MATRIX_UNIFORM = "avtModelViewMatrix";
static const char * AVT_VERTEX_TEXTURE_UNIFORM = "avtVertexTexture";
static const char * AVT_PRIMITIVE_OFFSET_UNIFORM = "avtPrimitiveOffset";

GLsync _renderSync;
//handles to global matrix uniforms to be passed to vertex shader
GLuint _projectionUniform;
GLuint _viewUniform;
GLuint _viewInverseUniform;
GLuint _modelViewUniform;

//handles to vertex buffer object attributes for passing to shader
GLuint _vertexAttribute;
GLuint _colorAttribute;
GLuint _normalAttribute;
GLuint _texelAttribute;

//texture uniforms to be passed to vertex shader
GLuint _textureUniform;
//GLuint _primitiveOffsetUniform;

//render AVT_VERTEX_MESH quad with texture lookup shader program definition
//i.e. the shader program that does the video playback
GLuint _meshProg;
GLuint _meshVertexShader;
GLuint _meshFragShader;

//render a video texture into a quad, just like one would render an fbo texture
GLuint _quadVertexBuffer;
GLuint _quadIndexBuffer;


//define pixel buffer objects
static const unsigned char _numPBOs = 4;
GLuint _pboIds[_numPBOs];                   // IDs of PBO
GLubyte * _pboMaps[_numPBOs];				// 

//TO DO: move this, it doesn't belong in shader specific header
static HANDLE _gpgpuIdleEvent = NULL;	
static HANDLE _videoIdleEvent = NULL;


inline unsigned int getNextPowerOfTwo(unsigned int x) {
    x += (x == 0);
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return ++x;
}

//general opengl utility routines
//TO DO:  move somewhere else
void getOpenGLErrors()
{

	GLenum err;
	while((err = glGetError()) != GL_NO_ERROR)
	{
	  //Process/log the error.

		if( err == GL_INVALID_ENUM )
			printf("\nsGL_INVALID_ENUM\n");
		else if( err == GL_INVALID_VALUE )
			printf("\nGL_INVALID_VALUE\n");
		else if( err == GL_INVALID_OPERATION )
			printf("\nGL_INVALID_OPERATION\n");
		else if( err == GL_STACK_OVERFLOW )
			printf("\nGL_STACK_OVERFLOW\n");
		else if( err == GL_STACK_UNDERFLOW )
			printf("\nGL_STACK_UNDERFLOW\n");
		else if( err == GL_OUT_OF_MEMORY )
			printf("\nGL_OUT_OF_MEMORY\n");
		else if( err == GL_INVALID_FRAMEBUFFER_OPERATION )
			printf("\nGL_INVALID_FRAMEBUFFER_OPERATION\n");
		else if( err == GL_CONTEXT_LOST )
			printf("\nGL_CONTEXT_LOST\n");
		else if( err == GL_TABLE_TOO_LARGE )
			printf("\nGL_TABLE_TOO_LARGE\n");

		//assert(1==0);
	}

}

//compile shader routines
//TO DO: move these somewhere else

static char * load_file_to_string(char const * const filepath)
{

    //std::ifstream ifile(fname);
    char * filetext;
	FILE * file;
	unsigned int fileSize;
	//unsigned int filetextSize;
	char line[4096];

    filetext = NULL;
	
#ifdef _WIN32
    errno = fopen_s(&file, filepath, "r"); // open the file in binary mode and check the result //

	if( errno != 0 )
		printf("avt_file_utils::loadFileToString Error %d for filepath: %s\n", errno, filepath);
#else
    file = fopen(filepath, "r");
#endif
    
	if(file)
	{
		//seek to get file size
		fseek(file, 0L, SEEK_END);
		fileSize = (unsigned int)ftell(file);

		//filetextSize = fileSize + 1;  //add room for a \0 character
		filetext = (char *)malloc( (fileSize) * sizeof( char ) );
		filetext[0] = '\0';

		//seek back to the beginning of the ifle
		//fseek(file, 0L, SEEK_SET);
		rewind(file);

		//read the entire file, rather than line by line
		//fread(filetext, fileSize, 1, file);
		//cap the string wtih a null character
		//filetext[fileSize] = '\0';


		
		while (fgets(line, 4096, file))
		{
#ifdef _WIN32
			strcat_s(filetext, fileSize, line);
#else
			strcat(filetext, line);
#endif
		}
		
		fclose(file);
	}



	//printf("\n%s\n", filetext);

    return filetext;
}

GLuint create_shader_string(char * src, GLuint type)
{
	GLuint shader = glCreateShader(type);
	GLchar const *shader_source = src;
    GLint const shader_length = (GLint)strlen(src);
	glShaderSource(shader, 1, &shader_source, &shader_length);
	glCompileShader(shader);
#ifdef AVT_SHADER_DEBUG
	avt_print_shader_info_log(shader);
#endif

	return shader;
}

GLuint create_shader_file(const char * filepath, GLuint type)
{
    GLuint shaderHandle = 0;
	char * src = load_file_to_string(filepath);		//need to release this memory

    if( src )
    {
        shaderHandle = create_shader_string(src,type);
	
        //free file string memory
        if( src )
            free(src);
    }
    
	return shaderHandle;
}


void cleanupShaders()
{
	glUseProgram(0);
	glDetachShader(_meshProg, _meshVertexShader);
	glDetachShader(_meshProg, _meshFragShader);

	glDeleteShader( _meshVertexShader );
	glDeleteShader( _meshFragShader );

	glDeleteProgram(_meshProg);

}


static void createShaderUniformsAndAttributes(GLuint prog)
{

	_vertexAttribute = glGetAttribLocation(prog, AVT_VERTEX_POSITION_SHADER_ATTRIBUTE);
	_colorAttribute = glGetAttribLocation(prog, AVT_VERTEX_COLOR_SHADER_ATTRIBUTE);
	_normalAttribute = glGetAttribLocation(prog, AVT_VERTEX_NORMAL_SHADER_ATTRIBUTE);
	_texelAttribute = glGetAttribLocation(prog, AVT_VERTEX_TEXTURE_UV_SHADER_ATTRIBUTE);
	
	printf("\n After glGetAttribLcoation\n");
		getOpenGLErrors();
	glEnableVertexAttribArray(_vertexAttribute);


	//TO DO:  Why color and normal attributes produce GL_INVALID_VALUE?
	printf("\nAffter vertexAttribute\n");
	getOpenGLErrors();

	//glEnableVertexAttribArray(_colorAttribute);

		printf("\nAffter colorAttribute\n");
	getOpenGLErrors();

	//glEnableVertexAttribArray(_normalAttribute);
	
	printf("\nAffter normalAttribute\n");
	getOpenGLErrors();
	
	glEnableVertexAttribArray(_texelAttribute);

	printf("\nAffter glEnableVertexAttribArray\n");
	getOpenGLErrors();
	
	_projectionUniform = glGetUniformLocation(prog, AVT_PROJECTION_MATRIX_UNIFORM);//"crProjectionMatrix");
	_viewUniform = glGetUniformLocation(prog, AVT_VIEW_MATRIX_UNIFORM);//"crModelViewMatrix");
	_modelViewUniform = glGetUniformLocation(prog, AVT_MODEL_VIEW_MATRIX_UNIFORM);//"crModelViewMatrix");
	_textureUniform = glGetUniformLocation(prog, AVT_VERTEX_TEXTURE_UNIFORM);//"crVertexTexture");
	
	printf("\nCompile Shaders End\n");
}



///////////////////////////////////////////////////////////////////////////////
/// \fn void compileShaders()
/// \brief main
///
/// Description: compile glsl shaders into a glProgram
///
void compileShaders()
{

	//C89 declarations must go at top
	//GLuint vertexShader;
	//GLuint fragmentShader;
	GLuint prog;
	GLint linkSuccess;
	
	//GLint comileSuccess;;
	int infologLength;
	int charsWritten;
	char *infoLog;

	const char * vertexShaderPath = "..\\..\\shaders\\avt_vertex_mesh.es2.vert.glsl";
	const char * fragmentShaderPath = "..\\..\\shaders\\avt_vertex_mesh.es2.frag.glsl";
	
	printf("\nCompile Shaders Start\n");


	//set shader program, is this necessary?
	glUseProgram(0);  
		
	//1 Create and compile shaders
    _meshVertexShader = create_shader_file(vertexShaderPath, GL_VERTEX_SHADER);//glCreateShader(GL_VERTEX_SHADER);
	_meshFragShader = create_shader_file(fragmentShaderPath, GL_FRAGMENT_SHADER);//glCreateShader(GL_FRAGMENT_SHADER);

	printf("\nCompile Shaders 1\n");

	//GLint comileSuccess;;
	infologLength = 0;
	charsWritten = 0;
	infoLog = NULL;

	//check the compile status of the shaders
	glGetShaderiv(_meshVertexShader, GL_INFO_LOG_LENGTH, &infologLength);
	if( infologLength > 0 )
	{
		infoLog = (char*)malloc(infologLength+1);
		if( infoLog )
		{
			glGetShaderInfoLog(_meshVertexShader, infologLength, &charsWritten, infoLog);
		
			if( charsWritten < infologLength) 
			{
				infoLog[charsWritten] = 0;
				printf("\nprintShaderInfoLog: %s\n", infoLog);
			}

			free(infoLog);
			infoLog = NULL;
		}
	}

	//check the compile status of the shaders
	glGetShaderiv(_meshFragShader, GL_INFO_LOG_LENGTH, &infologLength);
	if( infologLength > 0 )
	{
		infoLog = (char*)malloc(infologLength+1);
		if( infoLog )
		{
			glGetShaderInfoLog(_meshFragShader, infologLength, &charsWritten, infoLog);
		
			if( charsWritten < infologLength) 
			{
				infoLog[charsWritten] = 0;
				printf("\nprintShaderInfoLog: %s\n", infoLog);
			}

			free(infoLog);
			infoLog = NULL;
		}
	}


	//glShaderSource(vertexShader, 1, (const GLchar**)&simple_vs, NULL);
	//glCompileShader(vertexShader);
	
	//glShaderSource(fragmentShader, 1, (const GLchar**)&simple_fs, NULL);
	//glCompileShader(fragmentShader);
	
	//2 Create shader program, attach shaders, link program
	_meshProg = glCreateProgram();
	glAttachShader(_meshProg, _meshVertexShader);
	glAttachShader(_meshProg, _meshFragShader);
	glLinkProgram(_meshProg);


	//3 test link success
	glGetProgramiv(_meshProg, GL_LINK_STATUS, &linkSuccess);
	if( linkSuccess == GL_FALSE )
	{
		GLchar messages[256];
		printf("\nLink Shader Program Failed\n");
		glGetProgramInfoLog(_meshProg, sizeof(messages), 0, &messages[0]);
		printf("\n%s\n", messages);
	}

	//4 use program
	glUseProgram(_meshProg);

	//5 establish uniforms for vertex buffer objects
	createShaderUniformsAndAttributes(_meshProg);


}


#endif