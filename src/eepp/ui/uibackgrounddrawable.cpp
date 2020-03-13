#include <eepp/graphics/vertexbuffer.hpp>
#include <eepp/ui/uibackgrounddrawable.hpp>
#include <eepp/ui/uinode.hpp>

namespace EE { namespace UI {

UIBackgroundDrawable* UIBackgroundDrawable::New( UINode* owner ) {
	return eeNew( UIBackgroundDrawable, ( owner ) );
}

UIBackgroundDrawable::UIBackgroundDrawable( UINode* owner ) :
	Drawable( UIBACKGROUNDDRAWABLE ),
	mOwner( owner ),
	mVertexBuffer( VertexBuffer::NewVertexArray( VERTEX_FLAGS_PRIMITIVE, PRIMITIVE_TRIANGLE_FAN ) ),
	mNeedsUpdate( false ),
	mColorNeedsUpdate( false ) {}

UIBackgroundDrawable::~UIBackgroundDrawable() {
	eeSAFE_DELETE( mVertexBuffer );
}

void UIBackgroundDrawable::draw() {
	draw( mPosition, mSize );
}

void UIBackgroundDrawable::draw( const Vector2f& position ) {
	draw( position, mSize );
}

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

	// TODO: Optimize rendering for square backgrounds (no radius)
	// It will be cheaper to use the batch renderer for those cases.
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

bool UIBackgroundDrawable::hasRadius() const {
	return !mRadiusesStr.topLeft.empty() || !mRadiusesStr.topRight.empty() ||
		   !mRadiusesStr.bottomLeft.empty() || !mRadiusesStr.bottomRight.empty() ||
		   mRadiuses.topLeft != Sizef::Zero || mRadiuses.topRight != Sizef::Zero ||
		   mRadiuses.bottomLeft != Sizef::Zero || mRadiuses.bottomRight != Sizef::Zero;
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

void UIBackgroundDrawable::setTopLeftRadius( const std::string& radius ) {
	if ( mRadiusesStr.topLeft != radius ) {
		mRadiusesStr.topLeft = radius;
		mNeedsUpdate = true;
	}
}

void UIBackgroundDrawable::setTopRightRadius( const std::string& radius ) {
	if ( mRadiusesStr.topRight != radius ) {
		mRadiusesStr.topRight = radius;
		mNeedsUpdate = true;
	}
}

void UIBackgroundDrawable::setBottomLeftRadius( const std::string& radius ) {
	if ( mRadiusesStr.bottomLeft != radius ) {
		mRadiusesStr.bottomLeft = radius;
		mNeedsUpdate = true;
	}
}

void UIBackgroundDrawable::setBottomRightRadius( const std::string& radius ) {
	if ( mRadiusesStr.bottomRight != radius ) {
		mRadiusesStr.bottomRight = radius;
		mNeedsUpdate = true;
	}
}

Int32 UIBackgroundDrawable::getRadius() const {
	return mRadiuses.topLeft.x;
}

void UIBackgroundDrawable::invalidate() {
	mNeedsUpdate = true;
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
	updateRadiuses();
	Borders::createBackground( mVertexBuffer, mRadiuses, mPosition, mSize, mColor );
	mColorNeedsUpdate = false;
	mNeedsUpdate = false;
}

void UIBackgroundDrawable::updateRadiuses() {
	if ( !mRadiusesStr.topLeft.empty() )
		mRadiuses.topLeft = Borders::radiusFromString( mOwner, mRadiusesStr.topLeft );

	if ( !mRadiusesStr.topRight.empty() )
		mRadiuses.topRight = Borders::radiusFromString( mOwner, mRadiusesStr.topRight );

	if ( !mRadiusesStr.bottomLeft.empty() )
		mRadiuses.bottomLeft = Borders::radiusFromString( mOwner, mRadiusesStr.bottomLeft );

	if ( !mRadiusesStr.bottomRight.empty() )
		mRadiuses.bottomRight = Borders::radiusFromString( mOwner, mRadiusesStr.bottomRight );
}

}} // namespace EE::UI
