#include "crenderergl3.hpp"

#ifdef EE_GL3_ENABLED

namespace EE { namespace Graphics {

const GLchar *g_vertexShader[] = {"#version 130\n",
	"uniform mat4 glm_ProjectionMatrix;\n",	// replaces deprecated gl_ProjectionMatrix see http://www.lighthouse3d.com/opengl/glsl/index.php?minimal
	"uniform mat4 glm_ModelViewMatrix;\n",	// replaces deprecated gl_ModelViewMatrix
	"in		 vec4 dgl_Vertex;\n",			// replaces deprecated gl_Vertex
	"in		 vec4 dgl_Color;\n",			// replaces deprecated gl_Color
	"invariant out	vec4 Color;\n",			// to fragment shader
	"void main(void)\n",
	"{\n",
	"	Color = dgl_Color;\n",
	"	gl_Position = glm_ProjectionMatrix*glm_ModelViewMatrix*dgl_Vertex;\n", // replaces deprecated ftransform() see http://www.songho.ca/opengl/gl_transform.html
	" gl_TexCoord[0] = gl_MultiTexCoord0;\n",
	"}\n"
};

const GLchar *g_fragmentShader[] = {"#version 130\n",
	"invariant in vec4 Color;\n", // from vertex shader
	"out  vec4 dgl_FragColor;\n", // replaces deprecated gl_FragColor
	"void main(void)\n",
	"{\n",
	"  dgl_FragColor = Color;\n", // gl_FragColor is deprecated
	"}\n"
};

cRendererGL3::cRendererGL3() :
	shader_id(0),
	glm_ProjectionMatrix_id(0),
	glm_ModelViewMatrix_id(0),
	mCurrentMode(0)
{
	glm_ProjectionMatrix.push	( glm::mat4( 1.0f ) ); // identity matrix
	glm_ModelViewMatrix.push	( glm::mat4( 1.0f ) ); // identity matrix
}

cRendererGL3::~cRendererGL3() {
}

EEGL_version cRendererGL3::Version() {
	return GLv_3;
}

void cRendererGL3::Init() {
	cGL::Init();

	// compile Vertex shader
	GLuint m_vxShaderId			= glCreateShader(GL_VERTEX_SHADER);
	GLsizei nlines_vx			= sizeof(g_vertexShader)/sizeof(const GLchar*);
	glShaderSource( m_vxShaderId, nlines_vx, (const GLchar**)g_vertexShader, NULL );
	glCompileShader( m_vxShaderId );

	// compile Fragment shader
	GLuint m_fgShaderId			= glCreateShader(GL_FRAGMENT_SHADER);
	GLsizei nlines_fg			= sizeof(g_fragmentShader)/sizeof(const GLchar*);
	glShaderSource( m_fgShaderId, nlines_fg, (const GLchar**)g_fragmentShader, NULL );
	glCompileShader( m_fgShaderId );

	// link shaders
	shader_id					= glCreateProgram();
	glAttachShader( shader_id, m_vxShaderId );
	glAttachShader( shader_id, m_fgShaderId );
	glLinkProgram( shader_id );

	//hooks from CPU to GPU
	//define Uniform hooks
	glm_ProjectionMatrix_id		= glGetUniformLocation(shader_id, "glm_ProjectionMatrix");
	glm_ModelViewMatrix_id		= glGetUniformLocation(shader_id, "glm_ModelViewMatrix");

	// finally, use the shader for rendering
	glUseProgram(shader_id);            // select the shaders program
}

void cRendererGL3::PushMatrix() {
	mCurMatrix->push( mCurMatrix->top() );
	UpdateMatrix();
}

void cRendererGL3::PopMatrix() {
	mCurMatrix->pop();
	UpdateMatrix();
}

void cRendererGL3::UpdateMatrix() {
	switch ( mCurrentMode ) {
		case GL_PROJECTION:
		{
			glUniformMatrix4fv( glm_ProjectionMatrix_id, 1, false, &glm_ProjectionMatrix.top()[0][0] );
			break;
		}
		case GL_MODELVIEW:
		{
			glUniformMatrix4fv( glm_ModelViewMatrix_id, 1, false, &glm_ModelViewMatrix.top()[0][0] );
			break;
		}
	}
}

void cRendererGL3::LoadIdentity() {
	mCurMatrix->top() = glm::mat4(1.0);
	UpdateMatrix();
}

void cRendererGL3::Translatef( GLfloat x, GLfloat y, GLfloat z ) {
	mCurMatrix->top() *= glm::translate( glm::vec3( x, y, z ) );
	UpdateMatrix();
}

void cRendererGL3::Rotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z ) {
	mCurMatrix->top() *= glm::rotate( angle, glm::vec3( x, y, z ) );
	UpdateMatrix();
}

void cRendererGL3::Scalef( GLfloat x, GLfloat y, GLfloat z ) {
	mCurMatrix->top() *= glm::scale( glm::vec3( x, y, z ) );
	UpdateMatrix();
}

void cRendererGL3::MatrixMode(GLenum mode) {
	mCurrentMode = mode;

	switch ( mCurrentMode ) {
		case GL_PROJECTION:
		{
			mCurMatrix = &glm_ProjectionMatrix;
			break;
		}
		case GL_MODELVIEW:
		{
			mCurMatrix = &glm_ModelViewMatrix;
			break;
		}
	}
}

void cRendererGL3::Ortho( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) {
	mCurMatrix->top() = glm::ortho( left, right, bottom, top , zNear, zFar );
}

}}

#endif
