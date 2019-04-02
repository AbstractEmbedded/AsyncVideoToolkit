#include "avt_raw_image.h"

#include <stdio.h>      // printf
#include <stdlib.h>		// strtol
#include <ctype.h>		// isspace
#include <string.h>

#include <assert.h>

void avt_image_buffer_import_bytes(unsigned char ** imageBuffer, unsigned int width, unsigned int height, unsigned int numChannels, unsigned int depth, const char * filepath)
{
	FILE * file;
	size_t fileLength;
	
	unsigned long buffer_size;	
	buffer_size = width * height * numChannels * (depth/8);

	//allocate memory for the buffer, but first we might as well open the file and ensure the buffer size agrees
	
	/* open the file for reading bytes*/

#ifdef _WIN32 // || LINUX
    errno = fopen_s(&file, filepath, "rb"); // open the file in binary mode and check the result //

	//if( errno != 0 )
	//{
	//	printf("\ncr_dds_image unable to open file\n", errno);
		//return CR_IMAGE_API_INVALID_FILE;
	//}
#else
     file = fopen(filepath, "rb");
#endif

	//assert(file);

	//seek to end of file to get byte offset to know the total size of file
	//fseek(file, 0, SEEK_END);
	//fileLength = ftell(file);

	//ensure the image file is larger than the buffer we've allocated to store it in
	//printf("\n fileLength = %lu\n", fileLength);
	//printf("\n buffer_size = %lu\n", buffer_size);
	//assert( fileLength <= buffer_size );

	//Now allocate memory for the buffer and read
	// TO DO:  ensure buffer is 16 byte aligned when allocated
	//(*imageBuffer) = (unsigned char*)malloc( buffer_size * sizeof(unsigned char) );

	//seek back to beginning of file for reading
	//fseek(file, 0, SEEK_SET); //rewind(file);

	//read the entire image into buffer as one chunk of bytes
	fread((*imageBuffer), buffer_size, 1, file);


	/* close the file */
	fclose( file );
}

void avt_image_buffer_export_bytes(unsigned char * imageBuffer, unsigned int width, unsigned int height, unsigned int numChannels, unsigned int depth, const char * filepath)
{
	FILE * file;
	
	unsigned int buffer_size;	
	buffer_size = width * height * numChannels * (depth/8);

	/* open the file for writing bytes*/
	file = fopen (filepath,"wb");

	assert(file);

	//write the contents of the buffer
	fwrite(imageBuffer, buffer_size, 1, file);

	//close the file
	fclose(file);
}