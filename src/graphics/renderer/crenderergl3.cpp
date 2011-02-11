#include "crenderergl3.hpp"

#ifdef EE_GL3_ENABLED

namespace EE { namespace Graphics {

const char * EEGL_STATES_NAME[] = {
	"dgl_Vertex",
	"dgl_Normal",
	"dgl_Color",
	"dgl_Index",
	"dgl_MultiTexCoord0",
	"dgl_EdgeFlag"
};

const char * EEGL_PLANES_ENABLED_NAME[] = {
	"dgl_ClipEnabled[0]",
	"dgl_ClipEnabled[1]",
	"dgl_ClipEnabled[2]",
	"dgl_ClipEnabled[3]",
	"dgl_ClipEnabled[4]",
	"dgl_ClipEnabled[5]"
};

const char * EEGL_PLANES_NAME[] = {
	"dgl_ClipPlane[0]",
	"dgl_ClipPlane[1]",
	"dgl_ClipPlane[2]",
	"dgl_ClipPlane[3]",
	"dgl_ClipPlane[4]",
	"dgl_ClipPlane[5]"
};

const GLchar * EEGL_SHADER_BASE_VS[] = {
	"#version 150\n",
	"#define MAX_CLIP_PLANES 6\n",
	"uniform			mat4 dgl_ProjectionMatrix;\n",	// replaces deprecated gl_ProjectionMatrix
	"uniform			mat4 dgl_ModelViewMatrix;\n",	// replaces deprecated gl_ModelViewMatrix
	"uniform			int  dgl_ClippingEnabled = 0;\n",
	"uniform			int	 dgl_ClipEnabled[ MAX_CLIP_PLANES ] = { 0, 0, 0, 0, 0, 0 };\n",
	"uniform			vec4 dgl_ClipPlane[ MAX_CLIP_PLANES ];\n",
	#ifdef EE_GLES2
	"uniform			float dgl_PointSize = 1;\n",
	#endif
	"in					vec4 dgl_Vertex;\n",			// replaces deprecated gl_Vertex
	"in					vec4 dgl_Color;\n",				// replaces deprecated gl_Color
	"in					vec4 dgl_MultiTexCoord0;\n",	// replaces deprecated gl_MultiTexCoord0
	"invariant out		vec4 dgl_VertexColor;\n",		// to fragment shader
	"invariant out		vec4 dgl_TexCoord0;\n",			// to fragment shader
	"void main(void)\n",
	"{\n",
	#ifdef EE_GLES2
	"	gl_PointSize	= dgl_PointSize;\n",
	#endif
	"	dgl_VertexColor	= dgl_Color;\n",
	"	dgl_TexCoord0	= dgl_MultiTexCoord0;\n",
	"	vec4 vEye		= dgl_ModelViewMatrix * dgl_Vertex;\n",
	"	gl_Position		= dgl_ProjectionMatrix * vEye;\n",
	"	if ( 1 == dgl_ClippingEnabled ) {\n",
	"		for ( int i = 0; i < MAX_CLIP_PLANES; i++ ) {\n",
	"			if ( 1 == dgl_ClipEnabled[i] )\n",
	"				gl_ClipDistance[i] = dot( vEye, dgl_ClipPlane[i] );\n",
	"		}\n",
	"	}\n",
	"}\n"
};

const GLchar * EEGL_SHADER_BASE_FS[] = {
	"#version 150\n",
	"uniform		sampler2D	textureUnit0;\n",
	"uniform		int			dgl_TexActive = 1;\n",
	"uniform		int			dgl_PointSpriteActive = 0;\n",
	"invariant in	vec4		dgl_VertexColor;\n",
	"invariant in	vec4		dgl_TexCoord0;\n",
	"smooth out		vec4		dgl_FragColor;\n",
	"void main(void)\n",
	"{\n",
	"	if ( 0 == dgl_PointSpriteActive ) {\n",
	"		if ( 1 == dgl_TexActive )\n",
	"			dgl_FragColor = dgl_VertexColor * texture2D( textureUnit0, dgl_TexCoord0.xy );\n",
	"		else\n",
	"			dgl_FragColor = dgl_VertexColor;\n",
	"	} else\n",
	"		dgl_FragColor = dgl_VertexColor * texture2D( textureUnit0, gl_PointCoord );\n",
	"}\n"
};

cRendererGL3::cRendererGL3() :
	mProjectionMatrix_id(0),
	mModelViewMatrix_id(0),
	mCurrentMode(0),
	mCurShader(NULL),
	mShaderPrev(NULL),
	mTexActive(1),
	mTexActiveLoc(-1),
	mPointSpriteLoc(-1),
	mPointSize(1.f),
	mLoaded( false )
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

