#ifndef SGI_IMAGE_H
#define SGI_IMAGE_H


#include <stdio.h>              // includes standard input/output routines
#include "avt_byte_utils.h"

typedef struct //12 bytes
{
	short magic;
	char  storage;				//0 = uncompressed; 1 = RLE (Run Length Encoding)
	char  bpc;					//bytes per pixel channel
	unsigned short dimension;	//number of dimensions
	unsigned short xsize;		//image row size
	unsigned short ysize;		//num rows in image
	unsigned short zsize;		//num channels in image

	//ignore the remaining 512 bytes of the header, not needed

}sgi_header_data;

typedef struct
{
	unsigned int width, height;
    int bytesPerChannel, bitsPerChannel;
    char rle;
    short dimension;
    unsigned int tablen;
    unsigned int *offsetTable, *lengthTable;
    unsigned char * tmp;
} sgi_image_data;

static bool readSGIImageHeader(FILE * file, sgi_image_data *info, unsigned int * width, unsigned int * height, unsigned int * numChannels)
{

	//bool status = false;

	//unsigned char header[12]
    sgi_header_data sgi_header;

	//read 12 bytes sgi header struct
    fread(&sgi_header, 12, 1, file);
        
    //convert 16 bits from big endian to little endian
    sgi_header.magic = be16toh(sgi_header.magic);
    
	//rgb file is in Big Endian byte order.  stbi functions will get in the requested format and convert to host format
    if (sgi_header.magic != 474 ) {printf("\nreadSGIImageHeader failed.  Magic number does not indicate SGI file format.\n"); return false; }

    info->rle = sgi_header.storage; // discard storage format
    //if( info->rle == 1 ) {printf("\nRun Length Encoding (RLE) not supported for RGB\n"); }//return stbi__errpuc("Run Length Encoding (RLE) not supported for RGB", "Unsupported RGB");}
    
    info->bytesPerChannel = sgi_header.bpc;

    if( info->bytesPerChannel == 2 ) { printf("\nreadSGIImageHeader failed. RGB 16-bit Channel Precision currently not supported\n"); return false;}

    //info->numChannelsPerPixel = (int)info->bytesPerChannel * 8;
    info->dimension = be16toh(sgi_header.dimension);
    //x component row size
    *width = (unsigned int)be16toh(sgi_header.xsize);
	info->width = *width;
    //y component, image height
    *height = (unsigned int)be16toh(sgi_header.xsize);
    info->height = *height;
	//z component, num channels
    *numChannels = (unsigned int)be16toh(sgi_header.zsize);
    
	//if the header dimension is not 3, then the zsize may be 0
	//in which case, the number of channels is then presumed to be 1
    if( info->dimension != 3 )
       (*numChannels) = 1;
	else if( (*numChannels < 1) && (width > 0 && height > 0) )
		(*numChannels) = 1;
    
    //printf("\nMCM dimesnion = %d", info->dimension);
    //printf("\nMCM num channels = %d", *numChannels);
    //printf("\nMCM channel size: %d\n", info->bytesPerChannel);
    //printf("\nsizeof(unsigned) == %lu", sizeof(unsigned));
    
    info->bitsPerChannel = info->bytesPerChannel * 8 * (*numChannels);
    
	//seek forward past the rest of the header
    fseek(file, 500, SEEK_CUR);
    
    if( info->rle == 1 )
    {
        //read the two table containing the offset
        unsigned int tableIndex = 0;
        //unsigned int channelIndex = 0;
        
        //length of each table is determined by img.height * img.numChannels
        info->tablen = (*height)*(*numChannels)*sizeof(unsigned int);
        
        //allocate memory to store table data, must free memory when finished file load
        info->offsetTable = (unsigned int *)malloc(info->tablen);
        info->lengthTable = (unsigned int *)malloc(info->tablen);
        
        //read the table entry by entry as big endian, converting to host format (little endian)
        for( tableIndex = 0; tableIndex < info->tablen/sizeof(unsigned int); tableIndex++)
        {
			fread(&(info->offsetTable[tableIndex]), 4, 1, file);
            info->offsetTable[tableIndex] = be32toh(info->offsetTable[tableIndex]);
            //printf("\n offset[%u]:  %u", tableIndex, info->offsetTable[tableIndex]);
        }
        
        for( tableIndex = 0; tableIndex < info->tablen/sizeof(unsigned int); tableIndex++)
        {
            fread(&(info->lengthTable[tableIndex]), 4, 1, file);
            info->lengthTable[tableIndex] = be32toh(info->lengthTable[tableIndex]);
			//printf("\n length[%u]:  %u", tableIndex, info->offsetTable[tableIndex]);
        }

    }
    

    return true;
}

