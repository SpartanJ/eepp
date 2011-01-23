#include "crenderergl3.hpp"

#ifdef EE_GL3_ENABLED

namespace EE { namespace Graphics {

const char * EEGL_STATES_NAME[] = {
	"dgl_Vertex",
	"dgl_Normal",
	"dgl_Color",
	"dgl_Index",
	"dgl_TexCoord",
	"dgl_EdgeFlag"
};

const GLchar * EEGL_SHADER_BASE_VS[] = {
	"#version 150\n",
	"#extension GL_ARB_explicit_attrib_location : enable\n",
	"uniform					mat4 dgl_ProjectionMatrix;\n",
	"uniform					mat4 dgl_ModelViewMatrix;\n",
	"layout(location = 0) in	vec2 dgl_Vertex;\n",
	"layout(location = 2) in	vec4 dgl_Color;\n",
	"invariant out				vec4 Color;\n",
	"void main(void)\n",
	"{\n",
	"	Color			= dgl_Color;\n",
	"	vec4 v4			= vec4( dgl_Vertex.x, dgl_Vertex.y, 0.0, 1.0 );\n",
	"	gl_Position		= dgl_ProjectionMatrix * dgl_ModelViewMatrix * v4;\n",
	"}\n"
};

const GLchar * EEGL_SHADER_BASE_FS[] = {
	"#version 150\n",
	"invariant in	vec4		Color;\n",
	"out			vec4		dgl_FragColor;\n",
	"void main(void)\n",
	"{\n",
	"	dgl_FragColor = Color;\n",
	"}\n"
};

const GLchar * EEGL_SHADER_BASE_TEX_VS[] = {
	"#version 150\n",
	"#extension GL_ARB_explicit_attrib_location : enable\n",
	"uniform					mat4 dgl_ProjectionMatrix;\n",	// replaces deprecated gl_ProjectionMatrix
	"uniform					mat4 dgl_ModelViewMatrix;\n",	// replaces deprecated gl_ModelViewMatrix
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
	"	gl_Position		= dgl_ProjectionMatrix * dgl_ModelViewMatrix * v4;\n",
	"}\n"
};

const GLchar * EEGL_SHADER_BASE_TEX_FS[] = {
	"#version 150\n",
	"uniform		int			TexActive = 1;\n",
	"invariant in	vec4		Color;\n",
	"invariant in	vec2		TexCoord;\n",
	"out			vec4		dgl_FragColor;\n",
	"uniform		sampler2D	textureUnit0;\n",
	"void main(void)\n",
	"{\n",
	"	if ( 1 == TexActive )\n",
	"		dgl_FragColor = Color * texture2D( textureUnit0, TexCoord );\n",
	"	else\n",
	"		dgl_FragColor = Color;\n",
	"}\n"
};

const GLchar * EEGL_SHADER_POINT_SPRITE_VS[] = {
	"#version 150\n",
	"#extension GL_ARB_explicit_attrib_location : enable\n",
	"uniform					mat4 dgl_ProjectionMatrix;\n",
	"uniform					mat4 dgl_ModelViewMatrix;\n",
	"layout(location = 0) in	vec2 dgl_Vertex;\n",
	"layout(location = 2) in	vec4 dgl_Color;\n",
	"invariant out				vec4 Color;\n",
	"void main(void)\n",
	"{\n",
	"	Color			= dgl_Color;\n",
	"	vec4 v4			= vec4( dgl_Vertex.x, dgl_Vertex.y, 0.0, 1.0 );\n",
	"	gl_Position		= dgl_ProjectionMatrix * dgl_ModelViewMatrix * v4;\n",
	"}\n"
};

const GLchar * EEGL_SHADER_POINT_SPRITE_FS[] = {
	"#version 150\n",
	"invariant in	vec4		Color;\n",
	"out			vec4		dgl_FragColor;\n",
	"uniform		sampler2D	textureUnit0;\n",
	"void main(void)\n",
	"{\n",
	"	dgl_FragColor = Color * texture2D( textureUnit0, gl_PointCoord );\n",
	"}\n"
};

cRendererGL3::cRendererGL3() :
	mProjectionMatrix_id(0),
	mModelViewMatrix_id(0),
	mCurrentMode(0),
	mCurShader(NULL)
{
	mProjectionMatrix.push	( glm::mat4( 1.0f ) ); // identity matrix
	mModelViewMatrix.push	( glm::mat4( 1.0f ) ); // identity matrix
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

void cRendererGL3::SetShader( const EEGL_SHADERS_NUM& Shader ) {
	SetShader( mShaders[ Shader ] );
}

void cRendererGL3::SetShader( cShaderProgram * Shader ) {
	if ( NULL == Shader ) {
		Shader = mShaders[ EEGL_SHADER_BASE_TEX ];
	}

	if ( mCurShader == Shader ) {
		return;
	}

	mCurShader				= Shader;
	mProjectionMatrix_id	= mCurShader->UniformLocation( "dgl_ProjectionMatrix" );
	mModelViewMatrix_id		= mCurShader->UniformLocation( "dgl_ModelViewMatrix" );

	for ( Uint32 i = 0; i < EEGL_ARRAY_STATES_SIZE; i++ ) {
		mStates[ i ] = mCurShader->AttributeLocation( EEGL_STATES_NAME[ i ] );
	}

	glUseProgram( mCurShader->Handler() );

	GLenum CM = mCurrentMode;

	MatrixMode( GL_PROJECTION );
	UpdateMatrix();
	MatrixMode( GL_MODELVIEW );
	UpdateMatrix();
	MatrixMode( CM );
}

void cRendererGL3::Disable ( GLenum cap ) {
	cGL::Disable( cap );

	if ( GL_TEXTURE_2D == cap ) {
		mCurShader->SetUniform( "TexActive", 0 );

		//DisableClientState( GL_TEXTURE_COORD_ARRAY );
		//SetShader( EEGL_SHADER_BASE );
	}
}

void cRendererGL3::Enable( GLenum cap ) {
	cGL::Enable( cap );

	if ( GL_TEXTURE_2D == cap ) {
		mCurShader->SetUniform( "TexActive", 1 );

		//EnableClientState( GL_TEXTURE_COORD_ARRAY );
		//SetShader( EEGL_SHADER_BASE_TEX );
	}
}

void cRendererGL3::Init() {
	cGL::Init();

	mShaders[ EEGL_SHADER_BASE ]			= eeNew( cShaderProgram, ( (const char**)EEGL_SHADER_BASE_VS, sizeof(EEGL_SHADER_BASE_VS)/sizeof(const GLchar*), (const char**)EEGL_SHADER_BASE_FS, sizeof(EEGL_SHADER_BASE_FS)/sizeof(const GLchar*), "EEGL_SHADER_BASE" ) );
	mShaders[ EEGL_SHADER_BASE_TEX ]		= eeNew( cShaderProgram, ( (const char**)EEGL_SHADER_BASE_TEX_VS, sizeof(EEGL_SHADER_BASE_TEX_VS)/sizeof(const GLchar*), (const char**)EEGL_SHADER_BASE_TEX_FS, sizeof(EEGL_SHADER_BASE_TEX_FS)/sizeof(const GLchar*), "EEGL_SHADER_BASE_TEX" ) );
	mShaders[ EEGL_SHADER_POINT_SPRITE ]	= eeNew( cShaderProgram, ( (const char**)EEGL_SHADER_POINT_SPRITE_VS, sizeof(EEGL_SHADER_POINT_SPRITE_VS)/sizeof(const GLchar*), (const char**)EEGL_SHADER_POINT_SPRITE_FS, sizeof(EEGL_SHADER_POINT_SPRITE_FS)/sizeof(const GLchar*), "EEGL_SHADER_POINT_SPRITE" ) );

	//SetShader( mShaders[ EEGL_SHADER_BASE ] );
	//SetShader( mShaders[ EEGL_SHADER_POINT_SPRITE ] );
	SetShader( mShaders[ EEGL_SHADER_BASE_TEX ] );

	glGenVertexArrays( 1, &mVAO );
	glBindVertexArray( mVAO );

	memset( mVBO, 0, eeARRAY_SIZE( mVBO ) );

	glGenBuffers( EEGL_ARRAY_STATES_SIZE, &mVBO[0] );

	//"in		 vec2 dgl_Vertex;",
	glBindBuffer(GL_ARRAY_BUFFER, mVBO[ EEGL_VERTEX_ARRAY ] );
	glBufferData(GL_ARRAY_BUFFER, 131072, NULL, GL_STREAM_DRAW );

	//"in		 vec4 dgl_Color;",
	glBindBuffer( GL_ARRAY_BUFFER, mVBO[ EEGL_COLOR_ARRAY ] );
	glBufferData( GL_ARRAY_BUFFER, 131072, NULL, GL_STREAM_DRAW );

	//"in		 vec2 dgl_TexCoord;",
	glBindBuffer( GL_ARRAY_BUFFER, mVBO[ EEGL_TEXTURE_COORD_ARRAY ] );
	glBufferData( GL_ARRAY_BUFFER, 131072, NULL, GL_STREAM_DRAW );
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
			if ( -1 != mProjectionMatrix_id )
				mCurShader->SetUniformMatrix( mProjectionMatrix_id, &mProjectionMatrix.top()[0][0] );
			break;
		}
		case GL_MODELVIEW:
		{
			if ( -1 != mModelViewMatrix_id )
				mCurShader->SetUniformMatrix( mModelViewMatrix_id, &mModelViewMatrix.top()[0][0] );
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
			mCurMatrix = &mProjectionMatrix;
			break;
		}
		case GL_MODELVIEW:
		{
			mCurMatrix = &mModelViewMatrix;
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
	const GLint index = mStates[ EEGL_VERTEX_ARRAY ];

	if ( -1 != index ) {
		glBindVertexArray( mVAO );
		glBindBuffer( GL_ARRAY_BUFFER, mVBO[ EEGL_VERTEX_ARRAY ]			);
		//glBufferData( GL_ARRAY_BUFFER, allocate, pointer, GL_STREAM_DRAW );
		glBufferSubData( GL_ARRAY_BUFFER, 0, allocate, pointer );

		glVertexAttribPointer( index, size, type, GL_FALSE, stride, 0 );
	}
}

void cRendererGL3::ColorPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer, GLuint allocate ) {
	const GLint index = mStates[ EEGL_COLOR_ARRAY ];

	if ( -1 != index ) {
		glBindVertexArray( mVAO );
		glBindBuffer( GL_ARRAY_BUFFER, mVBO[ EEGL_COLOR_ARRAY ]				);
		//glBufferData( GL_ARRAY_BUFFER, allocate, pointer, GL_STREAM_DRAW );
		glBufferSubData( GL_ARRAY_BUFFER, 0, allocate, pointer );

		if ( type == GL_UNSIGNED_BYTE ) {
			glVertexAttribPointer( index, size, type, GL_TRUE, stride, 0 );
		} else {
			glVertexAttribPointer( index, size, type, GL_FALSE, stride, 0 );
		}
	}
}

void cRendererGL3::TexCoordPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer, GLuint allocate ) {
	const GLint index = mStates[ EEGL_TEXTURE_COORD_ARRAY ];

	if ( -1 != index ) {
		glBindVertexArray( mVAO );
		glBindBuffer( GL_ARRAY_BUFFER, mVBO[ EEGL_TEXTURE_COORD_ARRAY ]		);
		//glBufferData( GL_ARRAY_BUFFER, allocate, pointer, GL_STREAM_DRAW );
		glBufferSubData( GL_ARRAY_BUFFER, 0, allocate, pointer );

		glVertexAttribPointer( index, size, type, GL_FALSE, stride, 0 );
	}
}

GLint cRendererGL3::GetStateIndex( const Uint32& State ) {
	eeASSERT( State < EEGL_ARRAY_STATES_SIZE );
	return mStates[ State ];
}

}}

#endif