	#ifndef EE_GLES2
	glDeleteVertexArrays( 1, &mVAO );
	#endif
}

EEGL_version cRendererGL3::Version() {
	return GLv_3;
}

GLuint cRendererGL3::BaseShaderId() {
	return mCurShader->Handler();
}

void cRendererGL3::ReloadShader( cShaderProgram * Shader ) {
	mCurShader = NULL;

	SetShader( Shader );
}

void cRendererGL3::SetShader( const EEGL_SHADERS& Shader ) {
	SetShader( mShaders[ Shader ] );
}

void cRendererGL3::SetShader( cShaderProgram * Shader ) {
	if ( NULL == Shader ) {
		Shader = mShaders[ EEGL_SHADER_BASE ];
	}

	if ( mCurShader == Shader ) {
		return;
	}

	mShaderPrev				= mCurShader;
	mCurShader				= Shader;
	mProjectionMatrix_id	= mCurShader->UniformLocation( "dgl_ProjectionMatrix" );
	mModelViewMatrix_id		= mCurShader->UniformLocation( "dgl_ModelViewMatrix" );
	mTexActiveLoc			= mCurShader->UniformLocation( "dgl_TexActive" );
	mPointSpriteLoc			= mCurShader->UniformLocation( "dgl_PointSpriteActive" );

	Uint32 i;

	for ( i = 0; i < EEGL_ARRAY_STATES_COUNT; i++ ) {
		mStates[ i ] = mCurShader->AttributeLocation( EEGL_STATES_NAME[ i ] );
	}

	for ( i = 0; i < EE_MAX_PLANES; i++ ) {
		mPlanes[ i ] = mCurShader->UniformLocation( EEGL_PLANES_NAME[ i ] );
	}

	glUseProgram( mCurShader->Handler() );

	GLenum CM = mCurrentMode;

	MatrixMode( GL_PROJECTION );
	UpdateMatrix();
	MatrixMode( GL_MODELVIEW );
	UpdateMatrix();
	MatrixMode( CM );
}

void cRendererGL3::Enable( GLenum cap ) {
	switch ( cap ) {
		case GL_TEXTURE_2D:
		{
			if ( !glIsEnabled( cap ) ) {
				mTexActive = 1;
				mCurShader->SetUniform( mTexActiveLoc, mTexActive );
			}

			break;
		}
		case GL_CLIP_PLANE0:
		case GL_CLIP_PLANE1:
		case GL_CLIP_PLANE2:
		case GL_CLIP_PLANE3:
		case GL_CLIP_PLANE4:
		case GL_CLIP_PLANE5:
		{
			GLint plane = cap - GL_CLIP_PLANE0;

			mPlanesStates[ plane ] = 1;
			PlaneStateCheck( true );
			mCurShader->SetUniform( EEGL_PLANES_ENABLED_NAME[ plane ], 1 );

			break;
		}
		case GL_POINT_SPRITE:
		{
			mCurShader->SetUniform( mPointSpriteLoc, 1 );

			cGL::Enable( GL_VERTEX_PROGRAM_POINT_SIZE );

			break;
		}
	}

	cGL::Enable( cap );
}

void cRendererGL3::Disable ( GLenum cap ) {
	switch ( cap ) {
		case GL_TEXTURE_2D:
		{
			if ( glIsEnabled( cap ) ) {
				mTexActive = 0;
				mCurShader->SetUniform( mTexActiveLoc, mTexActive );
			}

			break;
		}
		case GL_CLIP_PLANE0:
		case GL_CLIP_PLANE1:
		case GL_CLIP_PLANE2:
		case GL_CLIP_PLANE3:
		case GL_CLIP_PLANE4:
		case GL_CLIP_PLANE5:
		{
			GLint plane = cap - GL_CLIP_PLANE0;

			mPlanesStates[ plane ] = 0;
			PlaneStateCheck( false );
			mCurShader->SetUniform( EEGL_PLANES_ENABLED_NAME[ plane ], 0 );

			break;
		}
		case GL_POINT_SPRITE:
		{
			mCurShader->SetUniform( mPointSpriteLoc, 0 );

			cGL::Disable( GL_VERTEX_PROGRAM_POINT_SIZE );

			break;
		}
	}

	cGL::Disable( cap );
}

void cRendererGL3::PlaneStateCheck( bool tryEnable ) {
	GLint i;

	if (  tryEnable  ) {
		for ( i = 0; i < EE_MAX_PLANES; i++ ) {
			if ( 0 != mPlanesStates[ i ] ) {
				mCurShader->SetUniform( "dgl_ClippingEnabled", 1 );
				return;
			}
		}
	} else {
		for ( i = 0; i < EE_MAX_PLANES; i++) {
			if ( 0 != mPlanesStates[ i ] ) {
				return;
			}
		}

		mCurShader->SetUniform( "dgl_ClippingEnabled", 0 );
	}
}

void cRendererGL3::Init() {
	if ( !mLoaded ) {
		cGL::Init();

		Uint32 i;

		for ( i = 0; i < EEGL_ARRAY_STATES_COUNT; i++ ) {
			mVBO[i] = 0;
		}

		for ( i = 0; i < EE_MAX_PLANES; i++ ) {
			mPlanes[i]			= -1;
			mPlanesStates[i]	= 0;
		}

		mShaders[ EEGL_SHADER_BASE ]			= eeNew( cShaderProgram, ( (const char**)EEGL_SHADER_BASE_VS, sizeof(EEGL_SHADER_BASE_VS)/sizeof(const GLchar*), (const char**)EEGL_SHADER_BASE_FS, sizeof(EEGL_SHADER_BASE_FS)/sizeof(const GLchar*), "EEGL_SHADER_BASE_TEX" ) );
		mShaders[ EEGL_SHADER_BASE ]->SetReloadCb( cb::Make1( this, &cRendererGL3::ReloadShader ) );

		SetShader( mShaders[ EEGL_SHADER_BASE ] );
	} else {
		mCurShader = NULL;

		mShaders[ EEGL_SHADER_BASE ]->Reload();

		SetShader( mShaders[ EEGL_SHADER_BASE ] );
	}

	#ifndef EE_GLES2
	glGenVertexArrays( 1, &mVAO );
	glBindVertexArray( mVAO );
	#endif

	glGenBuffers( EEGL_ARRAY_STATES_COUNT, &mVBO[0] );

	//"in		 vec2 dgl_Vertex;",
	glBindBuffer( GL_ARRAY_BUFFER, mVBO[ EEGL_VERTEX_ARRAY ] );
	glBufferData( GL_ARRAY_BUFFER, 131072, NULL, GL_STREAM_DRAW );

	//"in		 vec4 dgl_Color;",
	glBindBuffer( GL_ARRAY_BUFFER, mVBO[ EEGL_COLOR_ARRAY ] );
	glBufferData( GL_ARRAY_BUFFER, 131072, NULL, GL_STREAM_DRAW );

	//"in		 vec2 dgl_TexCoord;",
	glBindBuffer( GL_ARRAY_BUFFER, mVBO[ EEGL_TEXTURE_COORD_ARRAY ] );
	glBufferData( GL_ARRAY_BUFFER, 131072, NULL, GL_STREAM_DRAW );

	mLoaded = true;
}

void cRendererGL3::UpdateMatrix() {
	switch ( mCurrentMode ) {
		case GL_PROJECTION:
		{
			if ( -1 != mProjectionMatrix_id ) {
				mCurShader->SetUniformMatrix( mProjectionMatrix_id, &mProjectionMatrix.top()[0][0] );
			}

			break;
		}
		case GL_MODELVIEW:
		{
			if ( -1 != mModelViewMatrix_id ) {
				mCurShader->SetUniformMatrix( mModelViewMatrix_id, &mModelViewMatrix.top()[0][0] );
			}

			break;
		}
	}
}

void cRendererGL3::PushMatrix() {
	mCurMatrix->push( mCurMatrix->top() );
	UpdateMatrix();
}

void cRendererGL3::PopMatrix() {
	mCurMatrix->pop();
	UpdateMatrix();
}

void cRendererGL3::LoadIdentity() {
	mCurMatrix->top() = glm::mat4(1.0);
	UpdateMatrix();
}

glm::mat4 cRendererGL3::toGLMmat4( const GLfloat * m ) {
	return glm::mat4( m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8], m[9], m[10], m[11], m[12], m[13], m[14], m[15] );
}

