"#define MAX_CLIP_PLANES 6\n\
#ifdef GL_ES\n\
precision lowp float;\n\
precision lowp int;\n\
#else\n\
#version 120\n\
#endif\n\
uniform				mat4 dgl_ProjectionMatrix;\n\
uniform				mat4 dgl_ModelViewMatrix;\n\
attribute			vec4 dgl_Vertex;\n\
attribute			vec4 dgl_FrontColor;\n\
attribute			vec4 dgl_MultiTexCoord0;\n\
varying				vec4 dgl_Color;\n\
#ifndef GL_ES\n\
varying				vec4 dgl_TexCoord[ 1 ];\n\
#else\n\
varying		mediump	vec4 dgl_TexCoord[ 1 ];\n\
#endif\n\
void main(void)\n\
{\n\
	dgl_Color		= dgl_FrontColor;\n\
	dgl_TexCoord[0]	= dgl_MultiTexCoord0;\n\
	vec4 vEye		= dgl_ModelViewMatrix * dgl_Vertex;\n\
	gl_Position		= dgl_ProjectionMatrix * vEye;\n\
}\n\
";
