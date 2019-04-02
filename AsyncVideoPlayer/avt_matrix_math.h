#ifndef AVT_MATRIX_MATH_H
#define AVT_MATRIX_MATH_H

#include <stdio.h>	//uint#_t on non-windows platforms
#include <stdlib.h> //free, malloc
#include <string.h> // memcpy
#include "stdint.h" //uint#_t on windows platforms

#define _USE_MATH_DEFINES	//holy crap!!! must define this on ms windows to get M_PI definition!
#include <math.h>

//inline doesn't exist in C89, __inline is MSVC specific
#ifndef AVT_MATRIX_MATH_INLINE
#ifdef _WIN32
#define AVT_MATRIX_MATH_INLINE __inline
#else
#define AVT_MATRIX_MATH_INLINE
#endif
#endif

//__decspec doesn't exist in C89, __declspec is MSVC specific
#ifndef AVT_MATRIX_MATH_DECLSPEC
#ifdef _WIN32
#define AVT_MATRIX_MATH_DECLSPEC __declspec
#else
#define JAVT_MATRIX_MATH_DECLSPEC
#endif
#endif

//align functions are diffent on windows vs iOS, Linux, etc.
#ifndef AVT_MATRIX_MATH_ALIGN//(X)
#ifdef _WIN32
#define AVT_MATRIX_MATH_ALIGN(X) (align(X))
#else
#define AVT_MATRIX_MATH_ALIGN(X) __attribute__ ((aligned(X)))
#endif
#endif

//define datatypes needed for storing vertices and/or passing to graphics land
//packed_float2, packed_float3, packed_float4 equivalents
AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(4) typedef union avt_float2_packed
{
    AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(4) struct { float x, y; };
    AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(4) float vector[2];
}avt_float2_packed;

typedef avt_float2_packed avt_packed_float2;


AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(4) typedef union avt_float3_packed
{
    AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(4) struct { float x, y, z; };
    AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(4) float  vector[3];
}avt_float3_packed;


AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(4) typedef union avt_float4_packed // 40 bytes
{
    AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(4) struct { float x, y, z, w; };
    AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(4) struct { float r, g, b, a; };
    AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(4) float  vector[4];
    AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(4) avt_float3_packed float3;		//will allow use of assignment (copy) operator for vbos
    //AVT_VERTEX_MESH_DECLSPEC AVT_VERTEX_MESH_ALIGN(4) __m128 simd;
    
}avt_float4_packed;

typedef avt_float4_packed avt_packed_float4;


//float2, float3, float4 equivalents


AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(16) typedef union avt_float2
{
    AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(16) struct { float x, y; };
    AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(16) float vector[2];
    AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(16) avt_float2_packed packed;		//will allow use of assignment (copy) operator for vbos
    
}avt_float2;


AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(16) typedef union avt_float3
{
    AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(16) struct { float x, y, z; };
    AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(16) float vector[3];
    
}avt_float3;



static avt_float3 avt_float3_add(avt_float3* A, avt_float3* B)
{
    avt_float3 C;
    C.vector[0] = A->vector[0] + B->vector[0];
    C.vector[1] = A->vector[1] + B->vector[1];
    C.vector[2] = A->vector[2] + B->vector[2];
    
    return C;
}

static void avt_float3_normalize(avt_float3 * vector)
{
    float magnitude = sqrt( vector->x*vector->x + vector->y*vector->y + vector->z*vector->z);
    vector->x = vector->x/magnitude;
    vector->y = vector->y/magnitude;
    vector->z = vector->z/magnitude;
}

static avt_float3 avt_float3_cross(avt_float3 * vecA, avt_float3 * vecB)
{
    avt_float3 vecOut;
    vecOut.vector[0] = vecA->vector[1] * vecB->vector[2] - vecB->vector[1]*vecA->vector[2];
    vecOut.vector[1] = vecA->vector[2] * vecB->vector[0] - vecB->vector[2]*vecA->vector[0];
    vecOut.vector[2] = vecA->vector[0] * vecB->vector[1] - vecB->vector[0]*vecA->vector[1];
    
    return vecOut;
    
}

AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(16) typedef union avt_float4 // 48 bytes (multiple of 16 because of simd variable)
{
    AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(16) struct { float x, y, z, w; };
    AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(16) struct { float r, g, b, a; };
    AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(16) float  vector[4];

    AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(16) avt_float4_packed packed;		//will allow use of assignment (copy) operator for vbos
    AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(16) avt_float3 float3;            //will allow use of assignment (copy) operator for vbos
}avt_float4;

AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(16) typedef union avt_double4
{
    AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(16) struct { double x, y, z, w; };
    AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(16) struct { double r, g, b, a; };
    AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(16) double  vector[4];
}avt_double4;

//linked list node structs
AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(16) typedef struct avt_int4_node// 48 bytes (multiple of 16 because of simd variable)
{
    AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(16) int x,y,z,w;
    AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(16) struct avt_int4_node * next;
    
}avt_int4_node;

AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(16) typedef union avt_float4_matrix
{
    AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(16) float  vector[16];
	AVT_MATRIX_MATH_DECLSPEC AVT_MATRIX_MATH_ALIGN(16) avt_float4 vectors[4];			//columns or rows
    struct {
        float m11, m12, m13, m14;
        float m21, m22, m23, m24;
        float m31, m32, m33, m34;
        float m41, m42, m43, m44;
    };
}avt_float4_matrix;


//redefining this shouldn't be a problem
#define AVT_MATRIX_IDENTITY { 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 }

static void avt_float4_matrix_print(avt_float4_matrix * matrix, char * matrixName)
{
    printf("\n%s\n\n", matrixName);
    
//#ifdef CR_MATRIX_USE_COL_MAJOR
    printf("%0.5g\t%0.5g\t%0.5g\t%0.5g\t\n", matrix->vector[0], matrix->vector[4], matrix->vector[8], matrix->vector[12] );
    printf("%0.5g\t%0.5g\t%0.5g\t%0.5g\t\n", matrix->vector[1], matrix->vector[5], matrix->vector[9], matrix->vector[13] );
    printf("%0.5g\t%0.5g\t%0.5g\t%0.5g\t\n", matrix->vector[2], matrix->vector[6], matrix->vector[10], matrix->vector[14] );
    printf("%0.5g\t%0.5g\t%0.5g\t%0.5g\t\n", matrix->vector[3], matrix->vector[7], matrix->vector[11], matrix->vector[15] );
//#else
//    printf("%0.5g\t%0.5g\t%0.5g\t%0.5g\t\n", matrix->vector[0], matrix->vector[1], matrix->vector[2], matrix->vector[3] );
//    printf("%0.5g\t%0.5g\t%0.5g\t%0.5g\t\n", matrix->vector[4], matrix->vector[5], matrix->vector[6], matrix->vector[7] );
//    printf("%0.5g\t%0.5g\t%0.5g\t%0.5g\t\n", matrix->vector[8], matrix->vector[9], matrix->vector[10], matrix->vector[11] );
//    printf("%0.5g\t%0.5g\t%0.5g\t%0.5g\t\n", matrix->vector[12], matrix->vector[13], matrix->vector[14], matrix->vector[15] );
//#endif
    /*
     std::cout << "\n" << matrixName << "\n";
     std::cout << "\n";
     std::cout << matrix[0] << matrix[4] << matrix[8] << matrix[12] << "\n";
     std::cout << matrix[1] << matrix[5] << matrix[9] << matrix[13] << "\n";
     std::cout << matrix[2] << matrix[6] << matrix[10] << matrix[14] << "\n";
     std::cout << matrix[3] << matrix[7] << matrix[11] << matrix[15] << "\n";
     */
    
}