void cRendererGL3::MultMatrixf ( const GLfloat * m ) {
	mCurMatrix->top() *= toGLMmat4( m );
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

void cRendererGL3::LoadMatrixf( const GLfloat * m ) {
	mCurMatrix->top() = toGLMmat4( m );
	UpdateMatrix();
}

void cRendererGL3::Frustum( GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val ) {
	mCurMatrix->top() *= glm::frustum( left, right, bottom, top, near_val, far_val );
	UpdateMatrix();
}

void cRendererGL3::fromGLMmat4( glm::mat4 from, GLfloat * to ) {
	Int32 i,p;

	for ( i = 0; i < 4; i++ ) {
		glm::vec4 v = from[i];
		p		= i * 4;
		to[p  ]	= v.x;
		to[p+1]	= v.y;
		to[p+2]	= v.z;
		to[p+3]	= v.w;
	}
}

void cRendererGL3::GetCurrentMatrix( GLenum mode, GLfloat * m ) {
	switch ( mode ) {
		case GL_PROJECTION:
		{
			fromGLMmat4( mProjectionMatrix.top(), m );
			break;
		}
		case GL_MODELVIEW:
		{
			fromGLMmat4( mModelViewMatrix.top(), m );
			break;
		}
	}
}

GLenum cRendererGL3::GetCurrentMatrixMode() {
	return mCurrentMode;
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

void cRendererGL3::EnableClientState( GLenum array ) {
	glEnableVertexAttribArray( mStates[ array - GL_VERTEX_ARRAY ] );
}

void cRendererGL3::DisableClientState( GLenum array ) {
	glDisableVertexAttribArray( mStates[ array - GL_VERTEX_ARRAY ] );
}

void cRendererGL3::VertexPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid * pointer, GLuint allocate ) {
	const GLint index = mStates[ EEGL_VERTEX_ARRAY ];

	if ( -1 != index ) {
		#ifndef EE_GLES2
		glBindVertexArray( mVAO );
		#endif

		glBindBuffer( GL_ARRAY_BUFFER, mVBO[ EEGL_VERTEX_ARRAY ]			);
		glBufferSubData( GL_ARRAY_BUFFER, 0, allocate, pointer );

		#ifdef EE_GLES2
		glEnableVertexAttribArray( index );
		#endif

		glVertexAttribPointer( index, size, type, GL_FALSE, stride, 0 );
	}
}

void cRendererGL3::ColorPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer, GLuint allocate ) {
	const GLint index = mStates[ EEGL_COLOR_ARRAY ];

	if ( -1 != index ) {
		#ifndef EE_GLES2
		glBindVertexArray( mVAO );
		#endif

		glBindBuffer( GL_ARRAY_BUFFER, mVBO[ EEGL_COLOR_ARRAY ]				);
		glBufferSubData( GL_ARRAY_BUFFER, 0, allocate, pointer );

		#ifdef EE_GLES2
		glEnableVertexAttribArray( index );
		#endif

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
		#ifndef EE_GLES2
		glBindVertexArray( mVAO );
		#endif

		glBindBuffer( GL_ARRAY_BUFFER, mVBO[ EEGL_TEXTURE_COORD_ARRAY ]		);
		glBufferSubData( GL_ARRAY_BUFFER, 0, allocate, pointer );

		#ifdef EE_GLES2
		glEnableVertexAttribArray( index );
		#endif

		glVertexAttribPointer( index, size, type, GL_FALSE, stride, 0 );
	}
}

