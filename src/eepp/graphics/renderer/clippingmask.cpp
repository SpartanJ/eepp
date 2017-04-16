#include <eepp/graphics/renderer/clippingmask.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/renderer/openglext.hpp>
#include <eepp/graphics/drawable.hpp>
#include <eepp/window/engine.hpp>
#include <algorithm>

using namespace EE::Window;

namespace EE { namespace Graphics {

void ClippingMask::clipEnable( const Int32& x, const Int32& y, const Int32& Width, const Int32& Height ) {
	EE::Window::Window * window = Engine::instance()->getCurrentWindow();
	GlobalBatchRenderer::instance()->draw();

	Rectf r( x, y, x + Width, y + Height );

	if ( !mScissorsClipped.empty() ) {
		Rectf r2 = mScissorsClipped.back();
		r.shrink( r2 );
	}

	GLi->scissor( r.Left, window->getHeight() - r.Bottom, r.getWidth(), r.getHeight() );
	GLi->enable( GL_SCISSOR_TEST );

	if ( mPushScissorClip ) {
		mScissorsClipped.push_back( r );
	}
}

void ClippingMask::clipDisable() {
	GlobalBatchRenderer::instance()->draw();

	if ( ! mScissorsClipped.empty() ) { // This should always be true
		mScissorsClipped.pop_back();
	}

	if ( mScissorsClipped.empty() ) {
		GLi->disable( GL_SCISSOR_TEST );
	} else {
		Rectf R( mScissorsClipped.back() );
		mPushScissorClip = false;
		clipEnable( R.Left, R.Top, R.getWidth(), R.getHeight() );
		mPushScissorClip = true;
	}
}

void ClippingMask::clipPlaneEnable( const Int32& x, const Int32& y, const Int32& Width, const Int32& Height ) {
	GlobalBatchRenderer::instance()->draw();

	Rectf r( x, y, x + Width, y + Height );

	if ( !mPlanesClipped.empty() ) {
		Rectf r2 = mPlanesClipped.back();
		r.shrink( r2 );
	}

	GLi->clip2DPlaneEnable( r.Left, r.Top, r.getWidth(), r.getHeight() );

	if ( mPushClip ) {
		mPlanesClipped.push_back( r );
	}
}

void ClippingMask::clipPlaneDisable() {
	GlobalBatchRenderer::instance()->draw();

	if ( ! mPlanesClipped.empty() ) { // This should always be true
		mPlanesClipped.pop_back();
	}

	if ( mPlanesClipped.empty() ) {
		GLi->clip2DPlaneDisable();
	} else {
		Rectf R( mPlanesClipped.back() );
		mPushClip = false;
		clipPlaneEnable( R.Left, R.Top, R.getWidth(), R.getHeight() );
		mPushClip = true;
	}
}

ClippingMask::ClippingMask() :
	mPushScissorClip( true ),
	mMode( Inclusive )
{
}

std::size_t ClippingMask::getMaskCount() const {
	return mDrawables.size();
}

const Drawable*& ClippingMask::operator [](std::size_t index) {
	return mDrawables[index];
}

const Drawable* const& ClippingMask::operator [](std::size_t index) const {
	return mDrawables[index];
}

void ClippingMask::clearMasks() {
	mDrawables.clear();
}

void ClippingMask::appendMask(const Drawable& drawable) {
	mDrawables.push_back(&drawable);
}

void ClippingMask::removeMask(const Drawable& drawable) {
	mDrawables.erase(std::remove(mDrawables.begin(), mDrawables.end(), &drawable), mDrawables.end());
}

ClippingMask::Mode ClippingMask::getMaskMode() const {
	return mMode;
}

void ClippingMask::setMaskMode(Mode theMode) {
	mMode = theMode;
}

void ClippingMask::stencilMaskEnable() {
	GLi->enable(GL_STENCIL_TEST);
	GLi->stencilMask(0xFF);
	GLi->stencilFunc(GL_NEVER, 1, 0xFF);
	GLi->stencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);

	drawMask();

	GLi->stencilFunc(GL_EQUAL, getMaskMode() == Inclusive ? 1 : 0, 0xFF);
}

void ClippingMask::stencilMaskDisable( bool clearMasks ) {
	GLi->disable(GL_STENCIL_TEST);

	if ( clearMasks )
		this->clearMasks();
}

void ClippingMask::drawMask() {
	for ( std::size_t i = 0; i < getMaskCount(); i++ )
		const_cast<Drawable*>(mDrawables[i])->draw();
}

}}
