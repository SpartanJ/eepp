#ifdef EE_GLES2
"precision mediump float;\n"
"precision lowp int;\n"
#else
"#version 120\n"
#endif
"uniform	sampler2D	textureUnit0;\n\
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