GLint cRendererGL3::GetStateIndex( const Uint32& State ) {
	eeASSERT( State < EEGL_ARRAY_STATES_COUNT );
	return mStates[ State ];
}

void cRendererGL3::Clip2DPlaneEnable( const Int32& x, const Int32& y, const Int32& Width, const Int32& Height ) {
	GLfloat tX = (GLfloat)x;
	GLfloat tY = (GLfloat)y;
	GLfloat tW = (GLfloat)Width;
	GLfloat tH = (GLfloat)Height;

	glm::vec4 vclip_left	( 1.0	, 0.0	, 0.0	, -tX		);
	glm::vec4 vclip_right	( -1.0	, 0.0	, 0.0	, tX + tW	);
	glm::vec4 vclip_top		( 0.0	, 1.0	, 0.0	, -tY		);
	glm::vec4 vclip_bottom	( 0.0	, -1.0	, 0.0	, tY + tH	);

	glm::mat4 invMV = glm::inverse( mModelViewMatrix.top() );

	vclip_left		= vclip_left	* invMV;
	vclip_right		= vclip_right	* invMV;
	vclip_top		= vclip_top		* invMV;
	vclip_bottom	= vclip_bottom	* invMV;

	GLi->Enable(GL_CLIP_PLANE0);
	GLi->Enable(GL_CLIP_PLANE1);
	GLi->Enable(GL_CLIP_PLANE2);
	GLi->Enable(GL_CLIP_PLANE3);

	glUniform4fv( mPlanes[0], 1, static_cast<const GLfloat*>( &vclip_left[0]	)	);
	glUniform4fv( mPlanes[1], 1, static_cast<const GLfloat*>( &vclip_right[0]	)	);
	glUniform4fv( mPlanes[2], 1, static_cast<const GLfloat*>( &vclip_top[0]		)	);
	glUniform4fv( mPlanes[3], 1, static_cast<const GLfloat*>( &vclip_bottom[0]	)	);
}

