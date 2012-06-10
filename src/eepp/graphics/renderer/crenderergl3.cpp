#include <eepp/graphics/renderer/crenderergl3.hpp>

#ifdef EE_GL3_ENABLED

namespace EE { namespace Graphics {

const char * EEGL_STATES_NAME[] = {
	"dgl_Vertex",
	"dgl_Normal",
	"dgl_FrontColor"
};

const char * EEGL_TEXTUREUNIT_NAMES[] = {
	"dgl_MultiTexCoord0",
	"dgl_MultiTexCoord1",
	"dgl_MultiTexCoord2",
	"dgl_MultiTexCoord3"
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
	"#define MAX_CLIP_PLANES 6\n",
	"#ifdef GL_ES\n",
	"precision mediump float;\n",
	"precision lowp int;\n",
	"#else\n",
	"#version 120\n",
	"#endif\n",
	"uniform			mat4 dgl_ProjectionMatrix;\n",
	"uniform			mat4 dgl_ModelViewMatrix;\n",
	"uniform			int  dgl_ClippingEnabled;\n",
	"uniform			int	 dgl_ClipEnabled[ MAX_CLIP_PLANES ];\n",
	"uniform			vec4 dgl_ClipPlane[ MAX_CLIP_PLANES ];\n",
	"uniform			float dgl_PointSize;\n",
	"attribute			vec4 dgl_Vertex;\n",
	"attribute			vec4 dgl_FrontColor;\n",
	"attribute			vec4 dgl_MultiTexCoord0;\n",
	"varying			vec4 dgl_Color;\n",
	"varying			vec4 dgl_TexCoord[ 1 ];\n",
	"varying			float dgl_ClipDistance[ MAX_CLIP_PLANES ];\n",
	"void main(void)\n",
	"{\n",
	"	gl_PointSize	= dgl_PointSize;\n",
	"	dgl_Color		= dgl_FrontColor;\n",
	"	dgl_TexCoord[0]	= dgl_MultiTexCoord0;\n",
	"	vec4 vEye		= dgl_ModelViewMatrix * dgl_Vertex;\n",
	"	gl_Position		= dgl_ProjectionMatrix * vEye;\n",
	"	if ( 1 == dgl_ClippingEnabled ) {\n",
	"		for ( int i = 0; i < MAX_CLIP_PLANES; i++ ) {\n",
	"			if ( 1 == dgl_ClipEnabled[i] )\n",
	"				dgl_ClipDistance[i] = dot( vEye, dgl_ClipPlane[i] );\n",
	"		}\n",
	"	}\n",
	"}\n"
};

const GLchar * EEGL_SHADER_BASE_FS[] = {
	"#define MAX_CLIP_PLANES 6\n",
	"#ifdef GL_ES\n",
	"precision mediump float;\n",
	"precision lowp int;\n",
	"#else\n",
	"#version 120\n",
	"#endif\n",
	"uniform		sampler2D	textureUnit0;\n",
	"uniform		int			dgl_TexActive;\n",
	"uniform		int			dgl_PointSpriteActive;\n",
	"uniform		int			dgl_ClippingEnabled;\n",
	"uniform		int			dgl_ClipEnabled[ MAX_CLIP_PLANES ];\n",
	"uniform		vec4		dgl_ClipPlane[ MAX_CLIP_PLANES ];\n",
	"varying		vec4		dgl_Color;\n",
	"varying		vec4		dgl_TexCoord[ 1 ];\n",
	"varying		float		dgl_ClipDistance[ MAX_CLIP_PLANES ];\n",
	"void main(void)\n",
	"{\n",
	"	if ( 1 == dgl_ClippingEnabled ) {\n",
	"		for ( int i = 0; i < MAX_CLIP_PLANES; i++ ) {\n",
	"			if ( 1 == dgl_ClipEnabled[i] )\n",
	"				if ( dgl_ClipDistance[i] < 0.0 )\n",
	"					discard;\n",
	"		}\n",
	"	}\n",
	"	if ( 0 == dgl_PointSpriteActive ) {\n",
	"		if ( 1 == dgl_TexActive )\n",
	"			gl_FragColor = dgl_Color * texture2D( textureUnit0, dgl_TexCoord[ 0 ].xy );\n",
	"		else\n",
	"			gl_FragColor = dgl_Color;\n",
	"	} else\n",
	"		gl_FragColor = dgl_Color * texture2D( textureUnit0, gl_PointCoord );\n",
	"}\n",
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
	mClippingEnabledLoc(-1),
	mPointSize(1.f),
	mCurActiveTex( 0 ),
	mLoaded( false )
{
	mProjectionMatrix.push	( glm::mat4( 1.0f ) ); // identity matrix
	mModelViewMatrix.push	( glm::mat4( 1.0f ) ); // identity matrix
}

cRendererGL3::~cRendererGL3() {
}

EEGL_version cRendererGL3::Version() {
	#ifndef EE_GLES2
	return GLv_3;
	#else
	return GLv_ES2;
	#endif
}

std::string cRendererGL3::VersionStr() {
	#ifndef EE_GLES2
	return "OpenGL 3";
	#else
	return "OpenGL ES 2";
	#endif
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
	mClippingEnabledLoc		= mCurShader->UniformLocation( "dgl_ClippingEnabled" );
	mCurActiveTex			= 0;

	Uint32 i;

	for ( i = 0; i < EEGL_ARRAY_STATES_COUNT; i++ ) {
		mStates[ i ] = mCurShader->AttributeLocation( EEGL_STATES_NAME[ i ] );
	}

	for ( i = 0; i < EE_MAX_PLANES; i++ ) {
		mPlanes[ i ] = mCurShader->UniformLocation( EEGL_PLANES_NAME[ i ] );
	}

	for ( i = 0; i < EE_MAX_TEXTURE_UNITS; i++ ) {
		mTextureUnits[ i ] = mCurShader->AttributeLocation( EEGL_TEXTUREUNIT_NAMES[ i ] );
	}

	DisableClientState( GL_VERTEX_ARRAY );
	DisableClientState( GL_TEXTURE_COORD_ARRAY );
	DisableClientState( GL_COLOR_ARRAY );

	glUseProgram( mCurShader->Handler() );

	GLenum CM = mCurrentMode;

	MatrixMode( GL_PROJECTION );
	UpdateMatrix();
	MatrixMode( GL_MODELVIEW );
	UpdateMatrix();
	MatrixMode( CM );

	#ifdef EE_GLES2
	if ( -1 != mTexActiveLoc ) {
		mCurShader->SetUniform( mTexActiveLoc, 1 );
	}

	mCurShader->SetUniform( mClippingEnabledLoc, 0 );

	for ( i = 0; i < EE_MAX_PLANES; i++ ) {
		if ( -1 != mPlanes[ i ] ) {
			mCurShader->SetUniform( EEGL_PLANES_ENABLED_NAME[ i ], 0 );
		}
	}

	if ( -1 != mPointSpriteLoc ) {
		mCurShader->SetUniform( mPointSpriteLoc, 0 );
	}
	#endif
}

void cRendererGL3::Enable( GLenum cap ) {
	switch ( cap ) {
		case GL_TEXTURE_2D:
		{
			if ( 0 == mTexActive ) {
				mTexActive = 1;
				mCurShader->SetUniform( mTexActiveLoc, mTexActive );
			}

			return;
		}
		case GL_CLIP_PLANE0:
		case GL_CLIP_PLANE1:
		case GL_CLIP_PLANE2:
		case GL_CLIP_PLANE3:
		case GL_CLIP_PLANE4:
		case GL_CLIP_PLANE5:
		{
			GLint plane = cap - GL_CLIP_PLANE0;

			if ( 0 == mPlanesStates[ plane ] ) {
				mPlanesStates[ plane ] = 1;
				PlaneStateCheck( true );
				mCurShader->SetUniform( EEGL_PLANES_ENABLED_NAME[ plane ], 1 );
			}

			return;
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
			if ( 1 == mTexActive ) {
				mTexActive = 0;
				mCurShader->SetUniform( mTexActiveLoc, mTexActive );
			}

			return;
		}
		case GL_CLIP_PLANE0:
		case GL_CLIP_PLANE1:
		case GL_CLIP_PLANE2:
		case GL_CLIP_PLANE3:
		case GL_CLIP_PLANE4:
		case GL_CLIP_PLANE5:
		{
			GLint plane = cap - GL_CLIP_PLANE0;

			if ( 1 == mPlanesStates[ plane ] ) {
				mPlanesStates[ plane ] = 0;
				PlaneStateCheck( false );
				mCurShader->SetUniform( EEGL_PLANES_ENABLED_NAME[ plane ], 0 );
			}

			return;
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
				mCurShader->SetUniform( mClippingEnabledLoc, 1 );
				return;
			}
		}
	} else {
		for ( i = 0; i < EE_MAX_PLANES; i++) {
			if ( 0 != mPlanesStates[ i ] ) {
				return;
			}
		}

		mCurShader->SetUniform( mClippingEnabledLoc, 0 );
	}
}

void cRendererGL3::Init() {
	if ( !mLoaded ) {
		cGL::Init();

		mBaseVertexShader = "";

		for ( Uint32 i = 0; i < sizeof(EEGL_SHADER_BASE_VS)/sizeof(const GLchar*); i++ ) {
			mBaseVertexShader += std::string( EEGL_SHADER_BASE_VS[i] );
		}

		Uint32 i;

		for ( i = 0; i < EE_MAX_PLANES; i++ ) {
			mPlanes[i]			= -1;
			mPlanesStates[i]	= 0;
		}

		for ( i = 0; i < EE_MAX_TEXTURE_UNITS; i++ )
			mTextureUnits[i] = -1;

		cShader::Ensure = false;
		mShaders[ EEGL_SHADER_BASE ]			= eeNew( cShaderProgram, ( (const char**)EEGL_SHADER_BASE_VS, sizeof(EEGL_SHADER_BASE_VS)/sizeof(const GLchar*), (const char**)EEGL_SHADER_BASE_FS, sizeof(EEGL_SHADER_BASE_FS)/sizeof(const GLchar*), "EEGL_SHADER_BASE_TEX" ) );
		mShaders[ EEGL_SHADER_BASE ]->SetReloadCb( cb::Make1( this, &cRendererGL3::ReloadShader ) );
		cShader::Ensure = true;

		SetShader( mShaders[ EEGL_SHADER_BASE ] );
	} else {
		mCurShader = NULL;

		mShaders[ EEGL_SHADER_BASE ]->Reload();

		SetShader( mShaders[ EEGL_SHADER_BASE ] );
	}

	ClientActiveTexture( GL_TEXTURE0 );

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
		case GL_PROJECTION_MATRIX:
		{
			fromGLMmat4( mProjectionMatrix.top(), m );
			break;
		}
		case GL_MODELVIEW:
		case GL_MODELVIEW_MATRIX:
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
		case GL_PROJECTION_MATRIX:
		{
			mCurMatrix = &mProjectionMatrix;
			break;
		}
		case GL_MODELVIEW:
		case GL_MODELVIEW_MATRIX:
		{
			mCurMatrix = &mModelViewMatrix;
			break;
		}
	}
}

void cRendererGL3::EnableClientState( GLenum array ) {
	GLint state;

	if ( GL_TEXTURE_COORD_ARRAY == array ) {
		if ( -1 != ( state = mTextureUnits[ mCurActiveTex ] ) )
			glEnableVertexAttribArray( state );
	} else {
		if ( -1 != ( state = mStates[ array - GL_VERTEX_ARRAY ] ) )
			glEnableVertexAttribArray( state );
	}
}

void cRendererGL3::DisableClientState( GLenum array ) {
	GLint state;

	if ( GL_TEXTURE_COORD_ARRAY == array ) {
		if ( -1 != ( state = mTextureUnits[ mCurActiveTex ] ) )
			glDisableVertexAttribArray( state );
	} else {
		if ( -1 != ( state = mStates[ array - GL_VERTEX_ARRAY ] ) )
			glDisableVertexAttribArray( state );
	}
}

void cRendererGL3::VertexPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid * pointer, GLuint allocate ) {
	const GLint index = mStates[ EEGL_VERTEX_ARRAY ];

	if ( -1 != index ) {
		glEnableVertexAttribArray( index );

		glVertexAttribPointerARB( index, size, type, GL_FALSE, stride, pointer );
	}
}

