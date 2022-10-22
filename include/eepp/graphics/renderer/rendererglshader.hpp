#ifndef EE_GRAPHICS_RENDERERGLSHADER_HPP
#define EE_GRAPHICS_RENDERERGLSHADER_HPP

#include <eepp/graphics/renderer/renderer.hpp>

namespace EE { namespace Graphics {

namespace Private {
class MatrixStack;
}

class EE_API RendererGLShader : public Renderer {
  public:
	RendererGLShader();

	~RendererGLShader();

	void pushMatrix();

	void popMatrix();

	void loadIdentity();

	void loadMatrixf( const float* m );

	void translatef( float x, float y, float z );

	void rotatef( float angle, float x, float y, float z );

	void scalef( float x, float y, float z );

	void matrixMode( unsigned int mode );

	void ortho( float left, float right, float bottom, float top, float zNear, float zFar );

	void lookAt( float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ,
				 float upX, float upY, float upZ );

	void perspective( float fovy, float aspect, float zNear, float zFar );

	void frustum( float left, float right, float bottom, float top, float near_val, float far_val );

	void getCurrentMatrix( unsigned int mode, float* m );

	void multMatrixf( const float* m );

	unsigned int getCurrentMatrixMode();

	int project( float objx, float objy, float objz, const float modelMatrix[16],
				 const float projMatrix[16], const int viewport[4], float* winx, float* winy,
				 float* winz );

	int unProject( float winx, float winy, float winz, const float modelMatrix[16],
				   const float projMatrix[16], const int viewport[4], float* objx, float* objy,
				   float* objz );

  protected:
	Private::MatrixStack* mStack;
	int mProjectionMatrix_id; // cpu-side hook to shader uniform
	int mModelViewMatrix_id;  // cpu-side hook to shader uniform
	int mTextureMatrix_id;	  // cpu-side hook to shader uniform
	unsigned int mCurrentMode;
	ShaderProgram* mCurShader;
	ShaderProgram* mShaderPrev;

	void updateMatrix();
};

}} // namespace EE::Graphics

#endif