static void readSGIScanlineRLE(FILE *file, sgi_image_data * info, unsigned char *buf, int rowIndex, int channelIndex)
{
    unsigned char *iPtr, *oPtr, pixel;
    int count;
    
    unsigned long tableIndex, fileOffset, rleScanlineLength;
    
    //retrieve the file offset of the channel/row index scanline from the table
    tableIndex = rowIndex + channelIndex * info->height;
    fileOffset = info->offsetTable[tableIndex];
    rleScanlineLength = info->lengthTable[tableIndex];
    
    //seek to the file offset of the scanline we need
    fseek(file, fileOffset, SEEK_SET);

    //read the rle scanline entry into a temporary buffer which must be preallocated to a large enough size
    fread(info->tmp, (unsigned int)rleScanlineLength, 1, file);
    
    iPtr = info->tmp;
    oPtr = buf;
    for (;;) {
        pixel = *iPtr++;
        count = (int)(pixel & 0x7F);
        if (!count) {
            return;
        }
        if (pixel & 0x80) {
            while (count--) {
                *oPtr++ = *iPtr++;
            }
        } else {
            pixel = *iPtr++;
            while (count--) {
                *oPtr++ = pixel;
            }
        }
    }
}

static unsigned char * loadSGIImage(const char * pathToImage, unsigned int * width, unsigned int * height, unsigned int * numChannels)
{
	unsigned char *out, *outPtr;
    unsigned char **channelBuffers;

	out=NULL;

	 //open file just once
    FILE * file;
#if defined(_WIN32)
    errno = fopen_s(&file, pathToImage, "rb"); // open the file and check the result //
    
    if( errno != 0 )
    {
        printf("Error (%d) trying to open file %s \n", errno, pathToImage);
        return NULL;
    }
#else
    file = fopen(filepath, "rb");
#endif
    
    if( !file ) return NULL;
    
    
    //stbi_uc *rbuf, *bbuf, *gbuf, *abuf;
    
    //unsigned int rowIndex;

    int i,j;
	unsigned int rowIndex;
    unsigned int rowWidth;
    unsigned int bufferSize;
    unsigned int bufferIndex;

    //unsigned int mr=0,mg=0,mb=0,ma=0, all_a;
    //stbi_uc pal[256][4];
    
    int flip_vertically, target;
    sgi_image_data info;
    
	target = (*numChannels); // if they want monochrome, we'll post-convert
    
    //info.all_a = 255;
    if (readSGIImageHeader(file, &info, width, height, numChannels ) == NULL)
        return NULL; // error code already set
    
	if( target == 0 )
		target = (*numChannels);

    //flip_vertically = ((int) (*height)) > 0;
    //(*height) = abs((int) (*height));
       

    //allocate memory for n image channel buffers
    channelBuffers = (unsigned char**)malloc(sizeof(unsigned char*) * (*numChannels));
    //allocate memory of image row size for each channel buffer
    rowWidth = (*width)*info.bytesPerChannel;
    
    //allocate memory buffer to store the unpacked pixel buffer
    //only allocate now if channel is greater than 1, otherwise allocate when reading directly to buffer
    //if( target > 1 )
    out = (unsigned char *) malloc(target * rowWidth * (*height) * sizeof(unsigned char));
    outPtr = out;

    
    if( info.rle == 1 )
    {
        
        bufferSize = rowWidth;
        for(i=0; i<(*numChannels); i++)
        {
            channelBuffers[i] = (unsigned char*)malloc(bufferSize);
        }

        //create a temporary buffer of row size
        info.tmp = (unsigned char*)malloc(rowWidth* 256);

        //iterate over each scanline of the target buffer (i.e. the height/number of rows of the image)
        for (rowIndex=0; rowIndex<(*height); rowIndex++)
        {
            //read the image bytes as a contiguous chunk for each channel
            for(i=0; i<(*numChannels); i++)
            {
                //will copy memory read from file into buffer
                readSGIScanlineRLE(file, &info, channelBuffers[i], rowIndex, i);
                //stbi__getn(s, channelBuffers[i], bufferSize);
            }
            
            //copy the channel buffers to the target buffer
            //TO DO: unwrap double for loop
            for (bufferIndex=0; bufferIndex<bufferSize; bufferIndex++)
            {
                for(j=0;j<target;j++)
                {
                    //if channel index is less than num target channels and num img channels
                    //then we are guaranteed to have read that channel from file
                    
                    //assume the num target buffer channels is always greater than or equal to the number of image channels
                    //so, if the channel buffers exist, we can copy its values to the target buffer
					
                    if( target > *numChannels)
                    {
                        if( *numChannels == 1) //case:  1 input channel
                        {
                            if( j == 3 )
                                outPtr[j] = 0xff;
                            else
                                outPtr[j] = channelBuffers[0][bufferIndex];
                            
                        }
                        else if( *numChannels == 2) //case: 2 input channels, consider them to be a grayscale value and an alpha
                        {
                            if( j == 3 )
                                outPtr[j] = channelBuffers[1][bufferIndex];
                            else
                                outPtr[j] = channelBuffers[0][bufferIndex];
                        }
                        else if( *numChannels == 3 )
                        {
                            if( j==3 )          //case: 3 input channels, consider them to be rgb
                                outPtr[j] = 0xff;
                            else
                                outPtr[j] = channelBuffers[j][bufferIndex];
                            
                        }
                    }
                    else //if( channelBuffers[j] )
                    {
					
                        //if( j==3 )          //case: 3 input channels, consider them to be rgb
                        //    outPtr[j] = 0xff;
                        //else
                            outPtr[j] = channelBuffers[j][bufferIndex];
                    }
                }
                
                outPtr += target;

            }
        }
        
        //reset the file pointer to where it was, so we don't break stbi
        //fseek((FILE*)(s->io_user_data), 512 + 2*info.tablen, SEEK_SET);

        
        //cleanup intermediate file data
        if(info.offsetTable)
            free(info.offsetTable);
        if( info.lengthTable )
            free(info.lengthTable);
        if( info.tmp )
            free(info.tmp);

    }
    else
    {
        //allocate memory to read the raw channel bytes from the image
        bufferSize = rowWidth * (*height);
        for(i=0; i<(*numChannels); i++)
        {
            channelBuffers[i] = (unsigned char*)malloc(bufferSize);
        }

        
        //read the image bytes as a contiguous chunk for each channel
        for(i=0; i<(*numChannels); i++)
        {
            //copy memory read from file into buffer
            fread(channelBuffers[i], bufferSize, 1, file);
        }
        
        //copy the channel buffers to the target buffer
        //TO DO: unwrap double for loop
        for (bufferIndex=0; bufferIndex<bufferSize; bufferIndex++)
        {
            for(j=0;j<target;j++)
            {
                //if channel index is less than num target channels and num img channels
                //then we are guaranteed to have read that channel from file
                
                //assume the num target buffer channels is always greater than or equal to the number of image channels
                //so, if the channel buffers exist, we can copy its values to the target buffer
				
                if( target > *numChannels)
                {
                    if( *numChannels == 1) //case:  1 input channel
                    {
                        if( j == 3 )
                            outPtr[j] = 0xff;
                        else
                            outPtr[j] = channelBuffers[0][bufferIndex];
                        
                    }
                    else if( *numChannels == 2) //case: 2 input channels, consider them to be a grayscale value and an alpha
                    {
                        if( j == 3 )
                            outPtr[j] = channelBuffers[1][bufferIndex];
                        else
                            outPtr[j] = channelBuffers[0][bufferIndex];
                    }
                    else if( *numChannels == 3 )
                    {
                        if( j==3 )          //case: 3 input channels, consider them to be rgb
                            outPtr[j] = 0xff;
                        else
                            outPtr[j] = channelBuffers[j][bufferIndex];
                        
                    }
                }
                else //if( channelBuffers[j] )
				
                    outPtr[j] = channelBuffers[j][bufferIndex];
                
            }
            
            outPtr += target;

        }

    }
    
    
    //free the channel buffers, we are done with them
    for(i=0; i<(*numChannels); i++)
    {
        free(channelBuffers[i]);
        channelBuffers[i] = NULL;
    }

    free(channelBuffers);
    channelBuffers = NULL;
    
    
    //flip the buffer if desired
    /*
    if (1) {
        stbi_uc t;
        for (j=0; j < (int) s->img_y>>1; ++j) {
            stbi_uc *p1 = out +      j     *s->img_x*target;
            stbi_uc *p2 = out + (s->img_y-1-j)*s->img_x*target;
            for (i=0; i < (int) s->img_x*target; ++i) {
                t = p1[i], p1[i] = p2[i], p2[i] = t;
            }
        }
    }
     */
    
    (*numChannels) = target;
    
    //*x = s->img_x;
    //*y = s->img_y;
    //if (comp) *comp = s->img_n;
    
	fclose(file);

    return out;
}

#endif