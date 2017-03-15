#include <eepp/graphics/framebuffer.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/graphics/glextensions.hpp>
#include <eepp/graphics/renderer/gl.hpp>
#include <eepp/graphics/framebufferfbo.hpp>
#include <eepp/graphics/framebuffermanager.hpp>
#include <eepp/window/window.hpp>
using namespace EE::Graphics::Private;

namespace EE { namespace Graphics {

FrameBuffer * FrameBuffer::New( const Uint32& Width, const Uint32& Height, bool DepthBuffer, EE::Window::Window * window ) {
	if ( FrameBufferFBO::isSupported() )
		return eeNew( FrameBufferFBO, ( Width, Height, DepthBuffer, window ) );

	return NULL;
}

FrameBuffer::FrameBuffer( EE::Window::Window * window  ) :
	mWindow( window ),
	mWidth(0),
	mHeight(0),
	mHasDepthBuffer(false),
	mTexture(NULL),
	mClearColor(0,0,0,0)
{
	if ( NULL == mWindow ) {
		mWindow = Engine::instance()->getCurrentWindow();
	}

	FrameBufferManager::instance()->add( this );
}

FrameBuffer::~FrameBuffer() {
	if ( NULL != mTexture ) {
		eeSAFE_DELETE( mTexture );
	}

	FrameBufferManager::instance()->remove( this );
}

Texture * FrameBuffer::getTexture() const {
	return mTexture;
}

void FrameBuffer::setClearColor( ColorAf Color ) {
	mClearColor = Color;
}

ColorAf FrameBuffer::getClearColor() const {
	return mClearColor;
}

void FrameBuffer::clear() {
	GLi->clearColor( mClearColor.r(), mClearColor.g(), mClearColor.b(), mClearColor.a() );
	GLi->clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	mWindow->setClearColor( mWindow->getClearColor() );
}

void FrameBuffer::setBufferView() {
	mPrevView = mWindow->getView();

	// Get the user projection matrix
	GLi->getCurrentMatrix( GL_PROJECTION_MATRIX, mProjMat );

	GLi->viewport( 0, 0, mWidth, mHeight );
	GLi->matrixMode( GL_PROJECTION );
	GLi->loadIdentity();
	GLi->ortho( 0.0f, mWidth, 0.f, mHeight, -1000.0f, 1000.0f );
	GLi->matrixMode( GL_MODELVIEW );
	GLi->loadIdentity();
}

void FrameBuffer::recoverView() {
	GlobalBatchRenderer::instance()->draw();

	mWindow->setView( mPrevView );

	// Recover the user projection matrix
	GLi->loadIdentity();
	GLi->matrixMode( GL_PROJECTION );
	GLi->loadMatrixf( mProjMat );
	GLi->matrixMode( GL_MODELVIEW );
	GLi->loadIdentity();
}

const Int32& FrameBuffer::getWidth() const {
	return mWidth;
}

const Int32& FrameBuffer::getHeight() const {
	return mHeight;
}

const bool& FrameBuffer::hasDepthBuffer() const {
	return mHasDepthBuffer;
}

}}
