#include "avt_image.h"

//#include "crUtils/cr_file_utils.h"

#include <stdio.h>      // printf
//#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"
//#include "stbi_DDS_aug.h"
//#include "stbi_DDS_aug_c.h"

#include <stdio.h>      // printf
#include <stdlib.h>		// strtol
#include <ctype.h>		// isspace
#include <string.h>


static const char* avt_find_file_extension(const char *fileNameOrPath) 
{
    const char *dot = strrchr(fileNameOrPath, '.');
    if(!dot || dot == fileNameOrPath) return "";
    return dot + 1;
}


void avt_print_image_details(AVT_IMAGE * img)
{
    if( img )
    {
        printf("\n***avt_image->filepath = %s\n", img->filepath);
        printf("***avt_image->width = %d\n", img->width);
        printf("***avt_image->height = %d\n", img->height);
        printf("***avt_image->depth = %d\n", img->depth);
        printf("***avt_image->numChannels = %d\n", img->numChannels);
        printf("***avt_image->size = %d\n", img->size);
    
        if( img->pixelData )
        {
            printf("***avt_image->pixelData = VALID MEMORY\n");
        
        }
        else
        {
            printf("***avt_image->pixelData = NULL\n");

        }
    }
    else
           printf("avt_image_print_details ::  Unable to print image.  Image context is NULL\n");
}

unsigned int avt_image_compute_size( AVT_IMAGE * img)
{
    return img->width * img->height * img->numChannels * img->depth  ;//avt_image_compute_size_in_bytes(img->width, img->height, img->depth, img->pixelFormat, img->pixelType, 1, 1, 1);
}



void avt_free_image( AVT_IMAGE *img )
{
	if( img->filepath )
		free(img->filepath);
	img->filepath = NULL;

	if( img->pixelData )
		_aligned_free(img->pixelData);
		//free(img->pixelData);
		//stbi_image_free( img->pixelData );
}



AVT_IMAGE avt_load_image(const char * filepath)
{
	AVT_IMAGE img = {0};

	//alloc mem for filepath
	img.filepath = (char*)malloc( (strlen(filepath)+1) * sizeof(char) );
	//img.filepath[strlen(filepath)] = '\0';
	strcpy(img.filepath, filepath);
    img.filepath[strlen(filepath)] = '\0';
    
	//find file extension in filepath, point to it
	img.extension = (char *)avt_find_file_extension(img.filepath);	

    //printf("\ndds file extension: %s\n", img.extension);
    //start with some assumptions about an image
    //img.depth = 1;
    //img.pixelFormat = CRGC_RGB;
    //img.pixelType = CRGC_UNSIGNED_BYTE;

	//TO DO: incorporate bmp and SGI image loaders
	//but for now we just need raw uncompressed bytes
	/*
	if( strcmp(img.extension, "bmp") == 0 || strcmp(img.extension, "BMP") == 0 )
	{
		//printf("\nLoading BMP:	%s\n", filepath);
		
		//cr_load_png_image(&img);
        
        //img.pixelFormat = CRGC_RGB;
        //img.pixelType = CRGC_UNSIGNED_BYTE;
        
        //img.size = avt_image_compute_size(&img);

	}
    else if( strcmp(img.extension, "rgb") == 0 || strcmp(img.extension, "RGB") == 0 )
    {
        //printf("\nLoading BMP:	%s\n", filepath);
        
        //cr_load_png_image(&img);
        
        //img.pixelFormat = CRGC_RGB;
        //img.pixelType = CRGC_UNSIGNED_BYTE;
        
        //img.size = avt_image_compute_size(&img);
        
    }
	*/

	/*
	if( strcmp(img.extension, "rgba") == 0 || strcmp(img.extension, "bgra") == 0 )
	{
		img.numChannels = 4;
		img.depth = 8;
		img.width = 
	}
	*/

    //determine pixel format and size because stb_image does not do this internally
    if( img.numChannels  == 4 )
    {
        //img.pixelFormat = CRGC_RGBA;
    }
    else if( img.numChannels == 3 )
    {
        //img.pixelFormat = CRGC_RGB;
    }
    else if( img.numChannels == 2 )
    {
        printf("\n img.numChannels == 2 not yet supported\n");
        
    }
    else if( img.numChannels == 1)
    {
        printf("\n img.numChannels == 1 not yet supported\n");
    }
    else
    {
        printf("\n avt_image error:  img.numChannels == 0\n");
    }
    
    //img.pixelType = CRGC_UNSIGNED_BYTE;
    //img->numChannels = avt_image_compute_pixel_format_components(img->pixelFormat);
    img.size = avt_image_compute_size(&img);
    

	return img;

}


AVT_IMAGE avt_image_buffer_create(unsigned int width, unsigned int height, unsigned int numChannels, unsigned int depth, AVT_IMAGE_FORMAT imageFormat)
{
    AVT_IMAGE image = {0};
    
    
    // TO DO:  check input values
    
    image.width = width;
    image.height = height;
    image.numChannels = numChannels;
    image.depth = depth;
    image.format = imageFormat;
    
    //calculate total bytes, even if bit depth of a single channel is less than 8 bits (ie 1 byte)
    image.size = width * height * numChannels * (depth/8);
    
    image.pixelData =  (unsigned char*)_aligned_malloc(  image.size, 16);  //(unsigned char*)malloc( image.size);//(unsigned char*)_aligned_malloc( image.size, 16 );
    memset( image.pixelData, 0, image.size ); //set to black
    
    return image;
    
}


AVT_IMAGE_API_STATUS avt_image_export_to_source_format(AVT_IMAGE * image, const char * filepath)
{
	if( !image )
		return AVT_IMAGE_API_NULL_CONTAINER;

	if( !image->pixelData )
		return AVT_IMAGE_API_BUFFER_IS_NULL;

	switch( image->format )
	{

		case AVT_IMAGE_FORMAT_BYTES:
			//avt_image_buffer_export_bytes(image->pixelData, image->width, image->height, image->numChannels, image->depth, filepath);
			break;

		//case AVT_IMAGE_FORMAT_BMP:
			//avt_image_buffer_export_to_bmp(image->pixelData, image->width, image->height, image->numChannels, image->depth, filepath);
			//break;


		default:
			return AVT_IMAGE_API_UNSUPPORTED_FORMAT;

	}

	return AVT_IMAGE_API_SUCCESS;

}


