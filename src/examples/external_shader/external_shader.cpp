#include <eepp/ee.hpp>

/// This example is based on the WebGL demo from http://minimal.be/lab/fluGL/
Uint32 ParticlesNum	= 30000;
cWindow * win;
cShaderProgram * ShaderProgram;
eeFloat tw;
eeFloat th;
eeFloat aspectRatio;

void videoResize() {
	/// Video Resize event will re-setup the 2D projection and states, so we must rebuild them.
	aspectRatio	= (eeFloat)win->GetWidth()	/ (eeFloat)win->GetHeight();
	tw			= (eeFloat)win->GetWidth()	/ 2;
	th			= (eeFloat)win->GetHeight()	/ 2;

	eeFloat fieldOfView	= 30.0;
	eeFloat nearPlane	= 1.0;
	eeFloat farPlane	= 10000.0;
	eeFloat top			= nearPlane * eetan(fieldOfView * EE_PI / 360.0);
	eeFloat bottom		= -top;
	eeFloat right		= top * aspectRatio;
	eeFloat left		= -right;

	eeFloat a = (right + left) / (right - left);
	eeFloat b = (top + bottom) / (top - bottom);
	eeFloat c = (farPlane + nearPlane) / (farPlane - nearPlane);
	eeFloat d = (2 * farPlane * nearPlane) / (farPlane - nearPlane);
	eeFloat x = (2 * nearPlane) / (right - left);
	eeFloat y = (2 * nearPlane) / (top - bottom);
	GLfloat perspectiveMatrix[16] = {
		x, 0, a, 0,
		0, y, b, 0,
		0, 0, c, d,
		0, 0, -1, 0
	};

	/// Load the our default projection
	GLi->MatrixMode( GL_PROJECTION );
	GLi->LoadMatrixf( perspectiveMatrix );
	GLi->MatrixMode( GL_MODELVIEW );

	/// eepp enables some this client states by default, and textures by default
	GLi->Disable( GL_TEXTURE_2D );
	GLi->DisableClientState( GL_TEXTURE_COORD_ARRAY );
	GLi->DisableClientState( GL_COLOR_ARRAY );
	/// GL_VERTEX_ARRAY is needed, so we keep it enabled
	GLi->EnableClientState( GL_VERTEX_ARRAY );

	/// Reset the default blend func ( by default eepp use ALPHA_NORMAL )
	cTextureFactory::instance()->SetPreBlendFunc( ALPHA_BLENDONE );

	/// Set the line width
	cGlobalBatchRenderer::instance()->SetLineWidth( 2 );

	/// Rebind the Shader
	ShaderProgram->Bind();

	/// If you want to use the fixed-pipeline renderer you'll need to set up the projection and modelview matrix manually.
	/// Or if you want to use another name to the projection matrix or the modelview matrix ( programmable pipelines use
	/// dgl_ProjectionMatrix and dgl_ModelViewMatrix by default.
	if ( GLv_2 == GLi->Version() ) {
		ShaderProgram->SetUniformMatrix( "dgl_ProjectionMatrix", perspectiveMatrix );

		/// Get the identity matrix and set it to the modelview matrix
		GLfloat modelMatrix[16];
		GLi->LoadIdentity();
		GLi->GetCurrentMatrix( GL_MODELVIEW_MATRIX, modelMatrix );

		ShaderProgram->SetUniformMatrix( "dgl_ModelViewMatrix", modelMatrix );
	}
}

