
const GLchar * EEGLES2_SHADER_POINTSPRITE_VS = R"(
uniform				mat4 dgl_ProjectionMatrix;
uniform				mat4 dgl_ModelViewMatrix;
uniform				float dgl_PointSize;
attribute			vec4 dgl_Vertex;
attribute			vec4 dgl_FrontColor;
varying				vec4 dgl_Color;
void main(void)
{
	gl_PointSize	= dgl_PointSize;
	dgl_Color		= dgl_FrontColor;
	gl_Position		= dgl_ProjectionMatrix * ( dgl_ModelViewMatrix * dgl_Vertex );
}
)";
