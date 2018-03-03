
const GLchar * EEGLES2_SHADER_PRIMITIVE_FS = R"(
varying		vec4		dgl_Color;
void main(void)
{
	gl_FragColor = dgl_Color;
}
)";
