#ifndef EE_GRAPHICS_CRENDERERGL3_HPP
#define EE_GRAPHICS_CRENDERERGL3_HPP

#include <eepp/graphics/renderer/cgl.hpp>

#ifdef EE_GL3_ENABLED

namespace EE { namespace Graphics {

enum EEGL3_SHADERS {
	EEGL3_SHADER_BASE,
	EEGL3_SHADERS_COUNT
};

namespace Private {
class cMatrixStack;
}

class EE_API cRendererGL3 : public cGL {
	public:
		cRendererGL3();

		~cRendererGL3();

		EEGL_version Version();

		std::string VersionStr();

		void Init();

		void PointSize( GLfloat size );

		GLfloat PointSize();

		void PushMatrix();

		void PopMatrix();

		void LoadIdentity();

		void Disable ( GLenum cap );

		void Enable( GLenum cap );

		void Translatef( GLfloat x, GLfloat y, GLfloat z );

		void Rotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z );

		void Scalef( GLfloat x, GLfloat y, GLfloat z );

		void MatrixMode (GLenum mode);

		void Ortho ( GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar );

		void LookAt( GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ );

		void Perspective ( GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar );

		void EnableClientState( GLenum array );

		void DisableClientState( GLenum array );

		void VertexPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer, GLuint allocate );

		void ColorPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer, GLuint allocate );

		void TexCoordPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer, GLuint allocate );

		void ClientActiveTexture( GLenum texture );

		GLuint BaseShaderId();

		void SetShader( cShaderProgram * Shader );

		void SetShader( const EEGL3_SHADERS& Shader );

		GLint GetStateIndex( const Uint32& State );

		void Clip2DPlaneEnable( const Int32& x, const Int32& y, const Int32& Width, const Int32& Height );

		void Clip2DPlaneDisable();

		void MultMatrixf ( const GLfloat *m );

		void ClipPlane( GLenum plane, const GLdouble *equation );

		void TexEnvi( GLenum target, GLenum pname, GLint param );

		void LoadMatrixf( const GLfloat *m );

		void Frustum( GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val );

		void GetCurrentMatrix( GLenum mode, GLfloat * m );

		GLenum GetCurrentMatrixMode();

		std::string GetBaseVertexShader();

		GLint Project( GLfloat objx, GLfloat objy, GLfloat objz, const GLfloat modelMatrix[16], const GLfloat projMatrix[16], const GLint viewport[4], GLfloat *winx, GLfloat *winy, GLfloat *winz );

		GLint UnProject( GLfloat winx, GLfloat winy, GLfloat winz, const GLfloat modelMatrix[16], const GLfloat projMatrix[16], const GLint viewport[4], GLfloat *objx, GLfloat *objy, GLfloat *objz );
	protected:
		Private::cMatrixStack *	mStack;
		GLint					mProjectionMatrix_id;	// cpu-side hook to shader uniform
		GLint					mModelViewMatrix_id;	// cpu-side hook to shader uniform
		GLenum					mCurrentMode;
		cShaderProgram *		mShaders[ EEGL3_SHADERS_COUNT ];
		cShaderProgram *		mCurShader;
		GLint					mAttribsLoc[ EEGL_ARRAY_STATES_COUNT ];
		GLint					mAttribsLocStates[ EEGL_ARRAY_STATES_COUNT ];
		GLint					mPlanes[ EE_MAX_PLANES ];
		GLint					mPlanesStates[ EE_MAX_PLANES ];
		cShaderProgram *		mShaderPrev;
		Int32					mTexActive;
		GLint					mTexActiveLoc;
		GLint					mPointSpriteLoc;
		GLint					mClippingEnabledLoc;
		GLfloat					mPointSize;
		GLint					mTextureUnits[ EE_MAX_TEXTURE_UNITS ];
		GLint					mTextureUnitsStates[ EE_MAX_TEXTURE_UNITS ];
		GLint					mCurActiveTex;
		bool					mLoaded;
		std::string				mBaseVertexShader;

		void UpdateMatrix();

		void PlaneStateCheck( bool tryEnable );

		void ReloadShader( cShaderProgram * Shader );
};

}}

#endif

#endif
