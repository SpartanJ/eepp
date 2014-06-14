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

		void PointSize( float size );

		float PointSize();

		void PushMatrix();

		void PopMatrix();

		void LoadIdentity();

		void Disable ( unsigned int cap );

		void Enable( unsigned int cap );

		void Translatef( float x, float y, float z );

		void Rotatef( float angle, float x, float y, float z );

		void Scalef( float x, float y, float z );

		void MatrixMode (unsigned int mode);

		void Ortho ( float left, float right, float bottom, float top, float zNear, float zFar );

		void LookAt( float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ );

		void Perspective ( float fovy, float aspect, float zNear, float zFar );

		void EnableClientState( unsigned int array );

		void DisableClientState( unsigned int array );

		void VertexPointer ( int size, unsigned int type, int stride, const void *pointer, unsigned int allocate );

		void ColorPointer ( int size, unsigned int type, int stride, const void *pointer, unsigned int allocate );

		void TexCoordPointer ( int size, unsigned int type, int stride, const void *pointer, unsigned int allocate );

		void ClientActiveTexture( unsigned int texture );

		unsigned int BaseShaderId();

		void SetShader( cShaderProgram * Shader );

		void SetShader( const EEGL3_SHADERS& Shader );

		int GetStateIndex( const Uint32& State );

		void Clip2DPlaneEnable( const Int32& x, const Int32& y, const Int32& Width, const Int32& Height );

		void Clip2DPlaneDisable();

		void MultMatrixf ( const float *m );

		void ClipPlane( unsigned int plane, const double *equation );

		void TexEnvi( unsigned int target, unsigned int pname, int param );

		void LoadMatrixf( const float *m );

		void Frustum( float left, float right, float bottom, float top, float near_val, float far_val );

		void GetCurrentMatrix( unsigned int mode, float * m );

		unsigned int GetCurrentMatrixMode();

		std::string GetBaseVertexShader();

		int Project( float objx, float objy, float objz, const float modelMatrix[16], const float projMatrix[16], const int viewport[4], float *winx, float *winy, float *winz );

		int UnProject( float winx, float winy, float winz, const float modelMatrix[16], const float projMatrix[16], const int viewport[4], float *objx, float *objy, float *objz );

		void ReloadCurrentShader();
	protected:
		Private::cMatrixStack *	mStack;
		int					mProjectionMatrix_id;	// cpu-side hook to shader uniform
		int					mModelViewMatrix_id;	// cpu-side hook to shader uniform
		unsigned int					mCurrentMode;
		cShaderProgram *		mShaders[ EEGL3_SHADERS_COUNT ];
		cShaderProgram *		mCurShader;
		int					mAttribsLoc[ EEGL_ARRAY_STATES_COUNT ];
		int					mAttribsLocStates[ EEGL_ARRAY_STATES_COUNT ];
		int					mPlanes[ EE_MAX_PLANES ];
		int					mPlanesStates[ EE_MAX_PLANES ];
		cShaderProgram *		mShaderPrev;
		Int32					mTexActive;
		int					mTexActiveLoc;
		int					mPointSpriteLoc;
		int					mClippingEnabledLoc;
		float					mPointSize;
		int					mTextureUnits[ EE_MAX_TEXTURE_UNITS ];
		int					mTextureUnitsStates[ EE_MAX_TEXTURE_UNITS ];
		int					mCurActiveTex;
		bool					mLoaded;
		std::string				mBaseVertexShader;

		void UpdateMatrix();

		void PlaneStateCheck( bool tryEnable );

		void ReloadShader( cShaderProgram * Shader );
};

}}

#endif

#endif
