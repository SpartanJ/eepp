#include <eepp/core.hpp>
#include <eepp/graphics/vertexbuffer.hpp>
#include <eepp/ui/uiborderdrawable.hpp>

namespace EE { namespace UI {

UIBorderDrawable* UIBorderDrawable::New() {
	return eeNew( UIBorderDrawable, () );
}

UIBorderDrawable::UIBorderDrawable() :
	Drawable( UIBORDERDRAWABLE ),
	mVertexBuffer( VertexBuffer::New( VERTEX_FLAGS_PRIMITIVE, PRIMITIVE_TRIANGLE_STRIP ) ),
	mBorderType( BorderType::Inside ),
	mNeedsUpdate( false ),
	mColorNeedsUpdate( false ) {}

UIBorderDrawable::~UIBorderDrawable() {
	eeSAFE_DELETE( mVertexBuffer );
}

void UIBorderDrawable::draw() {}

void UIBorderDrawable::draw( const Vector2f& position ) {}

void UIBorderDrawable::draw( const Vector2f& position, const Sizef& size ) {
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

bool UIBorderDrawable::isStateful() {
	return false;
}

void UIBorderDrawable::setLineWidth( const Float& width ) {
	if ( mBorders.top.width != width || mBorders.left.width != width ||
		 mBorders.right.width != width || mBorders.bottom.width != width ) {
		mBorders.top.width = mBorders.left.width = mBorders.right.width = mBorders.bottom.width =
			width;
		mNeedsUpdate = true;
	}
}

Sizef UIBorderDrawable::getSize() {
	return mSize;
}

Float UIBorderDrawable::getLineWidth() const {
	return mBorders.top.width;
}

Int32 UIBorderDrawable::getRadius() const {
	return mBorders.radius.topLeft.x;
}

void UIBorderDrawable::setRadius( const Int32& radius ) {
	if ( mBorders.radius.topLeft.x != radius ) {
		mBorders.radius.topLeft.x = radius;
		mBorders.radius.topLeft.y = radius;
		mBorders.radius.topRight.x = radius;
		mBorders.radius.topRight.y = radius;
		mBorders.radius.bottomLeft.x = radius;
		mBorders.radius.bottomLeft.y = radius;
		mBorders.radius.bottomRight.x = radius;
		mBorders.radius.bottomRight.y = radius;
		mNeedsUpdate = true;
	}
}

Color UIBorderDrawable::getColorLeft() const {
	return mBorders.left.color;
}

void UIBorderDrawable::setColorLeft( const Color& colorLeft ) {
	if ( mBorders.left.color != colorLeft ) {
		mBorders.left.color = colorLeft;
		mColorNeedsUpdate = true;
	}
}

Color UIBorderDrawable::getColorRight() const {
	return mBorders.right.color;
}

void UIBorderDrawable::setColorRight( const Color& colorRight ) {
	if ( mBorders.left.color != colorRight ) {
		mBorders.left.color = colorRight;
		mColorNeedsUpdate = true;
	}
}

Color UIBorderDrawable::getColorTop() const {
	return mBorders.top.color;
}

void UIBorderDrawable::setColorTop( const Color& colorTop ) {
	if ( mBorders.top.color != colorTop ) {
		mBorders.top.color = colorTop;
		mColorNeedsUpdate = true;
	}
}

Color UIBorderDrawable::getColorBottom() const {
	return mBorders.bottom.color;
}

void UIBorderDrawable::setColorBottom( const Color& colorBottom ) {
	if ( mBorders.bottom.color != colorBottom ) {
		mBorders.bottom.color = colorBottom;
		mColorNeedsUpdate = true;
	}
}

const BorderType& UIBorderDrawable::getBorderType() const {
	return mBorderType;
}

void UIBorderDrawable::setBorderType( const BorderType& borderType ) {
	if ( mBorderType != borderType ) {
		mBorderType = borderType;
		mNeedsUpdate = true;
	}
}

void UIBorderDrawable::onAlphaChange() {
	mBorders.left.color.a = mBorders.right.color.a = mBorders.top.color.a =
		mBorders.bottom.color.a = mColor.a;
	mColorNeedsUpdate = true;
}

void UIBorderDrawable::onColorFilterChange() {
	Drawable::onColorFilterChange();
	mBorders.left.color = mBorders.right.color = mBorders.top.color = mBorders.bottom.color =
		mColor;
	mColorNeedsUpdate = true;
}

void UIBorderDrawable::onPositionChange() {
	mNeedsUpdate = true;
}

void UIBorderDrawable::update() {
	switch ( mBorderType ) {
		case BorderType::Outside: {
			Vector2f pos( mPosition );
			Sizef size( mSize );

			if ( mBorders.top.width > 0 ) {
				pos.y -= mBorders.top.width;
			}

			if ( mBorders.left.width > 0 ) {
				pos.x -= mBorders.left.width;
			}

			if ( mBorders.right.width > 0 ) {
				size.x += mBorders.right.width * 2;
			}

			if ( mBorders.bottom.width > 0 ) {
				size.y += mBorders.bottom.width * 2;
			}

			Borders::createBorders( mVertexBuffer, mBorders, pos, size );

			break;
		}
		case BorderType::Inside: {
			Borders::createBorders( mVertexBuffer, mBorders, mPosition, mSize );
			break;
		}
	}

	mColorNeedsUpdate = false;
	mNeedsUpdate = false;
}

}} // namespace EE::UI
