#ifndef EE_GRAPHICS_CRENDERERGLES2_HPP
#define EE_GRAPHICS_CRENDERERGLES2_HPP

#include <eepp/graphics/renderer/rendererglshader.hpp>

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

class EE_API RendererGLES2 : public RendererGLShader {
	public:
		RendererGLES2();

		~RendererGLES2();

		EEGL_version version();

		std::string versionStr();

		void init();

		void pointSize( float size );

		float pointSize();

		void disable ( unsigned int cap );

		void enable( unsigned int cap );

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

		void clipPlane( unsigned int plane, const double *equation );

		std::string getBaseVertexShader();

		void reloadCurrentShader();
	protected:
		ShaderProgram *		mShaders[ EEGLES2_SHADERS_COUNT ];
		int					mAttribsLoc[ EEGL_ARRAY_STATES_COUNT ];
		int					mAttribsLocStates[ EEGL_ARRAY_STATES_COUNT ];
		int					mPlanes[ EE_MAX_PLANES ];
		int					mPlanesStates[ EE_MAX_PLANES ];
		Int32				mTexActive;
		int					mTexActiveLoc;
		int					mClippingEnabledLoc;
		float				mPointSize;
		int					mTextureUnits[ EE_MAX_TEXTURE_UNITS ];
		int					mTextureUnitsStates[ EE_MAX_TEXTURE_UNITS ];
		int					mCurActiveTex;
		Uint8				mClippingEnabled;
		Uint8				mPointSpriteEnabled;
		bool				mLoaded;
		bool				mCurShaderLocal;
		std::string			mBaseVertexShader;

		void planeStateCheck( bool tryEnable );

		void reloadShader( ShaderProgram * Shader );

		void checkLocalShader();
};

}}

#endif

#endif
