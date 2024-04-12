#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/primitivedrawable.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/vertexbuffer.hpp>

namespace EE { namespace Graphics {

PrimitiveDrawable::PrimitiveDrawable( Type drawableType ) :
	Drawable( drawableType ),
	mFillMode( DRAW_FILL ),
	mBlendMode( BlendMode::Alpha() ),
	mLineWidth( 1.f ),
	mNeedsUpdate( true ),
	mRecreateVertexBuffer( true ),
	mVertexBuffer( NULL ) {}

PrimitiveDrawable::~PrimitiveDrawable() {
	eeSAFE_DELETE( mVertexBuffer );
}

void PrimitiveDrawable::draw( const Vector2f& position, const Sizef& ) {
	if ( mPosition != position ) {
		mPosition = position;
		mNeedsUpdate = true;
	}

	if ( mNeedsUpdate )
		updateVertex();

	if ( NULL != mVertexBuffer ) {
		BatchRenderer* BR = GlobalBatchRenderer::instance();

		bool isPolySmooth = GLi->isPolygonSmooth();
		bool isLineSmooth = GLi->isLineSmooth();

		if ( mSmooth ) {
			if ( !isPolySmooth )
				GLi->polygonSmooth( true );
			if ( !isLineSmooth )
				GLi->lineSmooth( true );
		}

		BR->draw();

		Float lw = BR->getLineWidth();
		BR->setLineWidth( mLineWidth );

		mVertexBuffer->bind();
		mVertexBuffer->draw();
		mVertexBuffer->unbind();

		BR->setLineWidth( lw );

		if ( mSmooth ) {
			if ( !isPolySmooth )
				GLi->polygonSmooth( isPolySmooth );

			if ( !isLineSmooth )
				GLi->lineSmooth( isLineSmooth );
		}
	}
}

void PrimitiveDrawable::setFillMode( const PrimitiveFillMode& Mode ) {
	mFillMode = Mode;
	mNeedsUpdate = true;
	mRecreateVertexBuffer = true;
}

const PrimitiveFillMode& PrimitiveDrawable::getFillMode() const {
	return mFillMode;
}

void PrimitiveDrawable::setBlendMode( const BlendMode& Mode ) {
	mBlendMode = Mode;
}

const BlendMode& PrimitiveDrawable::getBlendMode() const {
	return mBlendMode;
}

void PrimitiveDrawable::setLineWidth( const Float& width ) {
	mLineWidth = width;
}

const Float& PrimitiveDrawable::getLineWidth() const {
	return mLineWidth;
}

bool PrimitiveDrawable::isSmooth() const {
	return mSmooth;
}

void PrimitiveDrawable::setSmooth( bool smooth ) {
	mSmooth = smooth;
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

void PrimitiveDrawable::prepareVertexBuffer( const PrimitiveType& drawableType ) {
	if ( mRecreateVertexBuffer ) {
		eeSAFE_DELETE( mVertexBuffer );
		mVertexBuffer = VertexBuffer::NewVertexArray( VERTEX_FLAGS_PRIMITIVE, drawableType );
		mRecreateVertexBuffer = false;
	}

	mVertexBuffer->clear();
}

}} // namespace EE::Graphics
