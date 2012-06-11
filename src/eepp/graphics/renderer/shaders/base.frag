"#define MAX_CLIP_PLANES 6\n\
#ifdef GL_ES\n\
precision lowp float;\n\
precision lowp int;\n\
#else\n\
#version 120\n\
#endif\n\
uniform		sampler2D	textureUnit0;\n\
varying				vec4 dgl_Color;\n\
#ifndef GL_ES\n\
varying				vec4 dgl_TexCoord[ 1 ];\n\
#else\n\
varying		mediump	vec4 dgl_TexCoord[ 1 ];\n\
#endif\n\
void main(void)\n\
{\n\
	gl_FragColor = dgl_Color * texture2D( textureUnit0, dgl_TexCoord[ 0 ].xy );\n\
}";

