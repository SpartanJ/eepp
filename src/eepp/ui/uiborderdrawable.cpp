#include <eepp/core.hpp>
#include <eepp/graphics/vertexbuffer.hpp>
#include <eepp/ui/uiborderdrawable.hpp>
#include <eepp/ui/uinode.hpp>

namespace EE { namespace UI {

UIBorderDrawable* UIBorderDrawable::New( UINode* owner ) {
	return eeNew( UIBorderDrawable, ( owner ) );
}

UIBorderDrawable::UIBorderDrawable( UINode* owner ) :
	Drawable( UIBORDERDRAWABLE ),
	mOwner( owner ),
	mVertexBuffer( VertexBuffer::New( VERTEX_FLAGS_PRIMITIVE, PRIMITIVE_TRIANGLE_STRIP ) ),
	mBorderType( BorderType::Inside ),
	mNeedsUpdate( false ),
	mColorNeedsUpdate( false ),
	mHasBorder( false ) {}

UIBorderDrawable::~UIBorderDrawable() {
	eeSAFE_DELETE( mVertexBuffer );
}

void UIBorderDrawable::draw() {
	draw( mPosition, mSize );
}

void UIBorderDrawable::draw( const Vector2f& position ) {
	draw( position, mSize );
}

void UIBorderDrawable::draw( const Vector2f& position, const Sizef& size ) {
	if ( mPosition != position ) {
		mPosition = position;
		mNeedsUpdate = true;
	}

	if ( mSize != size ) {
		mSize = size;
		mNeedsUpdate = true;
	}

	// TODO: Implement color update.
	if ( mNeedsUpdate || mColorNeedsUpdate ) {
		update();
	}

	if ( mHasBorder ) {
		mVertexBuffer->bind();
		mVertexBuffer->draw();
		mVertexBuffer->unbind();
	}
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
	if ( mBorders.right.color != colorRight ) {
		mBorders.right.color = colorRight;
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

void UIBorderDrawable::invalidate() {
	mNeedsUpdate = true;
}

void UIBorderDrawable::setLeftWidth( const std::string& leftWidth ) {
	if ( mBorderStr.width.left != leftWidth ) {
		mBorderStr.width.left = leftWidth;
		mNeedsUpdate = true;
	}
}

void UIBorderDrawable::setRightWidth( const std::string& rightWidth ) {
	if ( mBorderStr.width.right != rightWidth ) {
		mBorderStr.width.right = rightWidth;
		mNeedsUpdate = true;
	}
}

void UIBorderDrawable::setTopWidth( const std::string& topWidth ) {
	if ( mBorderStr.width.top != topWidth ) {
		mBorderStr.width.top = topWidth;
		mNeedsUpdate = true;
	}
}

void UIBorderDrawable::setBottomWidth( const std::string& bottomWidth ) {
	if ( mBorderStr.width.bottom != bottomWidth ) {
		mBorderStr.width.bottom = bottomWidth;
		mNeedsUpdate = true;
	}
}

void UIBorderDrawable::setTopLeftRadius( const std::string& radius ) {
	if ( mBorderStr.radius.topLeft != radius ) {
		mBorderStr.radius.topLeft = radius;
		mNeedsUpdate = true;
	}
}

void UIBorderDrawable::setTopRightRadius( const std::string& radius ) {
	if ( mBorderStr.radius.topRight != radius ) {
		mBorderStr.radius.topRight = radius;
		mNeedsUpdate = true;
	}
}

void UIBorderDrawable::setBottomLeftRadius( const std::string& radius ) {
	if ( mBorderStr.radius.bottomLeft != radius ) {
		mBorderStr.radius.bottomLeft = radius;
		mNeedsUpdate = true;
	}
}

void UIBorderDrawable::setBottomRightRadius( const std::string& radius ) {
	if ( mBorderStr.radius.bottomRight != radius ) {
		mBorderStr.radius.bottomRight = radius;
		mNeedsUpdate = true;
	}
}

const Borders& UIBorderDrawable::getBorders() const {
	return mBorders;
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
	updateBorders();

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
		case BorderType::Outline: {
			Vector2f pos( mPosition );
			Sizef size( mSize );

			if ( mBorders.top.width > 0 ) {
				pos.y -= mBorders.top.width * 0.5f;
			}

			if ( mBorders.left.width > 0 ) {
				pos.x -= mBorders.left.width * 0.5f;
			}

			if ( mBorders.right.width > 0 ) {
				size.x += mBorders.right.width;
			}

			if ( mBorders.bottom.width > 0 ) {
				size.y += mBorders.bottom.width;
			}

			Borders::createBorders( mVertexBuffer, mBorders, pos, size );

			break;
		}
	}

	mColorNeedsUpdate = false;
	mNeedsUpdate = false;
}

void UIBorderDrawable::updateBorders() {
	if ( !mBorderStr.width.left.empty() ) {
		mBorders.left.width = mOwner->lengthFromValue(
			mBorderStr.width.left, CSS::PropertyRelativeTarget::LocalBlockRadiusWidth );
	}

	if ( !mBorderStr.width.right.empty() ) {
		mBorders.right.width = mOwner->lengthFromValue(
			mBorderStr.width.right, CSS::PropertyRelativeTarget::LocalBlockRadiusWidth );
	}

	if ( !mBorderStr.width.top.empty() ) {
		mBorders.top.width = mOwner->lengthFromValue(
			mBorderStr.width.top, CSS::PropertyRelativeTarget::LocalBlockRadiusHeight );
	}

	if ( !mBorderStr.width.bottom.empty() ) {
		mBorders.bottom.width = mOwner->lengthFromValue(
			mBorderStr.width.bottom, CSS::PropertyRelativeTarget::LocalBlockRadiusHeight );
	}

	if ( !mBorderStr.radius.topLeft.empty() )
		mBorders.radius.topLeft = Borders::radiusFromString( mOwner, mBorderStr.radius.topLeft );

	if ( !mBorderStr.radius.topRight.empty() )
		mBorders.radius.topRight = Borders::radiusFromString( mOwner, mBorderStr.radius.topRight );

	if ( !mBorderStr.radius.bottomLeft.empty() )
		mBorders.radius.bottomLeft =
			Borders::radiusFromString( mOwner, mBorderStr.radius.bottomLeft );

	if ( !mBorderStr.radius.bottomRight.empty() )
		mBorders.radius.bottomRight =
			Borders::radiusFromString( mOwner, mBorderStr.radius.bottomRight );

	mHasBorder = mBorders.top.width > 0 || mBorders.right.width > 0 || mBorders.bottom.width > 0 ||
				 mBorders.left.width > 0;
}

}} // namespace EE::UI
