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
	"	vec4 v4			= vec4( dgl_Vertex, 0.0, 1.0 );\n",
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
	"	vec4 v4			= vec4( dgl_Vertex, 0.0, 1.0 );\n",
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
	"	vec4 v4			= vec4( dgl_Vertex, 0.0, 1.0 );\n",
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

const GLchar * EEGL_SHADER_CLIP_VS[] = {
	"#version 150\n",
	"#extension GL_ARB_explicit_attrib_location : enable\n",
	"uniform					mat4 dgl_ProjectionMatrix;\n",
	"uniform					mat4 dgl_ModelViewMatrix;\n",
	"uniform					vec4 dgl_ClipPlane[ 4 ];\n",
	"layout(location = 0) in	vec2 dgl_Vertex;\n",
	"layout(location = 2) in	vec4 dgl_Color;\n",
	"layout(location = 4) in	vec2 dgl_TexCoord;\n",
	"invariant out				vec4 Color;\n",
	"invariant out				vec2 TexCoord;\n",
	"void main(void)\n",
	"{\n",
	"	Color			= dgl_Color;\n",
	"	TexCoord		= dgl_TexCoord;\n",
	"	vec4 v4			= vec4( dgl_Vertex, 0.0, 1.0 );\n",
	"	gl_Position		= dgl_ProjectionMatrix * dgl_ModelViewMatrix * v4;\n",
	"	gl_ClipDistance[0] = dot( v4, dgl_ClipPlane[0] * dgl_ModelViewMatrix );",
	"	gl_ClipDistance[1] = dot( v4, dgl_ClipPlane[1] * dgl_ModelViewMatrix );",
	"	gl_ClipDistance[2] = dot( v4, dgl_ClipPlane[2] * dgl_ModelViewMatrix );",
	"	gl_ClipDistance[3] = dot( v4, dgl_ClipPlane[3] * dgl_ModelViewMatrix );",
	"}\n"
};

const GLchar * EEGL_SHADER_CLIP_FS[] = {
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

cRendererGL3::cRendererGL3() :
	mProjectionMatrix_id(0),
	mModelViewMatrix_id(0),
	mCurrentMode(0),
	mCurShader(NULL),
	mShaderPrev(NULL),
	mTexActive(1)
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

void cRendererGL3::SetShader( const EEGL_SHADERS& Shader ) {
	SetShader( mShaders[ Shader ] );
}

void cRendererGL3::SetShader( cShaderProgram * Shader ) {
	if ( NULL == Shader ) {
		Shader = mShaders[ EEGL_SHADER_BASE_TEX ];
	}

	if ( mCurShader == Shader ) {
		return;
	}

	if ( mShaders[ EEGL_SHADER_CLIP ] != mCurShader ) {
		mShaderPrev				= mCurShader;
	}

	mCurShader				= Shader;
	mProjectionMatrix_id	= mCurShader->UniformLocation( "dgl_ProjectionMatrix" );
	mModelViewMatrix_id		= mCurShader->UniformLocation( "dgl_ModelViewMatrix" );

	for ( Uint32 i = 0; i < EEGL_ARRAY_STATES_COUNT; i++ ) {
		mStates[ i ] = mCurShader->AttributeLocation( EEGL_STATES_NAME[ i ] );
	}

	mTexActiveLoc = mCurShader->UniformLocation( "TexActive" );

	glUseProgram( mCurShader->Handler() );

	GLenum CM = mCurrentMode;

	MatrixMode( GL_PROJECTION );
	UpdateMatrix();
	MatrixMode( GL_MODELVIEW );
	UpdateMatrix();
	MatrixMode( CM );
}

void cRendererGL3::Disable ( GLenum cap ) {
	if ( GL_TEXTURE_2D == cap ) {
		if ( glIsEnabled( cap ) ) {
			mTexActive = 0;
			mCurShader->SetUniform( mTexActiveLoc, mTexActive );

			/// Reset the vertex attrib to zero to avoid crashes on draw calls
			TexCoordPointer( 2, GL_FLOAT, 0, (const GLvoid*)NULL, 0 );
		}

		/// Disabled shader changing because it seems to be slower than just passing a state flag
		/**
		DisableClientState( GL_TEXTURE_COORD_ARRAY );
		SetShader( EEGL_SHADER_BASE );
		*/
	}

	cGL::Disable( cap );
}

void cRendererGL3::Enable( GLenum cap ) {
	if ( GL_TEXTURE_2D == cap ) {
		if ( !glIsEnabled( cap ) ) {
			mTexActive = 1;
			mCurShader->SetUniform( mTexActiveLoc, mTexActive );
		}

		/**
		TexCoordPointer( 2, GL_FLOAT, 0, (const GLvoid*)NULL, 0 );
		EnableClientState( GL_TEXTURE_COORD_ARRAY );
		SetShader( EEGL_SHADER_BASE_TEX );
		*/
	}

	cGL::Enable( cap );
}

void cRendererGL3::Init() {
	cGL::Init();

	mShaders[ EEGL_SHADER_BASE ]			= eeNew( cShaderProgram, ( (const char**)EEGL_SHADER_BASE_VS, sizeof(EEGL_SHADER_BASE_VS)/sizeof(const GLchar*), (const char**)EEGL_SHADER_BASE_FS, sizeof(EEGL_SHADER_BASE_FS)/sizeof(const GLchar*), "EEGL_SHADER_BASE" ) );
	mShaders[ EEGL_SHADER_BASE_TEX ]		= eeNew( cShaderProgram, ( (const char**)EEGL_SHADER_BASE_TEX_VS, sizeof(EEGL_SHADER_BASE_TEX_VS)/sizeof(const GLchar*), (const char**)EEGL_SHADER_BASE_TEX_FS, sizeof(EEGL_SHADER_BASE_TEX_FS)/sizeof(const GLchar*), "EEGL_SHADER_BASE_TEX" ) );
	mShaders[ EEGL_SHADER_POINT_SPRITE ]	= eeNew( cShaderProgram, ( (const char**)EEGL_SHADER_POINT_SPRITE_VS, sizeof(EEGL_SHADER_POINT_SPRITE_VS)/sizeof(const GLchar*), (const char**)EEGL_SHADER_POINT_SPRITE_FS, sizeof(EEGL_SHADER_POINT_SPRITE_FS)/sizeof(const GLchar*), "EEGL_SHADER_POINT_SPRITE" ) );
	mShaders[ EEGL_SHADER_CLIP ]			= eeNew( cShaderProgram, ( (const char**)EEGL_SHADER_CLIP_VS, sizeof(EEGL_SHADER_CLIP_VS)/sizeof(const GLchar*), (const char**)EEGL_SHADER_CLIP_FS, sizeof(EEGL_SHADER_CLIP_FS)/sizeof(const GLchar*), "EEGL_SHADER_CLIP" ) );

	SetShader( mShaders[ EEGL_SHADER_BASE_TEX ] );

	glGenVertexArrays( 1, &mVAO );
	glBindVertexArray( mVAO );

	memset( mVBO, 0, eeARRAY_SIZE( mVBO ) );

	glGenBuffers( EEGL_ARRAY_STATES_COUNT, &mVBO[0] );

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
		glBufferSubData( GL_ARRAY_BUFFER, 0, allocate, pointer );

		glVertexAttribPointer( index, size, type, GL_FALSE, stride, 0 );
	}
}

