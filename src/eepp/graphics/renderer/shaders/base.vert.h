
const GLchar * EEGLES2_SHADER_BASE_VS = R"(
uniform			mat4 dgl_ProjectionMatrix;
uniform				mat4 dgl_ModelViewMatrix;
uniform				mat4 dgl_TextureMatrix;
attribute			vec4 dgl_Vertex;
attribute			vec4 dgl_FrontColor;
attribute			vec4 dgl_MultiTexCoord0;
varying				vec4 dgl_Color;
#ifndef GL_ES
varying				vec4 dgl_TexCoord[ 1 ];
#else
varying		mediump	vec4 dgl_TexCoord[ 1 ];
#endif
void main(void)
{
	dgl_Color		= dgl_FrontColor;
	dgl_TexCoord[0]	= dgl_TextureMatrix * dgl_MultiTexCoord0;
	gl_Position		= dgl_ProjectionMatrix * ( dgl_ModelViewMatrix * dgl_Vertex );
}
)";
