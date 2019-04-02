//
//  avt_byte_utils.h
//  JRM Vertex Mesh API
//
//  Created by Joe Moulton on 2/1/17.
//  Copyright © 2017 JRM Technologies. All rights reserved.
//

#ifndef avt_byte_utils_h
#define avt_byte_utils_h


#if (defined(_WIN16) || defined(_WIN32) || defined(_WIN64)) && !defined(__WINDOWS__)

#	define __WINDOWS__

#endif


/** compatibility header for endian.h
 * This is a simple compatibility shim to convert
 * BSD/Linux endian macros to the Mac OS X equivalents.
 * It is public domain.
 * */

//8 bit stubs are here for posterity, such as when using x-macro expansion for serializing with a function macro
#define htobe8(x) x
#define htole8(x) x
#define be8toh(x) x
#define le8toh(x) x

//Platform specific byte conversion functions
#ifdef __APPLE__

#include <libkern/OSByteOrder.h>

#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)

#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)

#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)

#elif defined(__OpenBSD__)

#	include <sys/endian.h>

#	define be16toh(x) betoh16(x)
#	define le16toh(x) letoh16(x)

#	define be32toh(x) betoh32(x)
#	define le32toh(x) letoh32(x)

#	define be64toh(x) betoh64(x)
#	define le64toh(x) letoh64(x)

#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__DragonFly__)

#	include <sys/endian.h>

#	define be16toh(x) betoh16(x)
#	define le16toh(x) letoh16(x)

#	define be32toh(x) betoh32(x)
#	define le32toh(x) letoh32(x)

#	define be64toh(x) betoh64(x)
#	define le64toh(x) letoh64(x)

#elif defined(__WINDOWS__)

#include <stdlib.h>
//#	include <winsock2.h>
//#	include <sys/param.h>
//#pragma comment(lib, "Ws2_32.lib")
#	if BYTE_ORDER == LITTLE_ENDIAN

/*
 #		define htobe16(x) htons(x)
 #		define htole16(x) (x)
 #		define be16toh(x) ntohs(x)
 #		define le16toh(x) (x)
 
 #		define htobe32(x) htonl(x)
 #		define htole32(x) (x)
 #		define be32toh(x) ntohl(x)
 #		define le32toh(x) (x)
 
 #		define htobe64(x) htonll(x)
 #		define htole64(x) (x)
 #		define be64toh(x) ntohll(x)
 #		define le64toh(x) (x)
 */

#		define htobe16(x) _byteswap_ushort(x)
#		define htole16(x) (x)
#		define be16toh(x) _byteswap_ushort(x)
#		define le16toh(x) (x)

#		define htobe32(x) _byteswap_ulong(x)
#		define htole32(x) (x)
#		define be32toh(x) _byteswap_ulong(x)
#		define le32toh(x) (x)

#		define htobe64(x) _byteswap_uint64(x)
#		define htole64(x) (x)
#		define be64toh(x) _byteswap_uint64(x)
#		define le64toh(x) (x)


#	elif BYTE_ORDER == BIG_ENDIAN

/* that would be xbox 360 */
#		define htobe16(x) (x)
#		define htole16(x) __builtin_bswap16(x)
#		define be16toh(x) (x)
#		define le16toh(x) __builtin_bswap16(x)

#		define htobe32(x) (x)
#		define htole32(x) __builtin_bswap32(x)
#		define be32toh(x) (x)
#		define le32toh(x) __builtin_bswap32(x)

#		define htobe64(x) (x)
#		define htole64(x) __builtin_bswap64(x)
#		define be64toh(x) (x)
#		define le64toh(x) __builtin_bswap64(x)

#	else

#		error byte order not supported

#	endif

#	define __BYTE_ORDER    BYTE_ORDER
#	define __BIG_ENDIAN    BIG_ENDIAN
#	define __LITTLE_ENDIAN LITTLE_ENDIAN
#	define __PDP_ENDIAN    PDP_ENDIAN

#else

//#	error platform not supported

#endif

#include <stdio.h>      /* printf, scanf, NULL */
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

static void avt_print_data_as_binary(unsigned * data, unsigned int numBitsToPrint)
{
    unsigned
    input = *data,
    n_bits = numBitsToPrint,
    *bits = (unsigned*)malloc(sizeof(unsigned) * n_bits),
    bit = 0;
    
    for(bit = 0; bit < n_bits; ++bit, input >>= 1)
        bits[bit] = input & 1;;
    
    for(bit = n_bits; bit--;)
        printf("%u", bits[bit]);
    
    free(bits);
}

//FYI:  client must free returned memory
static unsigned * avt_convert_to_binary(unsigned * data, unsigned int numBitsToPrint)
{
    unsigned
    input = *data,
    n_bits = numBitsToPrint,
    *bits = (unsigned*)malloc(sizeof(unsigned) * n_bits),
    bit = 0;
    
    for(bit = 0; bit < n_bits; ++bit, input >>= 1)
        bits[bit] = input & 1;;
    
    return bits;
}

#ifdef __cplusplus
    }
#endif

#endif /* avt_byte_utils_h */
