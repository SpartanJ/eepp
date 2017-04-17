#include <eepp/graphics/primitivedrawable.hpp>
#include <eepp/graphics/vertexbuffer.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>

namespace EE { namespace Graphics {

PrimitiveDrawable::PrimitiveDrawable(EE_DRAWABLE_TYPE drawableType) :
	Drawable( drawableType ),
	mFillMode( DRAW_FILL ),
	mBlendMode( ALPHA_NORMAL ),
	mLineWidth( 1.f ),
	mNeedsUpdate( true ),
	mRecreateVertexBuffer( true ),
	mVertexBuffer( NULL )
{
}

PrimitiveDrawable::~PrimitiveDrawable() {
	eeSAFE_DELETE( mVertexBuffer );
}

void PrimitiveDrawable::draw( const Vector2f & position, const Sizef& size ) {
	if ( mPosition != position ) {
		mPosition = position;
		mNeedsUpdate = true;
	}

	if ( mNeedsUpdate )
		updateVertex();

	if ( NULL != mVertexBuffer ) {
		BatchRenderer * BR = GlobalBatchRenderer::instance();

		BR->draw();

		Float lw = BR->getLineWidth();
		BR->setLineWidth( mLineWidth );

		mVertexBuffer->bind();
		mVertexBuffer->draw();
		mVertexBuffer->unbind();

		BR->setLineWidth( lw );
	}
}

void PrimitiveDrawable::setFillMode(const EE_FILL_MODE & Mode) {
	mFillMode = Mode;
	mNeedsUpdate = true;
	mRecreateVertexBuffer = true;
}

const EE_FILL_MODE& PrimitiveDrawable::getFillMode() const {
	return mFillMode;
}

void PrimitiveDrawable::setBlendMode( const EE_BLEND_MODE& Mode ) {
	mBlendMode = Mode;
}

const EE_BLEND_MODE& PrimitiveDrawable::getBlendMode() const {
	return mBlendMode;
}

void PrimitiveDrawable::setLineWidth( const Float& width ) {
	mLineWidth = width;
}

const Float& PrimitiveDrawable::getLineWidth() const {
	return mLineWidth;
}

void PrimitiveDrawable::onAlphaChange() {
	mNeedsUpdate = true;
}

void PrimitiveDrawable::onColorFilterChange() {
	mNeedsUpdate = true;
}

void PrimitiveDrawable::onPositionChange() {
	mNeedsUpdate = true;
}

void PrimitiveDrawable::prepareVertexBuffer( const EE_DRAW_MODE& drawableType ) {
	if ( mRecreateVertexBuffer ) {
		eeSAFE_DELETE( mVertexBuffer );
		mVertexBuffer = VertexBuffer::NewVertexArray( VERTEX_FLAGS_PRIMITIVE, drawableType );
		mRecreateVertexBuffer = false;
	}

	mVertexBuffer->clear();
}

}}
