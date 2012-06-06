#include "cframebuffer.hpp"
#include "ctexturefactory.hpp"
#include "cglobalbatchrenderer.hpp"
#include "../window/cengine.hpp"
#include "cframebufferfbo.hpp"
#include "cframebufferpbuffer.hpp"
#include "cframebuffermanager.hpp"
using namespace EE::Graphics::Private;

namespace EE { namespace Graphics {

cFrameBuffer * cFrameBuffer::CreateNew( const Uint32& Width, const Uint32& Height, bool DepthBuffer, Window::cWindow * window ) {
	if ( cFrameBufferFBO::IsSupported() )
		return eeNew( cFrameBufferFBO, ( Width, Height, DepthBuffer, window ) );

	if ( cFrameBufferPBuffer::IsSupported() )
		return eeNew( cFrameBufferPBuffer, ( Width, Height, DepthBuffer, window ) );

	return NULL;
}

cFrameBuffer::cFrameBuffer( Window::cWindow * window  ) :
	mWindow( window ),
	mWidth(0),
	mHeight(0),
	mHasDepthBuffer(false),
	mTexture(NULL),
	mClearColor(0,0,0,0)
{
	if ( NULL == mWindow ) {
		mWindow = cEngine::instance()->GetCurrentWindow();
	}

	cFrameBufferManager::instance()->Add( this );
}

cFrameBuffer::~cFrameBuffer() {
	if ( NULL != mTexture ) {
		eeSAFE_DELETE( mTexture );
	}

	cFrameBufferManager::instance()->Remove( this );
}

cTexture * cFrameBuffer::GetTexture() const {
	return mTexture;
}

void cFrameBuffer::ClearColor( eeColorAf Color ) {
	mClearColor = Color;
}

eeColorAf cFrameBuffer::ClearColor() const {
	return mClearColor;
}

void cFrameBuffer::Clear() {
	GLi->ClearColor( mClearColor.R(), mClearColor.G(), mClearColor.B(), mClearColor.A() );
	GLi->Clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	mWindow->BackColor( mWindow->BackColor() );
}

void cFrameBuffer::SetBufferView() {
	cGlobalBatchRenderer::instance()->Draw();

	mPrevView = mWindow->GetView();

	GLi->Viewport( 0, 0, mWidth, mHeight );
	GLi->MatrixMode( GL_PROJECTION );
	GLi->LoadIdentity();
	GLi->Ortho( 0.0f, mWidth, 0.f, mHeight, -1000.0f, 1000.0f );
	GLi->MatrixMode( GL_MODELVIEW );
	GLi->LoadIdentity();
}

void cFrameBuffer::RecoverView() {
	cGlobalBatchRenderer::instance()->Draw();

	mWindow->SetView( mPrevView );
}

const Int32& cFrameBuffer::GetWidth() const {
	return mWidth;
}

const Int32& cFrameBuffer::GetHeight() const {
	return mHeight;
}

const bool& cFrameBuffer::HasDepthBuffer() const {
	return mHasDepthBuffer;
}

}}
