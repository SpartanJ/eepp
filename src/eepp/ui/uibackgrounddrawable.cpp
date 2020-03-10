#include <eepp/graphics/vertexbuffer.hpp>
#include <eepp/ui/uibackgrounddrawable.hpp>

namespace EE { namespace UI {

UIBackgroundDrawable* UIBackgroundDrawable::New() {
	return eeNew( UIBackgroundDrawable, () );
}

UIBackgroundDrawable::UIBackgroundDrawable() :
	Drawable( UIBACKGROUNDDRAWABLE ),
	mVertexBuffer( VertexBuffer::New( VERTEX_FLAGS_PRIMITIVE, PRIMITIVE_TRIANGLE_FAN ) ),
	mNeedsUpdate( false ),
	mColorNeedsUpdate( false ) {}

UIBackgroundDrawable::~UIBackgroundDrawable() {
	eeSAFE_DELETE( mVertexBuffer );
}

void UIBackgroundDrawable::draw() {}

void UIBackgroundDrawable::draw( const Vector2f& position ) {}

void UIBackgroundDrawable::draw( const Vector2f& position, const Sizef& size ) {
	if ( mPosition != position ) {
		mPosition = position;
		mNeedsUpdate = true;
	}

	if ( mSize != size ) {
		mSize = size;
		mNeedsUpdate = true;
	}

	if ( mNeedsUpdate || mColorNeedsUpdate ) {
		update();
	}

	mVertexBuffer->bind();
	mVertexBuffer->draw();
	mVertexBuffer->unbind();
}

bool UIBackgroundDrawable::isStateful() {
	return false;
}

const BorderRadiuses& UIBackgroundDrawable::getRadiuses() const {
	return mRadiuses;
}

void UIBackgroundDrawable::setRadiuses( const BorderRadiuses& radiuses ) {
	mRadiuses = radiuses;
	mNeedsUpdate = true;
}

void UIBackgroundDrawable::setRadius( const Uint32& radius ) {
	if ( mRadiuses.topLeft.x != radius ) {
		mRadiuses.topLeft.x = radius;
		mRadiuses.topLeft.y = radius;
		mRadiuses.topRight.x = radius;
		mRadiuses.topRight.y = radius;
		mRadiuses.bottomLeft.x = radius;
		mRadiuses.bottomLeft.y = radius;
		mRadiuses.bottomRight.x = radius;
		mRadiuses.bottomRight.y = radius;
		mNeedsUpdate = true;
	}
}

Int32 UIBackgroundDrawable::getRadius() const {
	return mRadiuses.topLeft.x;
}

void UIBackgroundDrawable::setSize( const Sizef& size ) {
	if ( size != mSize ) {
		mSize = size;
		mNeedsUpdate = true;
	}
}

Sizef UIBackgroundDrawable::getSize() {
	return mSize;
}

void UIBackgroundDrawable::onAlphaChange() {
	mColorNeedsUpdate = true;
}

void UIBackgroundDrawable::onColorFilterChange() {
	mColorNeedsUpdate = true;
}

void UIBackgroundDrawable::onPositionChange() {
	mNeedsUpdate = true;
}

void UIBackgroundDrawable::update() {
	Borders::createBackground( mVertexBuffer, mRadiuses, mPosition, mSize, mColor );
	mColorNeedsUpdate = false;
	mNeedsUpdate = false;
}

}} // namespace EE::UI