void cRendererGL3::ColorPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer, GLuint allocate ) {
	const GLint index = mStates[ EEGL_COLOR_ARRAY ];

	if ( -1 != index ) {
		glEnableVertexAttribArray( index );

		if ( type == GL_UNSIGNED_BYTE ) {
			glVertexAttribPointerARB( index, size, type, GL_TRUE, stride, pointer );
		} else {
			glVertexAttribPointerARB( index, size, type, GL_FALSE, stride, pointer );
		}
	}
}

void cRendererGL3::TexCoordPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer, GLuint allocate ) {
	const GLint index = mTextureUnits[ mCurActiveTex ];

	if ( -1 != index ) {
		glEnableVertexAttribArray( index );

		glVertexAttribPointerARB( index, size, type, GL_FALSE, stride, pointer );
	}
}

GLint cRendererGL3::GetStateIndex( const Uint32& State ) {
	eeASSERT( State < EEGL_ARRAY_STATES_COUNT );

	if ( EEGL_TEXTURE_COORD_ARRAY == State )
		return mTextureUnits[ mCurActiveTex ];

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
	mCurShader->SetUniform( "dgl_PointSize", size );

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

	glUniform4f( location, (GLfloat)teq[0], (GLfloat)teq[1], (GLfloat)teq[2], (GLfloat)teq[3] );
}