void cRendererGL3::ColorPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer, GLuint allocate ) {
	const GLint index = mStates[ EEGL_COLOR_ARRAY ];

	if ( -1 != index ) {
		glBindVertexArray( mVAO );
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
	const GLint index = mStates[ EEGL_TEXTURE_COORD_ARRAY ];

	if ( -1 != index ) {
		glBindVertexArray( mVAO );
		glBindBuffer( GL_ARRAY_BUFFER, mVBO[ EEGL_TEXTURE_COORD_ARRAY ]		);
		glBufferSubData( GL_ARRAY_BUFFER, 0, allocate, pointer );

		glVertexAttribPointer( index, size, type, GL_FALSE, stride, 0 );
	}
}

GLint cRendererGL3::GetStateIndex( const Uint32& State ) {
	eeASSERT( State < EEGL_ARRAY_STATES_COUNT );
	return mStates[ State ];
}

void cRendererGL3::ClipPlaneEnable( const Int32& x, const Int32& y, const Int32& Width, const Int32& Height ) {
	GLfloat tX = (GLfloat)x;
	GLfloat tY = (GLfloat)y;
	GLfloat tW = (GLfloat)Width;
	GLfloat tH = (GLfloat)Height;

	GLfloat clip_left[] 	= { 1.0		, 0.0	, 0.0	, -tX 		};
	GLfloat clip_right[] 	= { -1.0	, 0.0	, 0.0	, tX + tW 	};
	GLfloat clip_top[]		= { 0.0		, 1.0	, 0.0	, -tY 		};
	GLfloat clip_bottom[] 	= { 0.0		, -1.0	, 0.0	, tY + tH 	};
	
	#ifndef EE_GLES2
	GLi->Enable(GL_CLIP_PLANE0);
	GLi->Enable(GL_CLIP_PLANE1);
	GLi->Enable(GL_CLIP_PLANE2);
	GLi->Enable(GL_CLIP_PLANE3);
	#endif
	
	SetShader( EEGL_SHADER_CLIP );

	mCurShader->SetUniform( "TexActive", mTexActive );

	glUniform4fv( glGetUniformLocation( mCurShader->Handler(), "dgl_ClipPlane[0]" ), 1, clip_left );
	glUniform4fv( glGetUniformLocation( mCurShader->Handler(), "dgl_ClipPlane[1]" ), 1, clip_right );
	glUniform4fv( glGetUniformLocation( mCurShader->Handler(), "dgl_ClipPlane[2]" ), 1, clip_top );
	glUniform4fv( glGetUniformLocation( mCurShader->Handler(), "dgl_ClipPlane[3]" ), 1, clip_bottom );
}

void cRendererGL3::ClipPlaneDisable() {
	#ifndef EE_GLES2
	GLi->Disable(GL_CLIP_PLANE0);
	GLi->Disable(GL_CLIP_PLANE1);
	GLi->Disable(GL_CLIP_PLANE2);
	GLi->Disable(GL_CLIP_PLANE3);
	#endif
	
	SetShader( mShaderPrev );
}

void cRendererGL3::ClientActiveTexture( GLenum texture ) {
	//! TODO: Implement multitexturing in GL3 renderer
}

}}

#endif
