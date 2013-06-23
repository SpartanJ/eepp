"#ifdef GL_ES\n\
precision lowp float;\n\
precision lowp int;\n\
#else\n\
#version 120\n\
#endif\n\
uniform		sampler2D	textureUnit0;\n\
varying		vec4		dgl_Color;\n\
void main(void)\n\
{\n\
	gl_FragColor = dgl_Color * texture2D( textureUnit0, gl_PointCoord );\n\
}";
