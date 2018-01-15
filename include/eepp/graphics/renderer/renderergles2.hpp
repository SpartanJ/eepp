#ifndef EE_GRAPHICS_CRENDERERGLES2_HPP
#define EE_GRAPHICS_CRENDERERGLES2_HPP

#include <eepp/graphics/renderer/renderer.hpp>

#ifdef EE_GL3_ENABLED

namespace EE { namespace Graphics {

enum EEGLES2_SHADERS {
	EEGLES2_SHADER_BASE,
	EEGLES2_SHADER_CLIPPED,
	EEGLES2_SHADER_POINTSPRITE,
	EEGLES2_SHADER_PRIMITIVE,
	EEGLES2_SHADERS_COUNT
};

namespace Private {
class MatrixStack;
}

class EE_API RendererGLES2 : public Renderer {
	public:
		RendererGLES2();

		~RendererGLES2();

		EEGL_version version();

		std::string versionStr();

		void init();

		void pointSize( float size );

		float pointSize();

		void pushMatrix();

		void popMatrix();

		void loadIdentity();

		void disable ( unsigned int cap );

		void enable( unsigned int cap );

		void translatef( float x, float y, float z );

		void rotatef( float angle, float x, float y, float z );

		void scalef( float x, float y, float z );

		void matrixMode (unsigned int mode);

		void ortho ( float left, float right, float bottom, float top, float zNear, float zFar );

		void lookAt( float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ );

		void perspective ( float fovy, float aspect, float zNear, float zFar );

		void enableClientState( unsigned int array );

		void disableClientState( unsigned int array );

		void vertexPointer ( int size, unsigned int type, int stride, const void *pointer, unsigned int allocate );

		void colorPointer ( int size, unsigned int type, int stride, const void *pointer, unsigned int allocate );

		void texCoordPointer ( int size, unsigned int type, int stride, const void *pointer, unsigned int allocate );

		void clientActiveTexture( unsigned int texture );

		unsigned int baseShaderId();

		void setShader( ShaderProgram * Shader );

		void setShader( const EEGLES2_SHADERS& Shader );

		int getStateIndex( const Uint32& State );

		void clip2DPlaneEnable( const Int32& x, const Int32& y, const Int32& Width, const Int32& Height );

		void clip2DPlaneDisable();

		void multMatrixf ( const float *m );

		void clipPlane( unsigned int plane, const double *equation );

		void loadMatrixf( const float *m );

		void frustum( float left, float right, float bottom, float top, float near_val, float far_val );

		void getCurrentMatrix( unsigned int mode, float * m );

		unsigned int getCurrentMatrixMode();

		std::string getBaseVertexShader();

		int project( float objx, float objy, float objz, const float modelMatrix[16], const float projMatrix[16], const int viewport[4], float *winx, float *winy, float *winz );

		int unProject( float winx, float winy, float winz, const float modelMatrix[16], const float projMatrix[16], const int viewport[4], float *objx, float *objy, float *objz );

		void reloadCurrentShader();
	protected:
		Private::MatrixStack *	mStack;
		int					mProjectionMatrix_id;	// cpu-side hook to shader uniform
		int					mModelViewMatrix_id;	// cpu-side hook to shader uniform
		unsigned int					mCurrentMode;
		ShaderProgram *		mShaders[ EEGLES2_SHADERS_COUNT ];
		ShaderProgram *		mCurShader;
		int					mAttribsLoc[ EEGL_ARRAY_STATES_COUNT ];
		int					mAttribsLocStates[ EEGL_ARRAY_STATES_COUNT ];
		int					mPlanes[ EE_MAX_PLANES ];
		int					mPlanesStates[ EE_MAX_PLANES ];
		ShaderProgram *		mShaderPrev;
		Int32					mTexActive;
		int					mTexActiveLoc;
		int					mClippingEnabledLoc;
		float					mPointSize;
		int					mTextureUnits[ EE_MAX_TEXTURE_UNITS ];
		int					mTextureUnitsStates[ EE_MAX_TEXTURE_UNITS ];
		int					mCurActiveTex;
		Uint8					mClippingEnabled;
		Uint8					mPointSpriteEnabled;
		bool					mLoaded;
		bool					mCurShaderLocal;
		std::string				mBaseVertexShader;

		void updateMatrix();

		void planeStateCheck( bool tryEnable );

		void reloadShader( ShaderProgram * Shader );

		void checkLocalShader();
};

}}

#endif

#endif
