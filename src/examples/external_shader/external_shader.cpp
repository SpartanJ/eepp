#include <eepp/ee.hpp>
#include <eepp/graphics/opengl.hpp>

/// This example is based on the WebGL demo from http://minimal.be/lab/fluGL/
namespace Demo_ExternalShader {

#if defined( EE_ARM ) || EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
static Float sqrt_aprox[20001];
#endif

Uint32 ParticlesNum	= 30000;

EE::Window::Window * win = NULL;
Input * imp = NULL;
ShaderProgram * shaderProgram = NULL;
bool ShadersSupported = false;
Float tw;
Float th;
Float aspectRatio;
Vector3ff * vertices		= eeNewArray( Vector3ff, ParticlesNum );
Vector3ff * velocities	= eeNewArray( Vector3ff, ParticlesNum );
ColorAf * colors			= eeNewArray( ColorAf, ParticlesNum );

void videoResize( EE::Window::Window * w ) {
	/// Video Resize event will re-setup the 2D projection and states, so we must rebuild them.
	aspectRatio	= (Float)win->getWidth()	/ (Float)win->getHeight();
	tw			= (Float)win->getWidth()	/ 2;
	th			= (Float)win->getHeight()	/ 2;

	float fieldOfView	= 30.0;
	float nearPlane	= 1.0;
	float farPlane	= 10000.0;
	float top			= nearPlane * eetan(fieldOfView * EE_PI_360);
	float bottom		= -top;
	float right		= top * aspectRatio;
	float left		= -right;

	float a = (right + left) / (right - left);
	float b = (top + bottom) / (top - bottom);
	float c = (farPlane + nearPlane) / (farPlane - nearPlane);
	float d = (2 * farPlane * nearPlane) / (farPlane - nearPlane);
	float x = (2 * nearPlane) / (right - left);
	float y = (2 * nearPlane) / (top - bottom);

	float perspectiveMatrix[16] = {
		x, 0, a, 0,
		0, y, b, 0,
		0, 0, c, d,
		0, 0, -1, 0
	};

	/// Load the our default projection
	GLi->matrixMode( GL_PROJECTION );
	GLi->loadMatrixf( perspectiveMatrix );
	GLi->matrixMode( GL_MODELVIEW );

	/// eepp enables some client states by default, and textures by default
	GLi->disable( GL_TEXTURE_2D );
	GLi->disableClientState( GL_TEXTURE_COORD_ARRAY );

	/// GL_VERTEX_ARRAY and GL_COLOR_ARRAY are needed, so we keep them enabled
	GLi->enableClientState( GL_VERTEX_ARRAY );
	GLi->enableClientState( GL_COLOR_ARRAY );

	/// Reset the default blend func ( by default eepp use ALPHA_NORMAL )
	BlendMode::setMode( ALPHA_BLENDONE );

	/// Set the line width
	GlobalBatchRenderer::instance()->setLineWidth( 2 );

	if ( ShadersSupported ) {
		/// Rebind the Shader
		shaderProgram->bind();

		/// If you want to use the programmable-pipeline renderer you'll need to set up the projection and modelview matrix manually.
		/// Or if you want to use another name to the projection matrix or the modelview matrix ( eepp programmable-pipeline use
		/// dgl_ProjectionMatrix and dgl_ModelViewMatrix by default.
		if ( GLv_2 == GLi->version() ) {
			shaderProgram->setUniformMatrix( "dgl_ProjectionMatrix", perspectiveMatrix );

			/// Get the identity matrix and set it to the modelview matrix
			float modelMatrix[16];
			GLi->loadIdentity();
			GLi->getCurrentMatrix( GL_MODELVIEW_MATRIX, modelMatrix );

			shaderProgram->setUniformMatrix( "dgl_ModelViewMatrix", modelMatrix );
		}
	}
}

}
using namespace Demo_ExternalShader;

