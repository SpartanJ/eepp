#include "cframebuffer.hpp"
#include "ctexturefactory.hpp"
#include "../window/cengine.hpp"
#include "cframebufferfbo.hpp"
#include "cframebufferpbuffer.hpp"

namespace EE { namespace Graphics {

cFrameBuffer * cFrameBuffer::CreateNew( const Uint32& Width, const Uint32& Height, bool DepthBuffer ) {
	if ( cFrameBufferFBO::IsSupported() )
		return new cFrameBufferFBO( Width, Height, DepthBuffer );

	if ( cFrameBufferPBuffer::IsSupported() )
		return new cFrameBufferPBuffer( Width, Height, DepthBuffer );

	return NULL;
}

cFrameBuffer::cFrameBuffer() :
	mWidth(0),
	mHeight(0),
	mTexture(NULL),
	mClearColor(0,0,0,0)
{}

cFrameBuffer::~cFrameBuffer() {
	if ( NULL != mTexture )
		cTextureFactory::instance()->Remove( mTexture->TexId() );
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

void cFrameBuffer::SetBufferView() {
	glClearColor( mClearColor.R(), mClearColor.G(), mClearColor.B(), mClearColor.A() );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

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