static void avt_float4_matrix_multiply_c(avt_float4_matrix* A, avt_float4_matrix* B, avt_float4_matrix * C)
{
	 float i11, i12, i13, i14;
     float i21, i22, i23, i24;
     float i31, i32, i33, i34;
     float i41, i42, i43, i44;

     i11 = A->m11 * B->m11 + A->m12 * B->m21 + A->m13 * B->m31 + A->m14 * B->m41;    
     i12 = A->m11 * B->m12 + A->m12 * B->m22 + A->m13 * B->m32 + A->m14 * B->m42;
     i13 = A->m11 * B->m13 + A->m12 * B->m23 + A->m13 * B->m33 + A->m14 * B->m43;
     i14 = A->m11 * B->m14 + A->m12 * B->m24 + A->m13 * B->m34 + A->m14 * B->m44;
     i21 = A->m21 * B->m11 + A->m22 * B->m21 + A->m23 * B->m31 + A->m24 * B->m41;
     i22 = A->m21 * B->m12 + A->m22 * B->m22 + A->m23 * B->m32 + A->m24 * B->m42;
     i23 = A->m21 * B->m13 + A->m22 * B->m23 + A->m23 * B->m33 + A->m24 * B->m43;
     i24 = A->m21 * B->m14 + A->m22 * B->m24 + A->m23 * B->m34 + A->m24 * B->m44;
     i31 = A->m31 * B->m11 + A->m32 * B->m21 + A->m33 * B->m31 + A->m34 * B->m41;
     i32 = A->m31 * B->m12 + A->m32 * B->m22 + A->m33 * B->m32 + A->m34 * B->m42;
     i33 = A->m31 * B->m13 + A->m32 * B->m23 + A->m33 * B->m33 + A->m34 * B->m43;
     i34 = A->m31 * B->m14 + A->m32 * B->m24 + A->m33 * B->m34 + A->m34 * B->m44;
     i41 = A->m41 * B->m11 + A->m42 * B->m21 + A->m43 * B->m31 + A->m44 * B->m41;
     i42 = A->m41 * B->m12 + A->m42 * B->m22 + A->m43 * B->m32 + A->m44 * B->m42;
     i43 = A->m41 * B->m13 + A->m42 * B->m23 + A->m43 * B->m33 + A->m44 * B->m43;
     i44 = A->m41 * B->m14 + A->m42 * B->m24 + A->m43 * B->m34 + A->m44 * B->m44;

	 

     C->m11 = i11; C->m12 = i12; C->m13 = i13; C->m14 = i14;
     C->m21 = i21; C->m22 = i22; C->m23 = i23; C->m24 = i24;
     C->m31 = i31; C->m32 = i32; C->m33 = i33; C->m34 = i34;
     C->m41 = i41; C->m42 = i42; C->m43 = i43; C->m44 = i44;
	 

}

#define AVT_MATRIX_COLUMN_MAJOR

static void avt_float4_matrix_multiply(avt_float4_matrix* A, avt_float4_matrix* B, avt_float4_matrix * C)
{
#ifdef AVT_MATRIX_COLUMN_MAJOR
	avt_float4_matrix_multiply_c(B, A, C);
#else
	avt_float4_matrix_multiply_c(A, B, C);
#endif
}

static void avt_float4_matrix_translate(avt_float4_matrix* matrix, float x, float y, float z)
{
    //create a translation matrix
    avt_float4_matrix transMatrix = AVT_MATRIX_IDENTITY;
    transMatrix.vector[12] = x;
    transMatrix.vector[13] = y;
    transMatrix.vector[14] = z;
    
    //and in-place post-multiply the input matrix by the translation matrix
    avt_float4_matrix_multiply(matrix, &transMatrix, matrix);

}

