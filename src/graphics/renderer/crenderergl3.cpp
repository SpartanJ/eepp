#include "crenderergl3.hpp"

#ifdef EE_GL3_ENABLED

namespace EE { namespace Graphics {

const GLchar *g_vertexShader[] = {"#version 330\n",
	"uniform					mat4 glm_ProjectionMatrix;\n",	// replaces deprecated gl_ProjectionMatrix
	"uniform					mat4 glm_ModelViewMatrix;\n",	// replaces deprecated gl_ModelViewMatrix
	"layout(location = 0) in	vec2 dgl_Vertex;\n",			// replaces deprecated gl_Vertex
	"layout(location = 2) in	vec4 dgl_Color;\n",				// replaces deprecated gl_Color
	"layout(location = 4) in	vec2 dgl_TexCoord;\n",			// replaces deprecated gl_TexCoord
	"invariant out				vec4 Color;\n",					// to fragment shader
	"invariant out				vec2 TexCoord;\n",				// to fragment shader
	"void main(void)\n",
	"{\n",
	"	Color			= dgl_Color;\n",
	"	TexCoord		= dgl_TexCoord;\n",
	"	vec4 v4			= vec4( dgl_Vertex.x, dgl_Vertex.y, 0.0, 1.0 );\n",
	"	gl_Position		= glm_ProjectionMatrix * glm_ModelViewMatrix * v4;\n",
	"}\n"
};

const GLchar *g_fragmentShader[] = {"#version 330\n",
	"uniform		int			glm_TexActive = 1;\n",
	"invariant in	vec4		Color;\n",
	"invariant in	vec2		TexCoord;\n",
	"out			vec4		dgl_FragColor;\n",
	"uniform		sampler2D	textureUnit0;\n",
	"void main(void)\n",
	"{\n",
	"	if ( 1 == glm_TexActive )\n",
	"		dgl_FragColor = Color * texture2D( textureUnit0, TexCoord );\n",
	"	else\n",
	"		dgl_FragColor = Color;\n"
	"}\n"
};

cRendererGL3::cRendererGL3() :
	glm_ProjectionMatrix_id(0),
	glm_ModelViewMatrix_id(0),
	mCurrentMode(0),
	mCurShader(NULL)
{
	glm_ProjectionMatrix.push	( glm::mat4( 1.0f ) ); // identity matrix
	glm_ModelViewMatrix.push	( glm::mat4( 1.0f ) ); // identity matrix
}

cRendererGL3::~cRendererGL3() {
	for ( Uint32 i = 0; i < eeARRAY_SIZE( mVBO ); i++ ) {
		if ( 0 != mVBO[i] ) {
			glDeleteBuffers( 1, &mVBO[i] );
		}
	}

	glDeleteVertexArrays( 1, &mVAO );
}

EEGL_version cRendererGL3::Version() {
	return GLv_3;
}

GLuint cRendererGL3::BaseShaderId() {
	return mCurShader->Handler();
}

void cRendererGL3::SetShader( cShaderProgram * Shader ) {
	if ( NULL == Shader ) {
		Shader = mShaders[ EEGL_SHADER_BASE_TEX ];
	}

	if ( mCurShader == Shader ) {
		return;
	}

	mCurShader				= Shader;
	glm_ProjectionMatrix_id = mCurShader->UniformLocation( "glm_ProjectionMatrix" );
	glm_ModelViewMatrix_id	= mCurShader->UniformLocation( "glm_ModelViewMatrix" );

	GLenum CM = mCurrentMode;

	MatrixMode( GL_PROJECTION );
	MatrixMode( GL_MODELVIEW );
	MatrixMode( CM );

	glUseProgram( mCurShader->Handler() );
}

void cRendererGL3::Disable ( GLenum cap ) {
	cGL::Disable( cap );

	if ( GL_TEXTURE_2D == cap ) {
		mCurShader->SetUniform( "glm_TexActive", 0 );
	}
}

void cRendererGL3::Enable( GLenum cap ) {
	cGL::Enable( cap );

	if ( GL_TEXTURE_2D == cap ) {
		mCurShader->SetUniform( "glm_TexActive", 1 );
	}
}

void cRendererGL3::Init() {
	cGL::Init();

	//mShaders[ EEGL_SHADER_BASE ]		= eeNew( cShaderProgram, ( (const char**)g_vertexShader_base, sizeof(g_vertexShader_base)/sizeof(const GLchar*), (const char**)g_fragmentShader_base, sizeof(g_fragmentShader_base)/sizeof(const GLchar*), "EEGL_SHADER_BASE" ) );
	mShaders[ EEGL_SHADER_BASE_TEX ]	= eeNew( cShaderProgram, ( (const char**)g_vertexShader, sizeof(g_vertexShader)/sizeof(const GLchar*), (const char**)g_fragmentShader, sizeof(g_fragmentShader)/sizeof(const GLchar*), "EEGL_SHADER_BASE_TEX" ) );
	SetShader( mShaders[ EEGL_SHADER_BASE_TEX ] );

	glGenVertexArrays( 1, &mVAO );
	glBindVertexArray( mVAO );

	memset( mVBO, 0, eeARRAY_SIZE( mVBO ) );

	glGenBuffers( EEGL_ARRAY_STATES_SIZE, &mVBO[0] );

	//"in		 vec2 dgl_Vertex;",
	glBindBuffer(GL_ARRAY_BUFFER, mVBO[ EEGL_VERTEX_ARRAY ] );
	glBufferData(GL_ARRAY_BUFFER, 131072, NULL, GL_DYNAMIC_DRAW );

	//"in		 vec4 dgl_Color;",
	glBindBuffer( GL_ARRAY_BUFFER, mVBO[ EEGL_COLOR_ARRAY ] );
	glBufferData( GL_ARRAY_BUFFER, 131072, NULL, GL_DYNAMIC_DRAW );

	//"in		 vec2 dgl_TexCoord;",
	glBindBuffer( GL_ARRAY_BUFFER, mVBO[ EEGL_TEXTURE_COORD_ARRAY ] );
	glBufferData( GL_ARRAY_BUFFER, 131072, NULL, GL_DYNAMIC_DRAW );

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
			if ( -1 != glm_ProjectionMatrix_id )
				mCurShader->SetUniformMatrix( glm_ProjectionMatrix_id, &glm_ProjectionMatrix.top()[0][0] );
			break;
		}
		case GL_MODELVIEW:
		{
			if ( -1 != glm_ModelViewMatrix_id )
				mCurShader->SetUniformMatrix( glm_ModelViewMatrix_id, &glm_ModelViewMatrix.top()[0][0] );
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

void cRendererGL3::Ortho( GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar ) {
	mCurMatrix->top() *= glm::ortho( left, right, bottom, top , zNear, zFar );
	UpdateMatrix();
}

void cRendererGL3::LookAt( GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ ) {
	mCurMatrix->top() *= glm::lookAt( glm::vec3(eyeX, eyeY, eyeZ), glm::vec3(centerX, centerY, centerZ), glm::vec3(upX, upY, upZ) );
	UpdateMatrix();
}

void cRendererGL3::Perspective ( GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar ) {
	mCurMatrix->top() *= glm::perspective( fovy, aspect, zNear, zFar );
	UpdateMatrix();
}

void cRendererGL3::EnableClientState( GLenum array ) {
	glEnableVertexAttribArray( array - GL_VERTEX_ARRAY );
}

void cRendererGL3::DisableClientState( GLenum array ) {
	glDisableVertexAttribArray( array - GL_VERTEX_ARRAY );
}

void cRendererGL3::VertexPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid * pointer, GLuint allocate ) {
	const GLint index = mCurShader->AttributeLocation( "dgl_Vertex" );

	//eeASSERT( -1 != index );

	if ( -1 != index ) {
		glBindBuffer( GL_ARRAY_BUFFER, mVBO[ EEGL_VERTEX_ARRAY ]			);
		glBufferSubData( GL_ARRAY_BUFFER, 0, allocate, pointer );

		glVertexAttribPointer( index, size, type, GL_FALSE, stride, 0 );
	}
}

void cRendererGL3::ColorPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer, GLuint allocate ) {
	const GLint index = mCurShader->AttributeLocation( "dgl_Color" );

	//eeASSERT( -1 != index );

	if ( -1 != index ) {
		glBindBuffer( GL_ARRAY_BUFFER, mVBO[ EEGL_COLOR_ARRAY ]				);
		glBufferSubData( GL_ARRAY_BUFFER, 0, allocate, pointer );

		if ( type == GL_UNSIGNED_BYTE ) {
			glVertexAttribPointer( index, size, type, GL_TRUE, stride, 0 );
		} else {
			glVertexAttribPointer( index, size, type, GL_FALSE, stride, 0 );
		}
	}
}

void cRendererGL3::TexCoordPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer, GLuint allocate ) {
	const GLint index = mCurShader->AttributeLocation( "dgl_TexCoord" );

	//eeASSERT( -1 != index );

	if ( -1 != index ) {
		glBindBuffer( GL_ARRAY_BUFFER, mVBO[ EEGL_TEXTURE_COORD_ARRAY ]		);
		glBufferSubData( GL_ARRAY_BUFFER, 0, allocate, pointer );

		glVertexAttribPointer( index, size, type, GL_FALSE, stride, 0 );
	}
}

}}

#endif
