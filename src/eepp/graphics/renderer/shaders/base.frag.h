
const GLchar * EEGLES2_SHADER_BASE_FS = R"(
uniform	sampler2D	textureUnit0;
varying				vec4 dgl_Color;
#ifndef GL_ES
varying				vec4 dgl_TexCoord[ 1 ];
#else
varying		mediump	vec4 dgl_TexCoord[ 1 ];
#endif
void main(void)
{
	gl_FragColor = dgl_Color * texture2D( textureUnit0, dgl_TexCoord[ 0 ].xy );
}
)";
