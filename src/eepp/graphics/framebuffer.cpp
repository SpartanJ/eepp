#include <eepp/graphics/framebuffer.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/graphics/renderer/opengl.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/framebufferfbo.hpp>
#include <eepp/graphics/framebuffermanager.hpp>
#include <eepp/window/window.hpp>
using namespace EE::Graphics::Private;

namespace EE { namespace Graphics {

static std::list<const View*> sFBOActiveViews;

FrameBuffer * FrameBuffer::New(const Uint32& Width, const Uint32& Height, bool StencilBuffer, bool DepthBuffer, const Uint32& channels, EE::Window::Window * window ) {
	if ( FrameBufferFBO::isSupported() )
		return eeNew( FrameBufferFBO, ( Width, Height, StencilBuffer, DepthBuffer, channels, window ) );
	eePRINTL( "FBO not supported" );
	return NULL;
}

FrameBuffer::FrameBuffer( EE::Window::Window * window  ) :
	mWindow( window ),
	mSize(0,0),
	mChannels(4),
	mHasDepthBuffer(false),
	mHasStencilBuffer(false),
	mTexture(NULL),
	mClearColor(0,0,0,0)
{
	if ( NULL == mWindow ) {
		mWindow = Engine::instance()->getCurrentWindow();
	}

	FrameBufferManager::instance()->add( this );
}

FrameBuffer::~FrameBuffer() {
	eeSAFE_DELETE( mTexture );

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
	GLi->clearColor( mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a );
	GLi->clear( GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	mWindow->setClearColor( mWindow->getClearColor() );
}

void FrameBuffer::setBufferView() {
	// Get the user projection matrix
	GLi->getCurrentMatrix( GL_PROJECTION_MATRIX, mProjMat );

	mView.setSize( mSize.getWidth(), mSize.getHeight() );
	sFBOActiveViews.push_back(&mView);

	GLi->viewport( 0, 0, mSize.getWidth(), mSize.getHeight() );
	GLi->matrixMode( GL_PROJECTION );
	GLi->loadIdentity();
	GLi->ortho( 0.0f, mSize.getWidth(), 0.f, mSize.getHeight(), -1000.0f, 1000.0f );
	GLi->matrixMode( GL_MODELVIEW );
	GLi->loadIdentity();
}

void FrameBuffer::recoverView() {
	GlobalBatchRenderer::instance()->draw();

	sFBOActiveViews.remove(&mView);

	if ( sFBOActiveViews.empty() ) {
		mWindow->setView( mWindow->getView() );
	} else {
		const View* view = sFBOActiveViews.back();
		GLi->viewport( 0, 0, view->getView().getWidth(), view->getView().getHeight() );
	}

	// Recover the user projection matrix
	GLi->loadIdentity();
	GLi->matrixMode( GL_PROJECTION );
	GLi->loadMatrixf( mProjMat );
	GLi->matrixMode( GL_MODELVIEW );
	GLi->loadIdentity();
}

const Int32& FrameBuffer::getWidth() const {
	return mSize.x;
}

const Int32& FrameBuffer::getHeight() const {
	return mSize.y;
}

const Sizef FrameBuffer::getSizef() {
	return Sizef( mSize.getWidth(), mSize.getHeight() );
}

const bool& FrameBuffer::hasDepthBuffer() const {
	return mHasDepthBuffer;
}

const bool &FrameBuffer::hasStencilBuffer() const {
	return mHasStencilBuffer;
}

}}
