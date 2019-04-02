#version 120

attribute vec4 avtVertexPosition;
//attribute vec4 avtVertexColor;
//attribute vec4 avtertexNormal;
attribute vec2 avtVertexTextureUV;

varying vec2 avtFragmentTextureUV;
//varying vec4 avtFragmentColor;


uniform mat4 avtProjectionMatrix;
uniform mat4 avtViewMatrix;
//uniform mat4 avtViewInverseMatrix;
uniform mat4 avtModelViewMatrix;

uniform sampler2D avtVertexTexture;


void main(void) 
{

	gl_Position = avtProjectionMatrix * avtModelViewMatrix * vec4(avtVertexPosition.x, avtVertexPosition.y, avtVertexPosition.z, 1.);//vertex;
	avtFragmentTextureUV = vec2(avtVertexTextureUV.x, avtVertexTextureUV.y);
}
