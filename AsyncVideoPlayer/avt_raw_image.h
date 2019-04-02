#ifndef _AVT_RAW_IMAGE_H
#define _AVT_RAW_IMAGE_H

//#include "avt_image.h"
//#ifdef _WIN32
//#pragma pack(push, 1)
//#endif

void avt_image_buffer_import_bytes(unsigned char ** imageBuffer, unsigned int width, unsigned int height, unsigned int numChannels, unsigned int depth, const char * filepath);
void avt_image_buffer_export_bytes(unsigned char * imageBuffer, unsigned int width, unsigned int height, unsigned int numChannels, unsigned int depth, const char * filepath);

#endif //_AVT_RAW_IMAGE_H
