#ifdef EE_GL3_ENABLED
"precision mediump float;\n"
"precision lowp int;\n"
#else
"#version 120\n"
#endif
"varying		vec4		dgl_Color;\n\
void main(void)\n\
{\n\
	gl_FragColor = dgl_Color;\n\
}";
