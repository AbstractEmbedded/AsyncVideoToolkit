
#ifndef _AVT_IMAGE_BUFFER_H_
#define _AVT_IMAGE_BUFFER_H_

//inline doesn't exist in C89, __inline is MSVC specific
#ifndef AVT_IMAGE_INLINE
#ifdef _WIN32
#define AVT_IMAGE_INLINE __inline
#else
#define AVT_IMAGE_INLINE
#endif
#endif

#include <stdlib.h>
#include "crMath.h"
//#include "crMath/cr_byte.h"
//#include "crMath/cr_float.h"
//#include "crMath/cr_uint.h"
//#include "crUtils/cr_inttypes.h"
#include <math.h>
//#include "crMath/cr_float_api.h"

//AVT_IMAGE_API AVT_IMAGE_INLINE avt_image avt_image_buffer_create(unsigned int width, unsigned int height, unsigned int numChannels, unsigned int depth, AVT_IMAGE_FORMAT imageFormat);


// *** private static rasterize functions *** //

//static void * avt_image_buffer_rasterize_float2_points(unsigned char ** imageBuffer, unsigned int width, unsigned int height, unsigned int numChannels, cr_float4* point_coordinates, unsigned int numCoordinates, cr_float4 color_value);

/*
AVT_IMAGE_API AVT_IMAGE_INLINE AVT_IMAGE_API_STATUS avt_image_buffer_rasterize_value_to_uint16_x_y_point(unsigned char * imageBuffer, unsigned int width, unsigned int height, unsigned int numChannels, unsigned int channelDepthInBytes, uint16_t x, uint16_t y, cr_byte4_packed *color_value);
AVT_IMAGE_API AVT_IMAGE_INLINE AVT_IMAGE_API_STATUS avt_image_buffer_rasterize_value_to_uint2_point(unsigned char * imageBuffer, unsigned int width, unsigned int height, unsigned int numChannels, unsigned int channelDepthInBytes, cr_uint2 *point_coordinate, cr_byte4_packed *color_value);
AVT_IMAGE_API AVT_IMAGE_INLINE AVT_IMAGE_API_STATUS avt_image_buffer_rasterize_value_to_float2_point(unsigned char * imageBuffer, unsigned int width, unsigned int height, unsigned int numChannels, unsigned int channelDepthInBytes, cr_float2 *point_coordinate, cr_byte4_packed color_value);
AVT_IMAGE_API AVT_IMAGE_INLINE AVT_IMAGE_API_STATUS avt_image_buffer_rasterize_value_to_uint2_points(unsigned char * imageBuffer, unsigned int width, unsigned int height, unsigned int numChannels, unsigned int channelDepthInBytes, cr_uint2* point_coordinates, unsigned int numCoordinates, cr_byte4_packed *color_value);
AVT_IMAGE_API AVT_IMAGE_INLINE AVT_IMAGE_API_STATUS avt_image_buffer_rasterize_value_to_float2_points(unsigned char * imageBuffer, unsigned int width, unsigned int height, unsigned int numChannels, unsigned int channelDepthInBytes, cr_float2* point_coordinates, unsigned int numCoordinates, cr_byte4_packed color_value);

// #define max(a,b) x ^ ((x ^ y) & -(x < y)) // max(x, y)
// #define min(x,y) y ^ ((x ^ y) & -(x < y)) // min(x, y);

AVT_IMAGE_API AVT_IMAGE_INLINE AVT_IMAGE_API_STATUS avt_image_buffer_rasterize_circles_from_uint16_x_y_points( unsigned char * imageBuffer, unsigned int width, unsigned int height, unsigned int numChannels, unsigned int bytesPerChannel, bool normalized, uint16_t* circle_center_x_values, uint16_t* circle_center_y_values, unsigned int numCircles, unsigned int radius, cr_byte4_packed *color_value);
AVT_IMAGE_API AVT_IMAGE_INLINE AVT_IMAGE_API_STATUS avt_image_buffer_rasterize_circles_from_uint2_points( unsigned char * imageBuffer, unsigned int width, unsigned int height, unsigned int numChannels, unsigned int bytesPerChannel, bool normalized, cr_uint2* circle_centers, unsigned int numCircles, unsigned int radius, cr_byte4_packed *color_value);
// *** Public API Functions *** //


AVT_IMAGE_API AVT_IMAGE_INLINE AVT_IMAGE_API_STATUS avt_image_draw_uint_pixels(AVT_IMAGE * image, cr_uint2* point_coordinates, unsigned int numCoordinates, cr_byte4_packed *color_value);
AVT_IMAGE_API AVT_IMAGE_INLINE AVT_IMAGE_API_STATUS avt_image_draw_circles_uint16(AVT_IMAGE * image, uint16_t* circle_center_x_values, uint16_t* circle_center_y_values, unsigned int numCircles, unsigned int radius, cr_byte4_packed *color_value);
AVT_IMAGE_API AVT_IMAGE_INLINE AVT_IMAGE_API_STATUS avt_image_draw_circles(AVT_IMAGE * image, cr_uint2* circle_centers, unsigned int numCircles, unsigned int radius, cr_byte4_packed *color_value);
AVT_IMAGE_API AVT_IMAGE_INLINE AVT_IMAGE_API_STATUS avt_image_draw_float_pixels(AVT_IMAGE * image, cr_float2* point_coordinates, unsigned int numCoordinates, cr_byte4_packed color_value);
*/

#endif //_AVT_IMAGE_BUFFER_H_