void MainLoop()
{
	win->clear();

	imp->update();

	if ( imp->isKeyDown( KEY_ESCAPE ) )
	{
		win->close();
	}

	if ( imp->isKeyUp( KEY_F ) )
	{
		if ( win->isWindowed() ) {
			win->setSize( win->getDesktopResolution().getWidth(), win->getDesktopResolution().getHeight(), false );
		} else {
			win->setSize( 960, 640, true );
			win->centerToScreen();
		}
	}

	Float p;
	Vector2f mf	= imp->getMousePosf();
	Float tratio	= tw / th;
	Float touchX	= ( mf.x / tw - 1 ) * tratio;
	Float touchY	= -( mf.y / th - 1 );
	bool touch		= imp->isMouseLeftPressed();

	for( Uint32 i = 0; i < ParticlesNum; i+=2 )
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
			Float dx	= touchX - vertices[i].x;
			Float dy	= touchY - vertices[i].y;
			Float distance = dx * dx + dy * dy;

			#if !defined( EE_ARM ) && EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
			Float d	= eesqrt( distance );
			#else
			Float d = sqrt_aprox[ (Int32)(distance * 1000) ];
			#endif

			if ( d < 2.f ) {
				if ( d < 0.03f ) {
					vertices[i+1].x = Math::randf( -1, 1 ) * aspectRatio;
					vertices[i+1].y = Math::randf( -1, 1 );
					velocities[i].x = 0;
					velocities[i].y = 0;
				} else {
					dx /= d;
					dy /= d;
					d = ( 2 - d ) * 0.5;
					d *= d;
					velocities[i].x += dx * d * .01;
					velocities[i].y += dy * d * .01;
				}
			}
		}
	}

	/// VertexPointer assigns values by default to the attribute "dgl_Vertex"
	/// TextureCoordPointer to "dgl_MultiTexCoord0"
	GLi->vertexPointer( 3, GL_FLOAT, sizeof(Vector3ff), reinterpret_cast<char*> ( &vertices[0] ), ParticlesNum * sizeof(float) * 3 );

	/// ColorPointer to "dgl_FrontColor"
	GLi->colorPointer( 4, GL_FP, sizeof(ColorAf), reinterpret_cast<char*> ( &colors[0] ), ParticlesNum * sizeof(Float) * 4 );

	/// Draw the lines
	GLi->drawArrays( DM_LINES, 0, ParticlesNum );

	/// Stop the simulation if the window is not visible
	while ( !win->isVisible() ) {
		imp->update();	/// To get the real state of the window you need to update the window input
		Sys::sleep( 100 ); /// Sleep 100 ms
	}

	win->display();
}

EE_MAIN_FUNC int main (int argc, char * argv [])
{
	win = Engine::instance()->createWindow( WindowSettings( 960, 640, "eepp - External Shaders" ), ContextSettings( true ) );

	if ( win->isOpen() )
	{
		/// This will work without shaders too
		ShadersSupported = GLi->shadersSupported();

		imp = win->getInput();

		/// We really don't need shaders for this, but the purpose of the example is to show how to work with external shaders
		if ( ShadersSupported ) {
			/// Disable the automatic shader conversion from fixed-pipeline to programmable-pipeline
			Shader::ensure( false );

			std::string fs( "#ifdef GL_ES\n\
				precision highp float;\n\
				#endif\n\
				varying	vec4 dgl_Color;\n\
				void main() { gl_FragColor = dgl_Color; }" );

			std::string vs( "#ifdef GL_ES\n\
				precision highp float;\n\
				#endif\n\
				attribute vec3 dgl_Vertex;\n\
				attribute vec4 dgl_FrontColor;\n\
				varying	vec4 dgl_Color;\n\
				uniform mat4 dgl_ProjectionMatrix;\n\
				uniform mat4 dgl_ModelViewMatrix;\n\
				void main()	{\n\
					dgl_Color	= dgl_FrontColor;\n\
					gl_Position = dgl_ProjectionMatrix * dgl_ModelViewMatrix * vec4(dgl_Vertex, 1.0);\n\
				}");

			/// Since fixed-pipeline OpenGL use gl_FrontColor for glColorPointer, we need to replace the color attribute
			/// This is all to show how it works, in a real world scenario, you will choose to work fixed-pipeline or programmable-pipeline.
			if ( GLi->version() == GLv_2 ) {
				String::replaceAll( fs, "gl_FragColor = dgl_Color", "gl_FragColor = gl_FrontColor" );
			}

			/// Create the new shader program
			shaderProgram = ShaderProgram::New( vs.c_str(), vs.size(), fs.c_str(), fs.size() );
		}

		/// Set the projection
		videoResize( win );

		/// Push a window resize callback the reset the projection when needed
		win->pushResizeCallback( cb::Make1( &videoResize ) );

		Uint32 i;

		for (i = 0; i < ParticlesNum; i++ )
		{
			vertices[i]		= Vector3ff( 0, 0, 1.83 );
			velocities[i]	= Vector3ff( (Math::randf() * 2 - 1)*.05, (Math::randf() * 2 - 1)*.05, .93 + Math::randf()*.02 );
			colors[i]		= ColorAf( Math::randf() * 0.5, 0.1, 0.8, 0.5 );
		}

		/** Optimized for ARM ( pre-cache sqrt ) */
		#if defined( EE_ARM ) || EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
		Float tFloat = 0;
		for ( int i = 0; i <= 20000; i++ ) {
			sqrt_aprox[i] = eesqrt( tFloat );
			tFloat += 0.001;
		}
		#endif

		win->runMainLoop( &MainLoop );

		eeSAFE_DELETE_ARRAY( vertices );
		eeSAFE_DELETE_ARRAY( velocities );
		eeSAFE_DELETE_ARRAY( colors );
	}

	Engine::destroySingleton();

	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
