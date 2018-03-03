
const GLchar * EEGL3CP_SHADER_BASE_FS = R"(
#define MAX_CLIP_PLANES 6
uniform		sampler2D	textureUnit0;
uniform		int			dgl_TexActive;
uniform		int			dgl_PointSpriteActive;
uniform		int			dgl_ClippingEnabled;
uniform		int			dgl_ClipEnabled[ MAX_CLIP_PLANES ];
uniform		vec4		dgl_ClipPlane[ MAX_CLIP_PLANES ];
in			vec4		dgl_Color;
in			vec4		dgl_TexCoord[ 1 ];
in			float		dgl_ClipDistance[ MAX_CLIP_PLANES ];
out			vec4		dgl_FragColor;
void main(void)
{
	if ( 1 == dgl_ClippingEnabled ) {
		for ( int i = 0; i < MAX_CLIP_PLANES; i++ ) {
			if ( 1 == dgl_ClipEnabled[i] )
				if ( dgl_ClipDistance[i] < 0.0 )
					discard;
		}
	}
	if ( 0 == dgl_PointSpriteActive ) {
		if ( 1 == dgl_TexActive )
			dgl_FragColor = dgl_Color * texture2D( textureUnit0, dgl_TexCoord[ 0 ].xy );
		else
			dgl_FragColor = dgl_Color;
	} else
		dgl_FragColor = dgl_Color * texture2D( textureUnit0, gl_PointCoord );
}
)";