static int avt_float4_matrix_inverse(float * src, float * dst)
{
    float inv[16], det;
    int i;

    inv[0] = src[5]  * src[10] * src[15] - 
             src[5]  * src[11] * src[14] - 
             src[9]  * src[6]  * src[15] + 
             src[9]  * src[7]  * src[14] +
             src[13] * src[6]  * src[11] - 
             src[13] * src[7]  * src[10];

    inv[4] = -src[4]  * src[10] * src[15] + 
              src[4]  * src[11] * src[14] + 
              src[8]  * src[6]  * src[15] - 
              src[8]  * src[7]  * src[14] - 
              src[12] * src[6]  * src[11] + 
              src[12] * src[7]  * src[10];

    inv[8] = src[4]  * src[9] * src[15] - 
             src[4]  * src[11] * src[13] - 
             src[8]  * src[5] * src[15] + 
             src[8]  * src[7] * src[13] + 
             src[12] * src[5] * src[11] - 
             src[12] * src[7] * src[9];

    inv[12] = -src[4]  * src[9] * src[14] + 
               src[4]  * src[10] * src[13] +
               src[8]  * src[5] * src[14] - 
               src[8]  * src[6] * src[13] - 
               src[12] * src[5] * src[10] + 
               src[12] * src[6] * src[9];

    inv[1] = -src[1]  * src[10] * src[15] + 
              src[1]  * src[11] * src[14] + 
              src[9]  * src[2] * src[15] - 
              src[9]  * src[3] * src[14] - 
              src[13] * src[2] * src[11] + 
              src[13] * src[3] * src[10];

    inv[5] = src[0]  * src[10] * src[15] - 
             src[0]  * src[11] * src[14] - 
             src[8]  * src[2] * src[15] + 
             src[8]  * src[3] * src[14] + 
             src[12] * src[2] * src[11] - 
             src[12] * src[3] * src[10];

    inv[9] = -src[0]  * src[9] * src[15] + 
              src[0]  * src[11] * src[13] + 
              src[8]  * src[1] * src[15] - 
              src[8]  * src[3] * src[13] - 
              src[12] * src[1] * src[11] + 
              src[12] * src[3] * src[9];

    inv[13] = src[0]  * src[9] * src[14] - 
              src[0]  * src[10] * src[13] - 
              src[8]  * src[1] * src[14] + 
              src[8]  * src[2] * src[13] + 
              src[12] * src[1] * src[10] - 
              src[12] * src[2] * src[9];

    inv[2] = src[1]  * src[6] * src[15] - 
             src[1]  * src[7] * src[14] - 
             src[5]  * src[2] * src[15] + 
             src[5]  * src[3] * src[14] + 
             src[13] * src[2] * src[7] - 
             src[13] * src[3] * src[6];

    inv[6] = -src[0]  * src[6] * src[15] + 
              src[0]  * src[7] * src[14] + 
              src[4]  * src[2] * src[15] - 
              src[4]  * src[3] * src[14] - 
              src[12] * src[2] * src[7] + 
              src[12] * src[3] * src[6];

    inv[10] = src[0]  * src[5] * src[15] - 
              src[0]  * src[7] * src[13] - 
              src[4]  * src[1] * src[15] + 
              src[4]  * src[3] * src[13] + 
              src[12] * src[1] * src[7] - 
              src[12] * src[3] * src[5];

    inv[14] = -src[0]  * src[5] * src[14] + 
               src[0]  * src[6] * src[13] + 
               src[4]  * src[1] * src[14] - 
               src[4]  * src[2] * src[13] - 
               src[12] * src[1] * src[6] + 
               src[12] * src[2] * src[5];

    inv[3] = -src[1] * src[6] * src[11] + 
              src[1] * src[7] * src[10] + 
              src[5] * src[2] * src[11] - 
              src[5] * src[3] * src[10] - 
              src[9] * src[2] * src[7] + 
              src[9] * src[3] * src[6];

    inv[7] = src[0] * src[6] * src[11] - 
             src[0] * src[7] * src[10] - 
             src[4] * src[2] * src[11] + 
             src[4] * src[3] * src[10] + 
             src[8] * src[2] * src[7] - 
             src[8] * src[3] * src[6];

    inv[11] = -src[0] * src[5] * src[11] + 
               src[0] * src[7] * src[9] + 
               src[4] * src[1] * src[11] - 
               src[4] * src[3] * src[9] - 
               src[8] * src[1] * src[7] + 
               src[8] * src[3] * src[5];

    inv[15] = src[0] * src[5] * src[10] - 
              src[0] * src[6] * src[9] - 
              src[4] * src[1] * src[10] + 
              src[4] * src[2] * src[9] + 
              src[8] * src[1] * src[6] - 
              src[8] * src[2] * src[5];

    det = src[0] * inv[0] + src[1] * inv[4] + src[2] * inv[8] + src[3] * inv[12];

    if (det == 0)
        return -1;

    det = 1.0f / det;

    for (i = 0; i < 16; i++)
        dst[i] = inv[i] * det;

    return 0;
}

