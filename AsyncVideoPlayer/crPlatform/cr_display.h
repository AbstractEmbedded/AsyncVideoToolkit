
#ifndef cr_display_h
#define cr_display_h

#ifdef __cplusplus
extern "C" {
#endif

//#include "crMath/cr_float.h"
//#include "crMath/cr_long.h"
//#include "crMath/cr_ulong.h"
//#include "crMath/cr_short.h"


//only define bool if not previoulsy defined by CoreRender crWindow global header
//and not using C++
#ifndef __cplusplus
//typedef unsigned char bool;
#ifndef bool
#define bool int
#define true 1
#define false 0
#endif
#endif

typedef union cr_ulong2
{
	struct { unsigned long x, y; };
	unsigned long vector[2];
	//CR_MATH_DECLSPEC CR_MATH_ALIGN(16) cr_ulong2_packed packed;		//will allow use of assignment (copy) operator for vbos
}cr_ulong2;

typedef union cr_float2
{
    struct { float x, y; };
    float vector[2];
    //CR_MATH_DECLSPEC CR_MATH_ALIGN(16) cr_float2_packed packed;		//will allow use of assignment (copy) operator for vbos
    
}cr_float2;



	
//inline doesn't exist in C89, __inline is MSVC specific
#ifndef CR_PLATFORM_INLINE
#ifdef _WIN32
#define CR_PLATFORM_INLINE __inline
#else
#define CR_PLATFORM_INLINE
#endif
#endif

//__decspec doesn't exist in C89, __declspec is MSVC specific
#ifndef CR_PLATFORM_DECLSPEC
#ifdef _WIN32
#define CR_PLATFORM_DECLSPEC __declspec
#else
#define CR_PLATFORM_DECLSPEC
#endif
#endif

//align functions are diffent on windows vs iOS, Linux, etc.
#ifndef CR_PLATFORM_ALIGN//(X)
#ifdef _WIN32
#define CR_PLATFORM_ALIGN(X) (align(X))
#else
#define CR_PLATFORM_ALIGN(X) __attribute__ ((aligned(X)))
#endif
#endif


cr_ulong2 cr_display_get_resolution();
cr_float2 cr_display_get_dimensions();

#ifdef __cplusplus
}
#endif

#endif