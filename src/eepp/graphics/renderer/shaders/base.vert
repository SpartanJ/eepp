#ifdef EE_GLES2
"precision mediump float;\n"
"precision lowp int;\n"
#else
"#version 120\n"
#endif
"uniform			mat4 dgl_ProjectionMatrix;\n\
uniform				mat4 dgl_ModelViewMatrix;\n\
uniform				mat4 dgl_TextureMatrix;\n\
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
	dgl_TexCoord[0]	= dgl_TextureMatrix * dgl_MultiTexCoord0;\n\
	gl_Position		= dgl_ProjectionMatrix * ( dgl_ModelViewMatrix * dgl_Vertex );\n\
}\n\
";
