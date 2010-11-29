#include "cframebuffer.hpp"
#include "ctexturefactory.hpp"
#include "../window/cengine.hpp"
#include "cframebufferfbo.hpp"
#include "cframebufferpbuffer.hpp"
#include "cframebuffermanager.hpp"

using namespace EE::Graphics::Private;

namespace EE { namespace Graphics {

cFrameBuffer * cFrameBuffer::CreateNew( const Uint32& Width, const Uint32& Height, bool DepthBuffer ) {
	if ( cFrameBufferFBO::IsSupported() )
		return eeNew( cFrameBufferFBO, ( Width, Height, DepthBuffer ) );

	if ( cFrameBufferPBuffer::IsSupported() )
		return eeNew( cFrameBufferPBuffer, ( Width, Height, DepthBuffer ) );

	return NULL;
}

cFrameBuffer::cFrameBuffer() :
	mWidth(0),
	mHeight(0),
	mHasDepthBuffer(false),
	mTexture(NULL),
	mClearColor(0,0,0,0)
{
	cFrameBufferManager::instance()->Add( this );
}

cFrameBuffer::~cFrameBuffer() {
	if ( NULL != mTexture )
		cTextureFactory::instance()->Remove( mTexture->Id() );

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
	glClearColor( mClearColor.R(), mClearColor.G(), mClearColor.B(), mClearColor.A() );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

void cFrameBuffer::SetBufferView() {
	mPrevView = Window::cEngine::instance()->GetView();

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glViewport( 0, 0, mWidth, mHeight );
	glOrtho( 0.0f, mWidth, 0.f, mHeight, -1000.0f, 1000.0f );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
}

void cFrameBuffer::RecoverView() {
	cEngine::instance()->SetView( mPrevView );
}

}}
