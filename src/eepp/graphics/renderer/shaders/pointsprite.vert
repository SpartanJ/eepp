"#define MAX_CLIP_PLANES 6\n\
#ifdef GL_ES\n\
precision lowp float;\n\
precision lowp int;\n\
#else\n\
#version 120\n\
#endif\n\
uniform				mat4 dgl_ProjectionMatrix;\n\
uniform				mat4 dgl_ModelViewMatrix;\n\
uniform				float dgl_PointSize;\n\
attribute			vec4 dgl_Vertex;\n\
attribute			vec4 dgl_FrontColor;\n\
varying				vec4 dgl_Color;\n\
void main(void)\n\
{\n\
	gl_PointSize	= dgl_PointSize;\n\
	dgl_Color		= dgl_FrontColor;\n\
	gl_Position		= dgl_ProjectionMatrix * ( dgl_ModelViewMatrix * dgl_Vertex );\n\
}";
