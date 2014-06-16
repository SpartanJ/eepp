#include <eepp/graphics/cframebuffer.hpp>
#include <eepp/graphics/cglobalbatchrenderer.hpp>
#include <eepp/window/cengine.hpp>
#include <eepp/graphics/glextensions.hpp>
#include <eepp/graphics/renderer/cgl.hpp>
#include <eepp/graphics/cframebufferfbo.hpp>
#include <eepp/graphics/cframebuffermanager.hpp>
#include <eepp/window/cwindow.hpp>
using namespace EE::Graphics::Private;

namespace EE { namespace Graphics {

cFrameBuffer * cFrameBuffer::New( const Uint32& Width, const Uint32& Height, bool DepthBuffer, EE::Window::Window * window ) {
	if ( cFrameBufferFBO::IsSupported() )
		return eeNew( cFrameBufferFBO, ( Width, Height, DepthBuffer, window ) );

	return NULL;
}

cFrameBuffer::cFrameBuffer( EE::Window::Window * window  ) :
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

void cFrameBuffer::ClearColor( ColorAf Color ) {
	mClearColor = Color;
}

ColorAf cFrameBuffer::ClearColor() const {
	return mClearColor;
}

void cFrameBuffer::Clear() {
	GLi->ClearColor( mClearColor.R(), mClearColor.G(), mClearColor.B(), mClearColor.A() );
	GLi->Clear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	mWindow->BackColor( mWindow->BackColor() );
}

void cFrameBuffer::SetBufferView() {
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

void cFrameBuffer::RecoverView() {
	cGlobalBatchRenderer::instance()->Draw();

	mWindow->SetView( mPrevView );

	// Recover the user projection matrix
	GLi->LoadIdentity();
	GLi->MatrixMode( GL_PROJECTION );
	GLi->LoadMatrixf( mProjMat );
	GLi->MatrixMode( GL_MODELVIEW );
	GLi->LoadIdentity();
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
