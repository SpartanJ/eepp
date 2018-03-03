
const GLchar * EEGLES2_SHADER_POINTSPRITE_FS = R"(
uniform	sampler2D	textureUnit0;
varying	vec4		dgl_Color;
void main(void)
{
	gl_FragColor = dgl_Color * texture2D( textureUnit0, gl_PointCoord );
}
)";
