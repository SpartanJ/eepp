
const GLchar * EEGL3CP_SHADER_BASE_FS = R"(
#define MAX_CLIP_PLANES 6
uniform		sampler2D	textureUnit0;
uniform		int			dgl_TexActive;
uniform		int			dgl_TextureColorMode;
uniform		vec3		dgl_TextureColorChannel;
uniform		int			dgl_PointSpriteActive;
uniform		int			dgl_ClippingEnabled;
uniform		int			dgl_ClipEnabled[ MAX_CLIP_PLANES ];
uniform		vec4		dgl_ClipPlane[ MAX_CLIP_PLANES ];
varying		vec4		dgl_Color;
varying		vec4		dgl_TexCoord[ 1 ];
varying		float		dgl_ClipDistance[ MAX_CLIP_PLANES ];
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
		if ( 1 == dgl_TexActive ) {
			vec4 texel = texture2D( textureUnit0, dgl_TexCoord[ 0 ].xy );
			if ( 0 == dgl_TextureColorMode )
				gl_FragColor = dgl_Color * texel;
			else {
				float coverage = dot( texel.rgb, dgl_TextureColorChannel );
				gl_FragColor = vec4( dgl_Color.rgb, dgl_Color.a * coverage );
			}
		}
		else
			gl_FragColor = dgl_Color;
	} else
		gl_FragColor = dgl_Color * texture2D( textureUnit0, gl_PointCoord );
}
)";
