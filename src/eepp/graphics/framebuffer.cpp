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
	if ( FrameBufferFBO::IsSupported() )
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
		mWindow = Engine::instance()->GetCurrentWindow();
	}

	FrameBufferManager::instance()->Add( this );
}

FrameBuffer::~FrameBuffer() {
	if ( NULL != mTexture ) {
		eeSAFE_DELETE( mTexture );
	}

	FrameBufferManager::instance()->Remove( this );
}

Texture * FrameBuffer::GetTexture() const {
	return mTexture;
}

void FrameBuffer::ClearColor( ColorAf Color ) {
	mClearColor = Color;
}

ColorAf FrameBuffer::ClearColor() const {
	return mClearColor;
}

void FrameBuffer::Clear() {
	GLi->ClearColor( mClearColor.R(), mClearColor.G(), mClearColor.B(), mClearColor.A() );
	GLi->Clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	mWindow->BackColor( mWindow->BackColor() );
}

void FrameBuffer::SetBufferView() {
	mPrevView = mWindow->GetView();

	// Get the user projection matrix
	GLi->GetCurrentMatrix( GL_PROJECTION_MATRIX, mProjMat );

	GLi->Viewport( 0, 0, mWidth, mHeight );
	GLi->MatrixMode( GL_PROJECTION );
	GLi->LoadIdentity();
	GLi->Ortho( 0.0f, mWidth, 0.f, mHeight, -1000.0f, 1000.0f );
	GLi->MatrixMode( GL_MODELVIEW );
	GLi->LoadIdentity();
}

void FrameBuffer::RecoverView() {
	GlobalBatchRenderer::instance()->Draw();

	mWindow->SetView( mPrevView );

	// Recover the user projection matrix
	GLi->LoadIdentity();
	GLi->MatrixMode( GL_PROJECTION );
	GLi->LoadMatrixf( mProjMat );
	GLi->MatrixMode( GL_MODELVIEW );
	GLi->LoadIdentity();
}

const Int32& FrameBuffer::GetWidth() const {
	return mWidth;
}

const Int32& FrameBuffer::GetHeight() const {
	return mHeight;
}

const bool& FrameBuffer::HasDepthBuffer() const {
	return mHasDepthBuffer;
}

}}
