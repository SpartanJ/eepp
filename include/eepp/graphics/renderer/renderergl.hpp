#ifndef EE_GRAPHICS_CRENDERERGL_HPP
#define EE_GRAPHICS_CRENDERERGL_HPP

#include <eepp/graphics/renderer/renderer.hpp>

namespace EE { namespace Graphics {

//! Avoid compilling the fixed pipeline renderer for GLES2, because it's not supported.
#if !defined( EE_GLES2 ) || defined( EE_GLES_BOTH )

class EE_API RendererGL : public Renderer {
  public:
	RendererGL();

	~RendererGL();

	EEGL_version version();

	std::string versionStr();

	void clientActiveTexture( unsigned int texture );

	void pointSize( float size );

	float pointSize();

	void pushMatrix();

	void popMatrix();

	void loadIdentity();

	void translatef( float x, float y, float z );

	void rotatef( float angle, float x, float y, float z );

	void scalef( float x, float y, float z );

	void matrixMode( unsigned int mode );

	void ortho( float left, float right, float bottom, float top, float zNear, float zFar );

	void lookAt( float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ,
				 float upX, float upY, float upZ );

	void perspective( float fovy, float aspect, float zNear, float zFar );

	void enableClientState( unsigned int array );

	void disableClientState( unsigned int array );

	void vertexPointer( int size, unsigned int type, int stride, const void* pointer,
						unsigned int allocate );

	void colorPointer( int size, unsigned int type, int stride, const void* pointer,
					   unsigned int allocate );

	void texCoordPointer( int size, unsigned int type, int stride, const void* pointer,
						  unsigned int allocate );

	void clipPlane( unsigned int plane, const double* equation );

	void clip2DPlaneEnable( const Int32& x, const Int32& y, const Int32& Width,
							const Int32& Height );

	void clip2DPlaneDisable();

	void multMatrixf( const float* m );

	void loadMatrixf( const float* m );

	void frustum( float left, float right, float bottom, float top, float near_val, float far_val );

	void getCurrentMatrix( unsigned int mode, float* m );

	unsigned int getCurrentMatrixMode();

	int project( float objx, float objy, float objz, const float modelMatrix[16],
				 const float projMatrix[16], const int viewport[4], float* winx, float* winy,
				 float* winz );

	int unProject( float winx, float winy, float winz, const float modelMatrix[16],
				   const float projMatrix[16], const int viewport[4], float* objx, float* objy,
				   float* objz );

  protected:
};

#endif

}} // namespace EE::Graphics

#endif
