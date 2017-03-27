#include <eepp/graphics/primitivedrawable.hpp>

namespace EE { namespace Graphics {

PrimitiveDrawable::PrimitiveDrawable(EE_DRAWABLE_TYPE drawableType) :
	Drawable( drawableType )
{
}

void PrimitiveDrawable::setFillMode( const EE_FILL_MODE& Mode ) {
	mFillMode = Mode;
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

}}
