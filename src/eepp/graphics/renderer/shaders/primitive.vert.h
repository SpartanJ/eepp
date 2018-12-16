
const GLchar * EEGLES2_SHADER_PRIMITIVE_VS = R"(
uniform			mat4 dgl_ProjectionMatrix;
uniform				mat4 dgl_ModelViewMatrix;
attribute			vec4 dgl_Vertex;
attribute			vec4 dgl_FrontColor;
varying				vec4 dgl_Color;
void main(void)
{
	dgl_Color		= dgl_FrontColor;
	gl_Position		= dgl_ProjectionMatrix * ( dgl_ModelViewMatrix * dgl_Vertex );
}
)";