void cRendererGL3::Clip2DPlaneDisable() {
	GLi->Disable(GL_CLIP_PLANE0);
	GLi->Disable(GL_CLIP_PLANE1);
	GLi->Disable(GL_CLIP_PLANE2);
	GLi->Disable(GL_CLIP_PLANE3);
}

void cRendererGL3::PointSize( GLfloat size ) {
	#ifndef EE_GLES2
	glPointSize( size );
	#else
	mCurShader->SetUniform( "dgl_PointSize", size );
	#endif

	mPointSize = size;
}

void cRendererGL3::ClipPlane( GLenum plane, const GLdouble * equation ) {
	Int32 nplane	= plane - GL_CLIP_PLANE0;
	Int32 location;

	if ( nplane < EE_MAX_PLANES ) {
		location = mPlanes[ nplane ];
	} else {
		std::string planeNum( "dgl_ClipPlane[" + toStr( nplane ) + "]" );

		location = glGetUniformLocation( mCurShader->Handler(), (GLchar*)&planeNum[0] );
	}

	glm::vec4 teq( equation[0], equation[1], equation[2], equation[3] );

	teq = teq * glm::inverse( mModelViewMatrix.top() );		/// Apply the inverse of the model view matrix to the equation

	glUniform4f( nplane, (GLfloat)teq[0], (GLfloat)teq[1], (GLfloat)teq[2], (GLfloat)teq[3] );
}

GLfloat cRendererGL3::PointSize() {
	return mPointSize;
}

void cRendererGL3::BindGlobalVAO() {
	#ifndef EE_GLES2
	glBindVertexArray( mVAO );
	#endif
}

void cRendererGL3::ClientActiveTexture( GLenum texture ) {
	/// TODO: Implement multitexturing in GL3 renderer
}

void cRendererGL3::TexEnvi( GLenum target, GLenum pname, GLint param ) {
	/// TODO: Implement TexEnvi
}

}}

#endif
