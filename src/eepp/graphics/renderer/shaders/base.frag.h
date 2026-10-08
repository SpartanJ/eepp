
const GLchar * EEGLES2_SHADER_BASE_FS = R"(
uniform	sampler2D	textureUnit0;
uniform	int			dgl_TextureColorMode;
uniform	vec3		dgl_TextureColorChannel;
varying				vec4 dgl_Color;
#ifndef GL_ES
varying				vec4 dgl_TexCoord[ 1 ];
#else
varying		mediump	vec4 dgl_TexCoord[ 1 ];
#endif
void main(void)
{
	vec4 texel = texture2D( textureUnit0, dgl_TexCoord[ 0 ].xy );
	if ( 0 == dgl_TextureColorMode )
		gl_FragColor = dgl_Color * texel;
	else {
		float coverage = dot( texel.rgb, dgl_TextureColorChannel );
		gl_FragColor = vec4( dgl_Color.rgb, dgl_Color.a * coverage );
	}
}
)";
