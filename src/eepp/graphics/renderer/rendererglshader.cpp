#include <eepp/graphics/renderer/openglext.hpp>
#include <eepp/graphics/renderer/rendererglshader.hpp>
#include <eepp/graphics/renderer/rendererstackhelper.hpp>

namespace EE { namespace Graphics {

RendererGLShader::RendererGLShader() :
	mProjectionMatrix_id( 0 ),
	mModelViewMatrix_id( 0 ),
	mTextureMatrix_id( 0 ),
	mCurrentMode( 0 ),
	mCurShader( NULL ),
	mShaderPrev( NULL ) {
	mStack = eeNew( Private::MatrixStack, () );
	mStack->mProjectionMatrix.push( glm::mat4( 1.0f ) ); // identity matrix
	mStack->mModelViewMatrix.push( glm::mat4( 1.0f ) );	 // identity matrix
	mStack->mTextureMatrix.push( glm::mat4( 1.0f ) );	 // identity matrix
}

RendererGLShader::~RendererGLShader() {
	eeSAFE_DELETE( mStack );
}

void RendererGLShader::updateMatrix() {
	switch ( mCurrentMode ) {
		case GL_PROJECTION: {
			if ( -1 != mProjectionMatrix_id ) {
				mCurShader->setUniformMatrix( mProjectionMatrix_id,
											  &mStack->mProjectionMatrix.top()[0][0] );
			}

			break;
		}
		case GL_MODELVIEW: {
			if ( -1 != mModelViewMatrix_id ) {
				mCurShader->setUniformMatrix( mModelViewMatrix_id,
											  &mStack->mModelViewMatrix.top()[0][0] );
			}

			break;
		}
		case GL_TEXTURE: {
			if ( -1 != mTextureMatrix_id ) {
				mCurShader->setUniformMatrix( mTextureMatrix_id,
											  &mStack->mTextureMatrix.top()[0][0] );
			}

			break;
		}
	}
}

void RendererGLShader::pushMatrix() {
	mStack->mCurMatrix->push( mStack->mCurMatrix->top() );
	updateMatrix();
}

void RendererGLShader::popMatrix() {
	mStack->mCurMatrix->pop();
	updateMatrix();
}

void RendererGLShader::loadIdentity() {
	mStack->mCurMatrix->top() = glm::mat4( 1.0 );
	updateMatrix();
}

void RendererGLShader::multMatrixf( const float* m ) {
	mStack->mCurMatrix->top() *= toGLMmat4( m );
	updateMatrix();
}

void RendererGLShader::translatef( float x, float y, float z ) {
	mStack->mCurMatrix->top() *= glm::translate( glm::vec3( x, y, z ) );
	updateMatrix();
}

void RendererGLShader::rotatef( float angle, float x, float y, float z ) {
	mStack->mCurMatrix->top() *= glm::rotate( angle, glm::vec3( x, y, z ) );
	updateMatrix();
}

void RendererGLShader::scalef( float x, float y, float z ) {
	mStack->mCurMatrix->top() *= glm::scale( glm::vec3( x, y, z ) );
	updateMatrix();
}

void RendererGLShader::ortho( float left, float right, float bottom, float top, float zNear,
							  float zFar ) {
	mStack->mCurMatrix->top() *= glm::ortho( left, right, bottom, top, zNear, zFar );
	updateMatrix();
}

void RendererGLShader::lookAt( float eyeX, float eyeY, float eyeZ, float centerX, float centerY,
							   float centerZ, float upX, float upY, float upZ ) {
	mStack->mCurMatrix->top() *=
		glm::lookAt( glm::vec3( eyeX, eyeY, eyeZ ), glm::vec3( centerX, centerY, centerZ ),
					 glm::vec3( upX, upY, upZ ) );
	updateMatrix();
}

void RendererGLShader::perspective( float fovy, float aspect, float zNear, float zFar ) {
	mStack->mCurMatrix->top() *= glm::perspective( fovy, aspect, zNear, zFar );
	updateMatrix();
}

void RendererGLShader::loadMatrixf( const float* m ) {
	mStack->mCurMatrix->top() = toGLMmat4( m );
	updateMatrix();
}

void RendererGLShader::frustum( float left, float right, float bottom, float top, float near_val,
								float far_val ) {
	mStack->mCurMatrix->top() *= glm::frustum( left, right, bottom, top, near_val, far_val );
	updateMatrix();
}

void RendererGLShader::getCurrentMatrix( unsigned int mode, float* m ) {
	switch ( mode ) {
		case GL_PROJECTION:
		case GL_PROJECTION_MATRIX: {
			fromGLMmat4( mStack->mProjectionMatrix.top(), m );
			break;
		}
		case GL_MODELVIEW:
		case GL_MODELVIEW_MATRIX: {
			fromGLMmat4( mStack->mModelViewMatrix.top(), m );
			break;
		}
		case GL_TEXTURE:
		case GL_TEXTURE_MATRIX: {
			fromGLMmat4( mStack->mTextureMatrix.top(), m );
			break;
		}
	}
}

unsigned int RendererGLShader::getCurrentMatrixMode() {
	return mCurrentMode;
}

void RendererGLShader::matrixMode( unsigned int mode ) {
	mCurrentMode = mode;

	switch ( mCurrentMode ) {
		case GL_PROJECTION:
		case GL_PROJECTION_MATRIX: {
			mStack->mCurMatrix = &mStack->mProjectionMatrix;
			break;
		}
		case GL_MODELVIEW:
		case GL_MODELVIEW_MATRIX: {
			mStack->mCurMatrix = &mStack->mModelViewMatrix;
			break;
		}
		case GL_TEXTURE:
		case GL_TEXTURE_MATRIX: {
			mStack->mCurMatrix = &mStack->mTextureMatrix;
			break;
		}
	}
}

int RendererGLShader::project( float objx, float objy, float objz, const float modelMatrix[16],
							   const float projMatrix[16], const int viewport[4], float* winx,
							   float* winy, float* winz ) {
	glm::vec3 tv3( glm::project(
		glm::vec3( objx, objy, objz ), toGLMmat4( modelMatrix ), toGLMmat4( projMatrix ),
		glm::vec4( viewport[0], viewport[1], viewport[2], viewport[3] ) ) );

	if ( NULL != winx )
		*winx = tv3.x;

	if ( NULL != winy )
		*winy = tv3.y;

	if ( NULL != winz )
		*winz = tv3.z;

	return GL_TRUE;
}

int RendererGLShader::unProject( float winx, float winy, float winz, const float modelMatrix[16],
								 const float projMatrix[16], const int viewport[4], float* objx,
								 float* objy, float* objz ) {
	glm::vec3 tv3( glm::unProject(
		glm::vec3( winx, winy, winz ), toGLMmat4( modelMatrix ), toGLMmat4( projMatrix ),
		glm::vec4( viewport[0], viewport[1], viewport[2], viewport[3] ) ) );

	if ( NULL != objx )
		*objx = tv3.x;

	if ( NULL != objy )
		*objy = tv3.y;

	if ( NULL != objz )
		*objz = tv3.z;

	return GL_TRUE;
}

}} // namespace EE::Graphics