EE_MAIN_FUNC int main (int argc, char * argv [])
{
	win = cEngine::instance()->CreateWindow( WindowSettings( 960, 640, 32, WindowStyle::Default, "", "eepp - External Shaders" ), ContextSettings( true ) );

	if ( win->Created() )
	{
		cInput * imp = win->GetInput();

		/// Disable the automatic shader conversion from fixed-pipeline to programmable-pipeline
		cShader::Ensure = false;

		std::string fs( "#ifdef GL_ES\n\
			precision highp float;\n\
			#endif\n\
			void main() { gl_FragColor = vec4(0.4, 0.01, 0.08, 0.5); }" );

		std::string vs( "#ifdef GL_ES\n\
			precision highp float;\n\
			#endif\n\
			attribute vec3 dgl_Vertex;\n\
			uniform mat4 dgl_ProjectionMatrix;\n\
			uniform mat4 dgl_ModelViewMatrix;\n\
			void main()	{ gl_Position = dgl_ProjectionMatrix * dgl_ModelViewMatrix * vec4(dgl_Vertex, 1.0); }" );

		/// Create the new shader program
		ShaderProgram = eeNew( cShaderProgram, ( vs.c_str(), vs.size(), fs.c_str(), fs.size() ) );

		/// Set the projection
		videoResize();

		/// Push a window resize callback the reset the projection when needed
		win->PushResizeCallback( cb::Make0( &videoResize ) );

		Uint32 i;
		eeVector3ff * vertices		= eeNewArray( eeVector3ff, ParticlesNum );
		eeVector3ff * velocities	= eeNewArray( eeVector3ff, ParticlesNum );

		for (i = 0; i < ParticlesNum; i++ )
		{
			vertices[i]		= eeVector3ff( 0, 0, 1.83 );
			velocities[i]	= eeVector3ff( (eeRandf() * 2 - 1)*.05, (eeRandf() * 2 - 1)*.05, .93 + eeRandf()*.02 );
		}

		while ( win->Running() )
		{
			imp->Update();

			if ( imp->IsKeyDown( KEY_ESCAPE ) )
			{
				win->Close();
			}

			eeFloat p;
			eeVector2f mf	= imp->GetMousePosf();
			eeFloat tratio	= tw / th;
			eeFloat touchX	= ( mf.x / tw - 1 ) * tratio;
			eeFloat touchY	= -( mf.y / th - 1 );
			bool touch		= imp->MouseLeftPressed();

			for( i = 0; i < ParticlesNum; i+=2 )
			{
				// copy old positions
				vertices[i].x = vertices[i+1].x;
				vertices[i].y = vertices[i+1].y;

				// inertia
				velocities[i].x *= velocities[i].z;
				velocities[i].y *= velocities[i].z;

				// horizontal
				p = vertices[i+1].x;
				p += velocities[i].x;

				if ( p < -aspectRatio ) {
					p = -aspectRatio;
					velocities[i].x = eeabs(velocities[i].x);
				} else if ( p > aspectRatio ) {
					p = aspectRatio;
					velocities[i].x = -eeabs(velocities[i].x);
				}
				vertices[i+1].x = p;

				// vertical
				p = vertices[i+1].y;
				p += velocities[i].y;
				if ( p < -aspectRatio ) {
					p = -aspectRatio;
					velocities[i].y = eeabs(velocities[i].y);
				} else if ( p > aspectRatio ) {
					p = aspectRatio;
					velocities[i].y = -eeabs(velocities[i].y);

				}
				vertices[i+1].y = p;

				if ( touch ) {
					eeFloat dx	= touchX - vertices[i].x;
					eeFloat dy	= touchY - vertices[i].y;
					eeFloat d	= eesqrt( dx * dx + dy * dy );

					if ( d < 2.f ) {
						if ( d < 0.03f ) {
							vertices[i+1].x = ( eeRandf() * 2 - 1 ) * aspectRatio;
							vertices[i+1].y = eeRandf() * 2 - 1;
							velocities[i].x = 0;
							velocities[i].y = 0;
						} else {
							dx /= d;
							dy /= d;
							d = ( 2 - d ) / 2;
							d *= d;
							velocities[i].x += dx * d * .01;
							velocities[i].y += dy * d * .01;
						}
					}
				}
			}

			/// VertexPointer assigns values by default to the attribute "dgl_Vertex"
			/// ColorPointer to "dgl_FrontColor"
			/// TextureCoordPointer to "dgl_MultiTexCoord0"
			GLi->VertexPointer( 3, GL_FP, sizeof(eeVector3ff), reinterpret_cast<char*> ( &vertices[0] ), 0 );

			/// Draw the lines
			GLi->DrawArrays( DM_LINES, 0, ParticlesNum );

			win->Display();
		}

		eeSAFE_DELETE_ARRAY( vertices );
		eeSAFE_DELETE_ARRAY( velocities );
	}

	cEngine::DestroySingleton();

	EE::MemoryManager::LogResults();

	return 0;
}
