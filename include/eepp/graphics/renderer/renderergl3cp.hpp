#ifndef EE_GRAPHICS_CRENDERERGL3CP_HPP
#define EE_GRAPHICS_CRENDERERGL3CP_HPP

#include <eepp/graphics/renderer/rendererglshader.hpp>

#ifdef EE_GL3_ENABLED

namespace EE { namespace Graphics {

enum EEGL3CP_SHADERS { EEGL3CP_SHADER_BASE, EEGL3CP_SHADERS_COUNT };

namespace Private {
class MatrixStack;
}

class EE_API RendererGL3CP : public RendererGLShader {
  public:
	RendererGL3CP();

	~RendererGL3CP();

	EEGL_version version();

	std::string versionStr();

	void init();

	void pointSize( float size );

	float pointSize();

	void disable( unsigned int cap );

	void enable( unsigned int cap );

	void enableClientState( unsigned int array );

	void disableClientState( unsigned int array );

	void vertexPointer( int size, unsigned int type, int stride, const void* pointer,
						unsigned int allocate );

	void colorPointer( int size, unsigned int type, int stride, const void* pointer,
					   unsigned int allocate );

	void texCoordPointer( int size, unsigned int type, int stride, const void* pointer,
						  unsigned int allocate );

	void clientActiveTexture( unsigned int texture );

	unsigned int baseShaderId();

	void setShader( ShaderProgram* Shader );

	void setShader( const EEGL3CP_SHADERS& Shader );

	int getStateIndex( const Uint32& State );

	void clip2DPlaneEnable( const Int32& x, const Int32& y, const Int32& Width,
							const Int32& Height );

	void clip2DPlaneDisable();

	void clipPlane( unsigned int plane, const double* equation );

	void bindGlobalVAO();

	std::string getBaseVertexShader();

	void reloadCurrentShader();

  protected:
	ShaderProgram* mShaders[EEGL3CP_SHADERS_COUNT];
	unsigned int mVAO;
	unsigned int mVBO[8];
	int mAttribsLoc[EEGL_ARRAY_STATES_COUNT];
	int mAttribsLocStates[EEGL_ARRAY_STATES_COUNT];
	int mPlanes[EE_MAX_PLANES];
	int mPlanesStates[EE_MAX_PLANES];
	Int32 mTexActive;
	int mTexActiveLoc;
	int mPointSpriteLoc;
	int mClippingEnabledLoc;
	float mPointSize;
	int mTextureUnits[EE_MAX_TEXTURE_UNITS];
	int mTextureUnitsStates[EE_MAX_TEXTURE_UNITS];
	int mCurActiveTex;
	unsigned int mCurTexCoordArray;
	Uint32 mVBOSizeAlloc;
	Uint32 mBiggestAlloc;
	bool mLoaded;
	std::string mBaseVertexShader;

	void planeStateCheck( bool tryEnable );

	void reloadShader( ShaderProgram* Shader );

	void allocateBuffers( const Uint32& size );
};

}} // namespace EE::Graphics

#endif

#endif
