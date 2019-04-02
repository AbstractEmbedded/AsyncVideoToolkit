#ifndef AVT_QUAD_VBO_H
#define AVT_QUAD_VBO_H


#if defined(__cplusplus)
extern "C" {
#endif

#include "avt_matrix_math.h"

//Since we only need to render quad geometry for the purposes of this application right now, we omit JRMVertexMesh object API
//and simply redefine its VBO here and define a quad based on it for passing vertex data to the shader pipeline

//A mesh encapsulates an array of vertices (vertex buffer objects to be exact)
//to be more specific: an array of ordered indices to vertices in the vertex (buffer object) array describing the tris (facets)
/*AVT_VERTEX_MESH_DECLSPEC (align(4))*/ typedef struct avt_vertex_mesh_vbo // 56 bytes
{
    /*AVT_VERTEX_MESH_DECLSPEC (align(4))*/ avt_float4_packed vertex;
    /*AVT_VERTEX_MESH_DECLSPEC (align(4))*/ avt_float4_packed color;
    /*AVT_VERTEX_MESH_DECLSPEC (align(4))*/ avt_float4_packed normal;
    /*AVT_VERTEX_MESH_DECLSPEC (align(4))*/ avt_float2_packed texel;
    
}avt_vbo;

#define AVT_TEX_COORD_MAX 1

//Define unit Quad Geometry Vertex Buffer and associated Index Buffer ready to be passed to the avt_vertex_mesh.vert.glsl shader
static const avt_vertex_mesh_vbo AVT_QUAD_VBO[] = {
    {{0.5, -0.5, 0, 1}, {1, 1, 1, 1}, {0,0,-1,1}, {AVT_TEX_COORD_MAX, 0}},
    {{0.5, 0.5, 0, 1}, {1, 1, 1, 1}, {0,0,-1,1}, {AVT_TEX_COORD_MAX, AVT_TEX_COORD_MAX}},
    {{-0.5, 0.5, 0, 1}, {1, 1, 1, 1}, {0,0,-1,1}, {0, AVT_TEX_COORD_MAX}},
    {{-0.5, -0.5, 0, 1}, {1, 1, 1, 1}, {0,0,-1,1}, {0, 0}}
};

static const unsigned char AVT_QUAD_VBO_INDICES[] = {
    0, 1, 2,
    2, 3, 0,
};


#if defined(__cplusplus)
}
#endif



#endif