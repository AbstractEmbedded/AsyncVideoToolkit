#ifndef _AVT_IMAGE_H_
#define _AVT_IMAGE_H_


#if defined(__cplusplus)
extern "C" {
#endif

#include "avt_raw_image.h"

typedef enum AVT_IMAGE_API_STATUS
{
	AVT_IMAGE_API_SUCCESS				= 0,
	AVT_IMAGE_API_INVALID_FILE			= 1,
	AVT_IMAGE_API_NULL_CONTAINER			= 2,
	AVT_IMAGE_API_INCOMPATIBLE_FOURCC	= 3,
	AVT_IMAGE_API_INPUT_FILE_ERROR		= 4,
	AVT_IMAGE_API_INVALID_FILE_STRUCTURE = 5,
	AVT_IMAGE_API_UNSUPPORTED_FORMAT		= 6,
	AVT_IMAGE_API_UNHANDLED_PIXEL_FORMAT	= 7,
	AVT_IMAGE_API_UNHANDLED_FOURCC_FORMAT= 8,
	AVT_IMAGE_API_UNABLE_TO_READ_DATA	= 9,
	AVT_IMAGE_API_BUFFER_IS_NULL			= 10
}AVT_IMAGE_API_STATUS;

typedef enum AVT_IMAGE_FORMAT
{
	AVT_IMAGE_FORMAT_BYTES					= 0
	//AVT_IMAGE_FORMAT_BMP					= 0,
	//AVT_IMAGE_FORMAT_PNG					= 1,
	//AVT_IMAGE_FORMAT_DDS					= 2,
	//AVT_IMAGE_FORMAT_SGI					= 3
}AVT_IMAGE_FORMAT;

typedef struct avt_image
{
	unsigned int width, height, depth, numChannels, size;
	unsigned int pixelInternalFormat, pixelFormat, pixelType;
	unsigned int numMipMaps, mipMapSize;
	AVT_IMAGE_FORMAT format;
	AVT_IMAGE_API_STATUS status;
	unsigned char * pixelData;			//currently a buffer allocated by stb_image, unless .dds
	char * filepath;
	char * extension;

}AVT_IMAGE;

void avt_print_image_details(AVT_IMAGE * img);

//compute image size
unsigned int avt_image_compute_size( AVT_IMAGE * img);


void avt_free_image( AVT_IMAGE *img );


AVT_IMAGE avt_load_image(const char * filepath);					//automatically uncompress compressed formats
//AVT_IMAGE_API AVT_IMAGE_INLINE AVT_IMAGE cr_load_image_compressed(const char * filepath);		//leave compressed formats compressed


AVT_IMAGE avt_image_buffer_create(unsigned int width, unsigned int height, unsigned int numChannels, unsigned int depth, AVT_IMAGE_FORMAT imageFormat);
AVT_IMAGE_API_STATUS avt_image_export_to_source_format(AVT_IMAGE * image, const char * filepath);

#if defined(__cplusplus)
}
#endif

#endif //_AVT_IMAGE_H_
