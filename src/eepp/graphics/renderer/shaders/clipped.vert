#ifdef EE_GL3_ENABLED
"precision mediump float;\n"
"precision lowp int;\n"
#else
"#version 120\n"
#endif
"#define MAX_CLIP_PLANES 6\n\
uniform				mat4 dgl_ProjectionMatrix;\n\
uniform				mat4 dgl_ModelViewMatrix;\n\
uniform				int  dgl_ClippingEnabled;\n\
uniform				int	 dgl_ClipEnabled[ MAX_CLIP_PLANES ];\n\
uniform				vec4 dgl_ClipPlane[ MAX_CLIP_PLANES ];\n\
attribute			vec4 dgl_Vertex;\n\
attribute			vec4 dgl_FrontColor;\n\
attribute			vec4 dgl_MultiTexCoord0;\n\
varying				vec4 dgl_Color;\n\
#ifndef GL_ES\n\
varying				vec4		dgl_TexCoord[ 1 ];\n\
#else\n\
varying		mediump	vec4		dgl_TexCoord[ 1 ];\n\
#endif\n\
varying				float dgl_ClipDistance[ MAX_CLIP_PLANES ];\n\
void main(void)\n\
{\n\
	dgl_Color		= dgl_FrontColor;\n\
	dgl_TexCoord[0]	= dgl_MultiTexCoord0;\n\
	vec4 vEye		= dgl_ModelViewMatrix * dgl_Vertex;\n\
	gl_Position		= dgl_ProjectionMatrix * vEye;\n\
	if ( 1 == dgl_ClippingEnabled ) {\n\
		for ( int i = 0; i < MAX_CLIP_PLANES; i++ ) {\n\
			if ( 1 == dgl_ClipEnabled[i] )\n\
				dgl_ClipDistance[i] = dot( vEye, dgl_ClipPlane[i] );\n\
		}\n\
	}\n\
}";
