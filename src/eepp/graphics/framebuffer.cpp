#include <algorithm>
#include <eepp/graphics/framebuffer.hpp>
#include <eepp/graphics/framebufferfbo.hpp>
#include <eepp/graphics/framebuffermanager.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/renderer/opengl.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/window.hpp>
using namespace EE::Graphics::Private;

namespace EE { namespace Graphics {

static std::vector<const View*> sFBOActiveViews;

FrameBuffer* FrameBuffer::New( const Uint32& Width, const Uint32& Height, bool StencilBuffer,
							   bool DepthBuffer, bool useColorBuffer, const Uint32& channels,
							   EE::Window::Window* window ) {
	if ( FrameBufferFBO::isSupported() )
		return eeNew( FrameBufferFBO, ( Width, Height, StencilBuffer, DepthBuffer, useColorBuffer,
										channels, window ) );
	Log::warning( "FBO not supported" );
	return NULL;
}

FrameBuffer::FrameBuffer( EE::Window::Window* window ) :
	mWindow( window ),
	mSize( 0, 0 ),
	mChannels( 4 ),
	mId( 0 ),
	mHasColorBuffer( false ),
	mHasDepthBuffer( false ),
	mHasStencilBuffer( false ),
	mTexture( NULL ),
	mClearColor( 0, 0, 0, 0 ) {
	if ( NULL == mWindow ) {
		mWindow = Engine::instance()->getCurrentWindow();
	}

	FrameBufferManager::instance()->add( this );
}

FrameBuffer::~FrameBuffer() {
	eeSAFE_DELETE( mTexture );

	FrameBufferManager::instance()->remove( this );
}

Texture* FrameBuffer::getTexture() const {
	return mTexture;
}

void FrameBuffer::setClearColor( const ColorAf& color ) {
	mClearColor.assign( color );
}

ColorAf FrameBuffer::getClearColor() const {
	return mClearColor;
}

void FrameBuffer::clear() {
	GLi->clearColor( mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a );
	GLi->clear( GL_COLOR_BUFFER_BIT | ( mHasDepthBuffer ? GL_DEPTH_BUFFER_BIT : 0 ) );
	mWindow->setClearColor( mWindow->getClearColor() );
}

void FrameBuffer::setBufferView() {
	// Get the user projection and modelview matrix
	GLi->getCurrentMatrix( GL_PROJECTION_MATRIX, mProjMat );
	GLi->getCurrentMatrix( GL_MODELVIEW_MATRIX, mModelViewMat );

	mView.reset( Rectf( 0, 0, mSize.getWidth(), mSize.getHeight() ) );
	sFBOActiveViews.push_back( &mView );

	GLi->viewport( 0, 0, mSize.getWidth(), mSize.getHeight() );
	GLi->matrixMode( GL_PROJECTION );
	GLi->loadIdentity();
	GLi->ortho( 0.0f, mSize.getWidth(), 0.f, mSize.getHeight(), -1000.0f, 1000.0f );
	GLi->matrixMode( GL_MODELVIEW );
	GLi->loadIdentity();

	auto cm = GLi->getClippingMask();
	if ( mAdjustCurrentClipping && cm->isScissorsClipEnabled() ) {
		mNeedsToRestoreScissorsClipping = true;
		mOldScissorsRect = cm->getScissorsClipped();
		Rectf currentClip = mOldScissorsRect.back();
		Rectf fboScreenRect( mPosition.floor(), mSize.asFloat() );
		Rectf newClipRect( fboScreenRect.getPosition() - currentClip.getPosition(),
						   currentClip.getSize() );
		cm->setScissorsClipped( { newClipRect } );
	}
}

void FrameBuffer::recoverView() {
	GlobalBatchRenderer::instance()->draw();

	auto found = std::find( sFBOActiveViews.begin(), sFBOActiveViews.end(), &mView );
	if ( found != sFBOActiveViews.end() )
		sFBOActiveViews.erase( found );

	if ( sFBOActiveViews.empty() ) {
		mWindow->setView( mWindow->getView(), true );
	} else {
		const View* view = sFBOActiveViews.back();
		GLi->viewport( 0, 0, view->getSize().getWidth(), view->getSize().getHeight() );
	}

	// Recover the user projection and modelview matrix
	GLi->loadIdentity();
	GLi->matrixMode( GL_PROJECTION );
	GLi->loadMatrixf( mProjMat );
	GLi->matrixMode( GL_MODELVIEW );
	GLi->loadIdentity();
	GLi->loadMatrixf( mModelViewMat );

	if ( mNeedsToRestoreScissorsClipping ) {
		GLi->getClippingMask()->setScissorsClipped( mOldScissorsRect );
		mNeedsToRestoreScissorsClipping = false;
	}
}

const Int32& FrameBuffer::getWidth() const {
	return mSize.x;
}

const Int32& FrameBuffer::getHeight() const {
	return mSize.y;
}

const Sizei& FrameBuffer::getSize() const {
	return mSize;
}

const Sizef FrameBuffer::getSizef() {
	return Sizef( mSize.getWidth(), mSize.getHeight() );
}

const bool& FrameBuffer::hasColorBuffer() const {
	return mHasColorBuffer;
}

const bool& FrameBuffer::hasDepthBuffer() const {
	return mHasDepthBuffer;
}

const bool& FrameBuffer::hasStencilBuffer() const {
	return mHasStencilBuffer;
}

const std::string& FrameBuffer::getName() const {
	return mName;
}

void FrameBuffer::setName( const std::string& name ) {
	mName = name;
	mId = String::hash( mName );
}

const String::HashType& FrameBuffer::getId() const {
	return mId;
}

}} // namespace EE::Graphics
