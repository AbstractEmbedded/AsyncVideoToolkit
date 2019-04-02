#version 120 //for gles2 glsl compatibility

#ifdef GL_ES
precision highp float;
precision highp int;
#else
#define highp
#define mediump
#define lowp
#endif

//varying vec4 avtFragmentColor;
varying vec2 avtFragmentTextureUV;
uniform sampler2D avtVertexTexture;

void main(void)
{
	//crFragmentTextureUV.y *= -1.0;
	
	vec4 rgba = texture2D(avtVertexTexture, avtFragmentTextureUV);
    gl_FragColor = vec4(rgba.xyz, 1.0);//vec4(scaledTemp, scaledTemp, scaledTemp, rgba.a);//rgba;//vec4(rgba.x, rgba.y, rgba.z, 1);
}