GLfloat cRendererGL3::PointSize() {
	return mPointSize;
}

void cRendererGL3::ClientActiveTexture( GLenum texture ) {
	mCurActiveTex = texture - GL_TEXTURE0;

	if ( mCurActiveTex >= EE_MAX_TEXTURE_UNITS )
		mCurActiveTex = 0;
}

void cRendererGL3::TexEnvi( GLenum target, GLenum pname, GLint param ) {
	//! @TODO: Implement TexEnvi
}

std::string cRendererGL3::GetBaseVertexShader() {
	return mBaseVertexShader;
}

GLint cRendererGL3::Project( GLfloat objx, GLfloat objy, GLfloat objz, const GLfloat modelMatrix[16], const GLfloat projMatrix[16], const GLint viewport[4], GLfloat *winx, GLfloat *winy, GLfloat *winz ) {
	glm::vec3 tv3( glm::project( glm::vec3( objx, objy, objz ), toGLMmat4( modelMatrix ), toGLMmat4( projMatrix ), glm::vec4( viewport[0], viewport[1], viewport[2], viewport[3] ) ) );

	if ( NULL != winx )
		*winx = tv3.x;

	if ( NULL != winy )
		*winy = tv3.y;

	if ( NULL != winz )
		*winz = tv3.z;

	return GL_TRUE;
}

GLint cRendererGL3::UnProject( GLfloat winx, GLfloat winy, GLfloat winz, const GLfloat modelMatrix[16], const GLfloat projMatrix[16], const GLint viewport[4], GLfloat *objx, GLfloat *objy, GLfloat *objz ) {
	glm::vec3 tv3( glm::unProject( glm::vec3( winx, winy, winz ), toGLMmat4( modelMatrix ), toGLMmat4( projMatrix ), glm::vec4( viewport[0], viewport[1], viewport[2], viewport[3] ) ) );

	if ( NULL != objx )
		*objx = tv3.x;

	if ( NULL != objy )
		*objy = tv3.y;

	if ( NULL != objz )
		*objz = tv3.z;

	return GL_TRUE;
}

}}

#endif
