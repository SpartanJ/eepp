
const GLchar * EEGL3CP_SHADER_BASE_VS = R"(
#define MAX_CLIP_PLANES 6
uniform			mat4 dgl_ProjectionMatrix;
uniform			mat4 dgl_ModelViewMatrix;
uniform			mat4 dgl_TextureMatrix;
uniform			int  dgl_ClippingEnabled;
uniform			int	 dgl_ClipEnabled[ MAX_CLIP_PLANES ];
uniform			vec4 dgl_ClipPlane[ MAX_CLIP_PLANES ];
uniform			float dgl_PointSize;
in				vec4 dgl_Vertex;
in				vec4 dgl_FrontColor;
in				vec4 dgl_MultiTexCoord0;
out				vec4 dgl_Color;
out				vec4 dgl_TexCoord[ 1 ];
out				float dgl_ClipDistance[ MAX_CLIP_PLANES ];
void main(void)
{
	gl_PointSize	= dgl_PointSize;
	dgl_Color		= dgl_FrontColor;
	dgl_TexCoord[0]	= dgl_TextureMatrix * dgl_MultiTexCoord0;
	vec4 vEye		= dgl_ModelViewMatrix * dgl_Vertex;
	gl_Position		= dgl_ProjectionMatrix * vEye;
	if ( 1 == dgl_ClippingEnabled ) {
		for ( int i = 0; i < MAX_CLIP_PLANES; i++ ) {
			if ( 1 == dgl_ClipEnabled[i] )
				dgl_ClipDistance[i] = dot( vEye, dgl_ClipPlane[i] );
		}
	}
}
)";
