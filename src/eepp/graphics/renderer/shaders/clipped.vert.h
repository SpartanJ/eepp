
const GLchar * EEGLES2_SHADER_CLIPPED_VS = R"(
#define MAX_CLIP_PLANES 6
uniform				mat4 dgl_ProjectionMatrix;
uniform				mat4 dgl_ModelViewMatrix;
uniform				mat4 dgl_TextureMatrix;
#ifndef GL_ES
uniform				int			dgl_ClippingEnabled;
uniform				int			dgl_ClipEnabled[ MAX_CLIP_PLANES ];
#else
uniform	lowp		int			dgl_ClippingEnabled;
uniform	lowp		int			dgl_ClipEnabled[ MAX_CLIP_PLANES ];
#endif
uniform				vec4 dgl_ClipPlane[ MAX_CLIP_PLANES ];
attribute			vec4 dgl_Vertex;
attribute			vec4 dgl_FrontColor;
attribute			vec4 dgl_MultiTexCoord0;
varying				vec4 dgl_Color;
#ifndef GL_ES
varying				vec4		dgl_TexCoord[ 1 ];
#else
varying		mediump	vec4		dgl_TexCoord[ 1 ];
#endif
varying				float dgl_ClipDistance[ MAX_CLIP_PLANES ];
void main(void)
{
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
