
const GLchar * EEGLES2_SHADER_CLIPPED_FS = R"(#define MAX_CLIP_PLANES 6
uniform				sampler2D	textureUnit0;
uniform				int			dgl_TexActive;
#ifndef GL_ES
uniform				int			dgl_ClippingEnabled;
uniform				int			dgl_ClipEnabled[ MAX_CLIP_PLANES ];
#else
uniform	lowp		int			dgl_ClippingEnabled;
uniform	lowp		int			dgl_ClipEnabled[ MAX_CLIP_PLANES ];
#endif
uniform				vec4		dgl_ClipPlane[ MAX_CLIP_PLANES ];
varying				vec4		dgl_Color;
#ifndef GL_ES
varying				vec4		dgl_TexCoord[ 1 ];
#else
varying		mediump	vec4		dgl_TexCoord[ 1 ];
#endif
varying				float		dgl_ClipDistance[ MAX_CLIP_PLANES ];
void main(void)
{
	if ( 1 == dgl_ClippingEnabled ) {
		for ( int i = 0; i < MAX_CLIP_PLANES; i++ ) {
			if ( 1 == dgl_ClipEnabled[i] )
				if ( dgl_ClipDistance[i] < 0.0 )
					discard;
		}
	}
	if ( 1 == dgl_TexActive )
		gl_FragColor = dgl_Color * texture2D( textureUnit0, dgl_TexCoord[ 0 ].xy );
	else
		gl_FragColor = dgl_Color;
}
)";

