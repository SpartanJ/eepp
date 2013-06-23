#ifdef EE_GL3_ENABLED
"precision mediump float;\n"
"precision lowp int;\n"
#else
"#version 120\n"
#endif
"#define MAX_CLIP_PLANES 6\n\
uniform				sampler2D	textureUnit0;\n\
uniform				int			dgl_TexActive;\n\
uniform				int			dgl_ClippingEnabled;\n\
uniform				int			dgl_ClipEnabled[ MAX_CLIP_PLANES ];\n\
uniform				vec4		dgl_ClipPlane[ MAX_CLIP_PLANES ];\n\
varying				vec4		dgl_Color;\n\
#ifndef GL_ES\n\
varying				vec4		dgl_TexCoord[ 1 ];\n\
#else\n\
varying		mediump	vec4		dgl_TexCoord[ 1 ];\n\
#endif\n\
varying				float		dgl_ClipDistance[ MAX_CLIP_PLANES ];\n\
void main(void)\n\
{\n\
	if ( 1 == dgl_ClippingEnabled ) {\n\
		for ( int i = 0; i < MAX_CLIP_PLANES; i++ ) {\n\
			if ( 1 == dgl_ClipEnabled[i] )\n\
				if ( dgl_ClipDistance[i] < 0.0 )\n\
					discard;\n\
		}\n\
	}\n\
	if ( 1 == dgl_TexActive )\n\
		gl_FragColor = dgl_Color * texture2D( textureUnit0, dgl_TexCoord[ 0 ].xy );\n\
	else\n\
		gl_FragColor = dgl_Color;\n\
}";

