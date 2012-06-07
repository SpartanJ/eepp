#ifndef EE_GRAPHICS_CRENDERERGL3_HPP
#define EE_GRAPHICS_CRENDERERGL3_HPP

#include "cgl.hpp"

#ifdef EE_GL3_ENABLED

// Xlib Madness
#ifdef True
#undef True
#endif

#ifdef False
#undef False
#endif

#include "../../helper/glm/gtx/transform.hpp"

namespace EE { namespace Graphics {

#define EE_MAX_PLANES 6

/** Just for reference */
enum EEGL_ARRAY_STATES {
	EEGL_VERTEX_ARRAY			= 0,
	EEGL_NORMAL_ARRAY			= 1,
	EEGL_COLOR_ARRAY			= 2,
	EEGL_INDEX_ARRAY			= 3,
	EEGL_TEXTURE_COORD_ARRAY	= 4,
	EEGL_EDGE_FLAG_ARRAY		= 5,
	EEGL_TEXTURE_COORD_ARRAY1	= 6,
	EEGL_TEXTURE_COORD_ARRAY2	= 7,
	EEGL_TEXTURE_COORD_ARRAY3	= 8,
	EEGL_ARRAY_STATES_COUNT
};

enum EEGL_SHADERS {
	EEGL_SHADER_BASE,
	EEGL_SHADERS_COUNT
};

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

		void SetShader( const EEGL_SHADERS& Shader );

		GLint GetStateIndex( const Uint32& State );

		void Clip2DPlaneEnable( const Int32& x, const Int32& y, const Int32& Width, const Int32& Height );

		void Clip2DPlaneDisable();

		void MultMatrixf ( const GLfloat *m );

		void ClipPlane( GLenum plane, const GLdouble *equation );

		void TexEnvi( GLenum target, GLenum pname, GLint param );

		void LoadMatrixf( const GLfloat *m );

		void Frustum( GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val );

		void GetCurrentMatrix( GLenum mode, GLfloat * m );

		glm::mat4 toGLMmat4( const GLfloat * m );

		void fromGLMmat4( glm::mat4 from, GLfloat * to );

		GLenum GetCurrentMatrixMode();

		std::string GetBaseVertexShader();

		GLint Project( GLfloat objx, GLfloat objy, GLfloat objz, const GLfloat modelMatrix[16], const GLfloat projMatrix[16], const GLint viewport[4], GLfloat *winx, GLfloat *winy, GLfloat *winz );

		GLint UnProject( GLfloat winx, GLfloat winy, GLfloat winz, const GLfloat modelMatrix[16], const GLfloat projMatrix[16], const GLint viewport[4], GLfloat *objx, GLfloat *objy, GLfloat *objz );
	protected:
		std::stack<glm::mat4>	mProjectionMatrix;		// cpu-side
		GLint					mProjectionMatrix_id;	// cpu-side hook to shader uniform
		std::stack<glm::mat4>	mModelViewMatrix;		// cpu-side
		GLint					mModelViewMatrix_id;	// cpu-side hook to shader uniform
		GLenum					mCurrentMode;
		std::stack<glm::mat4>*	mCurMatrix;
		cShaderProgram *		mShaders[ EEGL_SHADERS_COUNT ];
		cShaderProgram *		mCurShader;
		GLint					mStates[ EEGL_ARRAY_STATES_COUNT ];
		GLint					mPlanes[ EE_MAX_PLANES ];
		GLint					mPlanesStates[ EE_MAX_PLANES ];
		cShaderProgram *		mShaderPrev;
		Int32					mTexActive;
		GLint					mTexActiveLoc;
		GLint					mPointSpriteLoc;
		GLint					mClippingEnabledLoc;
		GLfloat					mPointSize;
		GLint					mTextureUnits[ EE_MAX_TEXTURE_UNITS ];
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