static void avt_float4_matrix_look( avt_float4_matrix * matrix, avt_float3 * eyeVector, avt_float3 * centerVector, avt_float3 * upVector )
{
    avt_float3 forward, side, up;
    avt_float4_matrix matrix2 = AVT_MATRIX_IDENTITY;
    avt_float4_matrix resultMatrix = AVT_MATRIX_IDENTITY;
    //------------------
    forward.vector[0] = centerVector->vector[0] - eyeVector->vector[0];
    forward.vector[1] = centerVector->vector[1] - eyeVector->vector[1];
    forward.vector[2] = centerVector->vector[2] - eyeVector->vector[2];
    avt_float3_normalize(&forward);
    //------------------
    //Side = forward x up
    side = avt_float3_cross(&forward, upVector);
    avt_float3_normalize(&side);
    //------------------
    //Recompute up as: up = side x forward
    up = avt_float3_cross(&side, &forward);
    //------------------
    //matrix2 = CR_MATRIX_IDENTITY;
    matrix2.vector[0] = side.vector[0];
    matrix2.vector[4] = side.vector[1];
    matrix2.vector[8] = side.vector[2];
    //matrix2.vector[12] = -eyeVector->vector[0];//0.0;
    //------------------
    matrix2.vector[1] = up.vector[0];
    matrix2.vector[5] = up.vector[1];
    matrix2.vector[9] = up.vector[2];
    //matrix2.vector[13] = -eyeVector->vector[1];//0.0;
    //------------------
    matrix2.vector[2] = -forward.vector[0];
    matrix2.vector[6] = -forward.vector[1];
    matrix2.vector[10] = -forward.vector[2];
    //matrix2.vector[14] = -eyeVector->vector[2];//0.0;
    //------------------
    matrix2.vector[3] = matrix2.vector[7] = matrix2.vector[11] = 0.0;
    //matrix2.vector[15] = 1.0;
    //------------------
    //MultiplyMatrices4by4OpenGL_FLOAT(resultMatrix, matrix, matrix2);
    avt_float4_matrix_multiply(matrix, &matrix2, &resultMatrix);
    
    
    avt_float4_matrix_translate(&resultMatrix, -eyeVector->vector[0], -eyeVector->vector[1], -eyeVector->vector[2]);

    
    //resultMatrix.vector[12] -= eyeVector->vector[0];
    //resultMatrix.vector[13] -= eyeVector->vector[1];
    //resultMatrix.vector[14] -= eyeVector->vector[2];
    
    //glTranslate produces a translation by xyz
    //The current matrix (see glMatrixMode) is multiplied by this translation matrix, with the product replacing the current matrix, as if glMultMatrix were called with the following matrix for its argument:
    //glhTranslatef2(resultMatrix,
    //               -eyeVector->vector[0], -eyeVector->vector[1], -eyeVector->vector[2]);
    
    
    
    
    //------------------
    //memcpy(matrix->vector, resultMatrix->vector, 16*sizeof(float));
    *matrix = resultMatrix; //use copy constructor
    
}

//input fovy is in degress, it needs to be converted to radians for tangent
static void avt_float4_matrix_perspective(avt_float4_matrix* matrix, float fovy, float aspect, float znear, float zfar)
{
	float rad = fovy * (float)M_PI/360.f;
	float h = 1.f / tanf( rad );
	float w = h / aspect;
	float depth = znear - zfar;
	float q = (zfar + znear) / depth;
	float qn = 2.f * zfar * znear / depth;
	
	memset(matrix, 0, sizeof(avt_float4_matrix));

	matrix->vector[0] = w;
	matrix->vector[5] = h;
	matrix->vector[10] = q;
	matrix->vector[11] = -1.f;
	matrix->vector[14] = qn;


 /*
	  float xymax = znear * tan(fovy * M_PI/360.f);
  float ymin = -xymax;
  float xmin = -xymax;
 
  float width = xymax - xmin;
  float height = xymax - ymin;
 
  float depth = zfar - znear;
  float q = -(zfar + znear) / depth;
  float qn = -2 * (zfar * znear) / depth;
 
  float w = 2 * znear / width;
  float h = 2 * znear / height;
  w = w / aspect;

  matrix->vector[0]  = w;
  matrix->vector[1]  = 0;
  matrix->vector[2]  = 0;
  matrix->vector[3]  = 0;
 
  matrix->vector[4]  = 0;
  matrix->vector[5]  = h;
  matrix->vector[6]  = 0;
  matrix->vector[7]  = 0;
 
  matrix->vector[8]  = 0;
  matrix->vector[9]  = 0;
  matrix->vector[10] = q;
  matrix->vector[11] = -1;
 
  matrix->vector[12] = 0;
  matrix->vector[13] = 0;
  matrix->vector[14] = qn;
  matrix->vector[15] = 0;
  */


}


static const avt_float4_packed avt_float4_PACKED_UP = {0, 1, 0, 0};
static const avt_float2_packed avt_float2_PACKED_ZERO = { 0, 0 };
static const uint32_t AVT_VERTEX_MESH_INT_INVALID_INDEX = 0xffff;

#endif