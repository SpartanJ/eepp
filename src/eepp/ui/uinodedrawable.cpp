#include <eepp/core/core.hpp>
#include <eepp/graphics/drawableresource.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/math/easing.hpp>
#include <eepp/ui/css/stylesheetlength.hpp>
#include <eepp/ui/uinode.hpp>
#include <eepp/ui/uinodedrawable.hpp>
using namespace EE::Math::easing;

namespace EE { namespace UI {

UINodeDrawable::Repeat UINodeDrawable::repeatFromText( const std::string& text ) {
	if ( "repeat" == text )
		return UINodeDrawable::Repeat::RepeatXY;
	if ( "repeat-x" == text )
		return UINodeDrawable::Repeat::RepeatX;
	if ( "repeat-y" == text )
		return UINodeDrawable::Repeat::RepeatY;
	return UINodeDrawable::Repeat::NoRepeat;
}

UINodeDrawable* UINodeDrawable::New( UINode* owner ) {
	return eeNew( UINodeDrawable, ( owner ) );
}

UINodeDrawable::UINodeDrawable( UINode* owner ) :
	Drawable( Drawable::UINODEDRAWABLE ),
	mOwner( owner ),
	mNeedsUpdate( true ),
	mClipEnabled( false ) {
	mBackgroundColor.setColor( Color::Transparent );
}

UINodeDrawable::~UINodeDrawable() {
	clearDrawables();
}

void UINodeDrawable::clearDrawables() {
	for ( auto& drawable : mGroup ) {
		eeDelete( drawable.second );
	}

	mGroup.clear();
	mBackgroundColor.setColor( Color::Transparent );
}

void UINodeDrawable::setBorderRadius( const Uint32& corners ) {
	mBackgroundColor.setCorners( corners );
}

Uint32 UINodeDrawable::getBorderRadius() const {
	return mBackgroundColor.getCorners();
}

bool UINodeDrawable::layerExists( int index ) {
	return mGroup.find( index ) != mGroup.end();
}

UINodeDrawable::LayerDrawable* UINodeDrawable::getLayer( int index ) {
	auto it = mGroup.find( index );

	if ( it == mGroup.end() )
		mGroup[index] = UINodeDrawable::LayerDrawable::New( this );

	return mGroup[index];
}

void UINodeDrawable::setDrawable( int index, Drawable* drawable, bool ownIt ) {
	if ( drawable != getLayer( index )->getDrawable() ) {
		getLayer( index )->setDrawable( drawable, ownIt );
	}
}

void UINodeDrawable::setDrawablePositionX( int index, const std::string& positionX ) {
	getLayer( index )->setPositionX( positionX );
}

void UINodeDrawable::setDrawablePositionY( int index, const std::string& positionY ) {
	getLayer( index )->setPositionY( positionY );
}

void UINodeDrawable::setDrawableRepeat( int index, const std::string& repeatRule ) {
	getLayer( index )->setRepeat( repeatFromText( repeatRule ) );

	for ( auto& layIt : mGroup ) {
		if ( layIt.second->getRepeat() != Repeat::NoRepeat ) {
			setClipEnabled( true );
			break;
		}
	}
}

void UINodeDrawable::setDrawableSize( int index, const std::string& sizeEq ) {
	getLayer( index )->setSizeEq( sizeEq );
}

void UINodeDrawable::setBackgroundColor( const Color& color ) {
	mBackgroundColor.setColor( color );
}

Color UINodeDrawable::getBackgroundColor() const {
	return mBackgroundColor.getColor();
}

bool UINodeDrawable::getClipEnabled() const {
	return mClipEnabled;
}

void UINodeDrawable::setClipEnabled( bool clipEnabled ) {
	mClipEnabled = clipEnabled;
}

void UINodeDrawable::invalidate() {
	mNeedsUpdate = true;
}

Sizef UINodeDrawable::getSize() {
	return mSize;
}

void UINodeDrawable::setSize( const Sizef& size ) {
	if ( size != mSize ) {
		mSize = size;
		onSizeChange();
	}
}

void UINodeDrawable::draw( const Vector2f& position, const Sizef& size ) {
	draw( position, size, 255 );
}

void UINodeDrawable::draw( const Vector2f& position, const Sizef& size, const Uint32& alpha ) {
	if ( position != mPosition ) {
		mPosition = position;
		invalidate();
	}

	if ( size != mSize ) {
		mSize = size;
		invalidate();
	}

	if ( mNeedsUpdate )
		update();

	if ( mClipEnabled )
		GLi->getClippingMask()->clipPlaneEnable( mPosition.x, mPosition.y, mSize.x, mSize.y );

	if ( mBackgroundColor.getColor().a != 0 ) {
		if ( alpha != 255 ) {
			Color color = mBackgroundColor.getColor();
			mBackgroundColor.setAlpha( alpha * color.a / 255 );
			mBackgroundColor.draw( position, size );
			mBackgroundColor.setAlpha( color.a );

		} else {
			mBackgroundColor.draw( position, size );
		}
	}

	for ( auto& drawableIt : mGroup ) {
		UINodeDrawable::LayerDrawable* drawable = drawableIt.second;

		if ( alpha != 255 ) {
			Color color = drawable->getColor();
			drawable->setAlpha( alpha * color.a / 255 );
			drawable->draw( position, size );
			drawable->setAlpha( color.a );
		} else {
			drawable->draw( position, size );
		}
	}

	if ( mClipEnabled )
		GLi->getClippingMask()->clipPlaneDisable();
}

void UINodeDrawable::draw( const Vector2f& position ) {
	draw( position, mSize );
}

void UINodeDrawable::draw() {
	draw( mPosition, mSize );
}

void UINodeDrawable::onPositionChange() {
	invalidate();
}

void UINodeDrawable::onSizeChange() {
	invalidate();
}

void UINodeDrawable::update() {
	mBackgroundColor.setPosition( mPosition );
	mBackgroundColor.setSize( mSize );
	mNeedsUpdate = false;
}

/**** UINodeDrawable::LayerDrawable ****/

UINodeDrawable::LayerDrawable* UINodeDrawable::LayerDrawable::New( UINodeDrawable* container ) {
	return eeNew( UINodeDrawable::LayerDrawable, ( container ) );
}

UINodeDrawable::LayerDrawable::LayerDrawable( UINodeDrawable* container ) :
	Drawable( UINODEDRAWABLE_LAYERDRAWABLE ),
	mContainer( container ),
	mPositionX( "0px" ),
	mPositionY( "0px" ),
	mSizeEq( "auto" ),
	mNeedsUpdate( false ),
	mOwnsDrawable( false ),
	mDrawable( NULL ),
	mResourceChangeCbId( 0 ),
	mRepeat( Repeat::NoRepeat ) {}

UINodeDrawable::LayerDrawable::~LayerDrawable() {
	if ( NULL != mDrawable && 0 != mResourceChangeCbId && mDrawable->isDrawableResource() ) {
		reinterpret_cast<DrawableResource*>( mDrawable )
			->popResourceChangeCallback( mResourceChangeCbId );
	}

	if ( mOwnsDrawable )
		eeSAFE_DELETE( mDrawable );
}

void UINodeDrawable::LayerDrawable::draw() {
	draw( mPosition, mSize );
}

void UINodeDrawable::LayerDrawable::draw( const Vector2f& position ) {
	draw( position, mSize );
}

static void repeatYdraw( Drawable* drawable, const Vector2f& position, const Vector2f& offset,
						 const Sizef& size, const Sizef& drawableSize ) {
	Float startY = position.y + offset.y - drawableSize.getHeight();
	while ( startY > position.y - drawableSize.getHeight() ) {
		drawable->draw( Vector2f( position.x + offset.x, startY ), drawableSize );
		startY -= drawableSize.getHeight();
	};
	drawable->draw( position + offset, drawableSize );
	startY = position.y + offset.y + drawableSize.getHeight();
	while ( startY < position.y + size.getHeight() ) {
		drawable->draw( Vector2f( position.x + offset.x, startY ), drawableSize );
		startY += drawableSize.getHeight();
	};
}

void UINodeDrawable::LayerDrawable::draw( const Vector2f& position, const Sizef& size ) {
	if ( position != mPosition ) {
		mPosition = position;
		invalidate();
	}

	if ( size != mSize ) {
		mSize = size;
		invalidate();
	}

	if ( mDrawable == NULL )
		return;

	if ( mNeedsUpdate )
		update();

	switch ( mRepeat ) {
		case Repeat::NoRepeat:
			mDrawable->draw( mPosition + mOffset, mDrawableSize );
			break;
		case Repeat::RepeatX: {
			Float startX = mPosition.x + mOffset.x - mDrawableSize.getWidth();
			while ( startX > mPosition.x - mDrawableSize.getWidth() ) {
				mDrawable->draw( Vector2f( startX, mPosition.y + mOffset.y ), mDrawableSize );
				startX -= mDrawableSize.getWidth();
			};
			mDrawable->draw( mPosition + mOffset, mDrawableSize );
			startX = mPosition.x + mOffset.x + mDrawableSize.getWidth();
			while ( startX < mPosition.x + mSize.getWidth() ) {
				mDrawable->draw( Vector2f( startX, mPosition.y + mOffset.y ), mDrawableSize );
				startX += mDrawableSize.getWidth();
			};
			break;
		}
		case Repeat::RepeatY: {
			repeatYdraw( mDrawable, mPosition, mOffset, mSize, mDrawableSize );
			break;
		}
		case Repeat::RepeatXY: {
			Float startX = mPosition.x + mOffset.x - mDrawableSize.getWidth();
			while ( startX > mPosition.x - mDrawableSize.getWidth() ) {
				repeatYdraw( mDrawable, mPosition, Vector2f( startX - mPosition.x, mOffset.y ), mSize,
							 mDrawableSize );
				startX -= mDrawableSize.getWidth();
			};
			repeatYdraw( mDrawable, mPosition, mOffset, mSize, mDrawableSize );
			startX = mPosition.x + mOffset.x + mDrawableSize.getWidth();
			while ( startX < mPosition.x + mSize.getWidth() ) {
				repeatYdraw( mDrawable, mPosition, Vector2f( startX - mPosition.x, mOffset.y ), mSize,
							 mDrawableSize );
				startX += mDrawableSize.getWidth();
			};
			break;
		}
	}
}

Sizef UINodeDrawable::LayerDrawable::getSize() {
	return mSize;
}

void UINodeDrawable::LayerDrawable::setSize( const Sizef& size ) {
	if ( size != mSize ) {
		mSize = size;
		invalidate();
	}
}

Drawable* UINodeDrawable::LayerDrawable::getDrawable() const {
	return mDrawable;
}

void UINodeDrawable::LayerDrawable::setDrawable( Drawable* drawable, const bool& ownIt ) {
	if ( drawable == mDrawable )
		return;

	if ( NULL != mDrawable ) {
		if ( mDrawable->isDrawableResource() ) {
			reinterpret_cast<DrawableResource*>( mDrawable )
				->popResourceChangeCallback( mResourceChangeCbId );
		}

		if ( mOwnsDrawable ) {
			eeSAFE_DELETE( mDrawable );
		}
	}

	mDrawable = drawable;
	mOwnsDrawable = ownIt;
	invalidate();

	if ( mDrawable->isDrawableResource() ) {
		mResourceChangeCbId = reinterpret_cast<DrawableResource*>( mDrawable )
								  ->pushResourceChangeCallback(
									  [&]( DrawableResource::Event event, DrawableResource* ) {
										  invalidate();
										  if ( event == DrawableResource::Event::Unload ) {
											  mResourceChangeCbId = 0;
											  mDrawable = NULL;
											  mOwnsDrawable = false;
										  }
									  } );
	}
}

const Vector2f& UINodeDrawable::LayerDrawable::getOffset() const {
	if ( mNeedsUpdate )
		const_cast<LayerDrawable*>( this )->update();

	return mOffset;
}

const UINodeDrawable::Repeat& UINodeDrawable::LayerDrawable::getRepeat() const {
	return mRepeat;
}

void UINodeDrawable::LayerDrawable::setRepeat( const UINodeDrawable::Repeat& repeat ) {
	if ( mRepeat != repeat ) {
		mRepeat = repeat;
		invalidate();
	}
}

void UINodeDrawable::LayerDrawable::invalidate() {
	mNeedsUpdate = true;
}

const Sizef& UINodeDrawable::LayerDrawable::getDrawableSize() const {
	if ( mNeedsUpdate )
		const_cast<LayerDrawable*>( this )->update();

	return mDrawableSize;
}

void UINodeDrawable::LayerDrawable::setDrawableSize( const Sizef& drawableSize ) {
	mDrawableSize = drawableSize;
}

Sizef UINodeDrawable::LayerDrawable::calcDrawableSize( const std::string& drawableSizeEq ) {
	Sizef size;

	if ( drawableSizeEq == "auto" ) {
		if ( mDrawable->getDrawableType() == Drawable::RECTANGLE ) {
			size = mSize;
		} else {
			size = mDrawable->getSize();
		}
	} else if ( drawableSizeEq == "expand" ) {
		size = mSize;
	} else if ( drawableSizeEq == "contain" ) {
		Sizef drawableSize( mDrawable->getSize() );
		Float Scale1 = mSize.getWidth() / drawableSize.getWidth();
		Float Scale2 = mSize.getHeight() / drawableSize.getHeight();
		if ( Scale1 < 1 || Scale2 < 1 ) {
			Scale1 = eemin( Scale1, Scale2 );
			size = Sizef( drawableSize.getWidth() * Scale1, drawableSize.getHeight() * Scale1 );
		} else {
			size = drawableSize;
		}
	} else if ( drawableSizeEq == "cover" ) {
		Sizef drawableSize( mDrawable->getSize() );
		Float Scale1 = mSize.getWidth() / drawableSize.getWidth();
		Float Scale2 = mSize.getHeight() / drawableSize.getHeight();
		Scale1 = eemax( Scale1, Scale2 );
		size = Sizef( drawableSize.getWidth() * Scale1, drawableSize.getHeight() * Scale1 );
	} else {
		std::vector<std::string> sizePart = String::split( drawableSizeEq, ' ' );

		if ( sizePart.size() == 1 ) {
			sizePart.push_back( "auto" );
		}

		if ( sizePart.size() == 2 ) {
			if ( sizePart[0] == "auto" && sizePart[1] == "auto" ) {
				if ( mDrawable->getDrawableType() == Drawable::RECTANGLE ) {
					size = mSize;
				} else {
					size = mDrawable->getSize();
				}
			} else if ( sizePart[0] != "auto" ) {
				CSS::StyleSheetLength wl( sizePart[0] );
				size.x = mContainer->getOwner()->convertLength(
					wl, mContainer->getOwner()->getPixelsSize().getWidth() );

				if ( sizePart[1] == "auto" ) {
					Sizef drawableSize( mDrawable->getSize() );
					size.y =
						drawableSize.getHeight() * ( size.getWidth() / drawableSize.getWidth() );
				} else {
					CSS::StyleSheetLength hl( sizePart[1] );
					size.y = mContainer->getOwner()->convertLength(
						hl, mContainer->getOwner()->getPixelsSize().getHeight() );
				}
			} else {
				CSS::StyleSheetLength hl( sizePart[1] );
				size.y = mContainer->getOwner()->convertLength(
					hl, mContainer->getOwner()->getPixelsSize().getHeight() );

				Sizef drawableSize( mDrawable->getSize() );
				size.x = drawableSize.getWidth() * ( size.getHeight() / drawableSize.getHeight() );
			}
		}
	}

	return size;
}

Vector2f UINodeDrawable::LayerDrawable::calcPosition( const std::string& positionEq ) {
	Vector2f position( Vector2f::Zero );
	std::vector<std::string> pos = String::split( positionEq, ' ' );

	if ( pos.size() == 1 )
		pos.push_back( "center" );

	if ( pos.size() == 2 ) {
		int xFloatIndex = 0;
		int yFloatIndex = 1;

		if ( "bottom" == pos[0] || "top" == pos[0] ) {
			xFloatIndex = 1;
			yFloatIndex = 0;
		}

		CSS::StyleSheetLength xl( pos[xFloatIndex] );
		CSS::StyleSheetLength yl( pos[yFloatIndex] );
		position.x = mContainer->getOwner()->convertLength(
			xl, mContainer->getOwner()->getPixelsSize().getWidth() - mDrawableSize.getWidth() );
		position.y = mContainer->getOwner()->convertLength(
			yl, mContainer->getOwner()->getPixelsSize().getHeight() - mDrawableSize.getHeight() );
	} else if ( pos.size() > 2 ) {
		if ( pos.size() == 3 ) {
			pos.push_back( "0dp" );
		}

		int xFloatIndex = 0;
		int yFloatIndex = 2;

		if ( "bottom" == pos[0] || "top" == pos[0] ) {
			xFloatIndex = 2;
			yFloatIndex = 0;
		}

		CSS::StyleSheetLength xl1( pos[xFloatIndex] );
		CSS::StyleSheetLength xl2( pos[xFloatIndex + 1] );
		CSS::StyleSheetLength yl1( pos[yFloatIndex] );
		CSS::StyleSheetLength yl2( pos[yFloatIndex + 1] );

		position.x = mContainer->getOwner()->convertLength(
			xl1, mContainer->getOwner()->getPixelsSize().getWidth() - mDrawableSize.getWidth() );

		Float xl2Val = mContainer->getOwner()->convertLength(
			xl2, mContainer->getOwner()->getPixelsSize().getWidth() - mDrawableSize.getWidth() );
		position.x += ( pos[xFloatIndex] == "right" ) ? -xl2Val : xl2Val;

		position.y = mContainer->getOwner()->convertLength(
			yl1, mContainer->getOwner()->getPixelsSize().getWidth() - mDrawableSize.getHeight() );

		Float yl2Val = mContainer->getOwner()->convertLength(
			yl2, mContainer->getOwner()->getPixelsSize().getWidth() - mDrawableSize.getHeight() );
		position.y += ( pos[yFloatIndex] == "bottom" ) ? -yl2Val : yl2Val;
	}

	return position;
}

const std::string& UINodeDrawable::LayerDrawable::getSizeEq() const {
	return mSizeEq;
}

void UINodeDrawable::LayerDrawable::setSizeEq( const std::string& sizeEq ) {
	if ( mSizeEq != sizeEq ) {
		mSizeEq = sizeEq;
		invalidate();
	}
}

const std::string& UINodeDrawable::LayerDrawable::getPositionY() const {
	return mPositionY;
}

void UINodeDrawable::LayerDrawable::setPositionY( const std::string& positionY ) {
	if ( mPositionY != positionY ) {
		mPositionY = positionY;
		invalidate();
	}
}

const std::string& UINodeDrawable::LayerDrawable::getPositionX() const {
	return mPositionX;
}

void UINodeDrawable::LayerDrawable::setPositionX( const std::string& positionX ) {
	if ( mPositionX != positionX ) {
		mPositionX = positionX;
		invalidate();
	}
}

void UINodeDrawable::LayerDrawable::setOffset( const Vector2f& offset ) {
	mOffset = offset;
}

std::string UINodeDrawable::LayerDrawable::getOffsetEq() {
	return String::fromFloat( mOffset.x, "px" ) + " " + String::fromFloat( mOffset.y, "px" );
}

void UINodeDrawable::LayerDrawable::onPositionChange() {
	invalidate();
}

void UINodeDrawable::LayerDrawable::update() {
	if ( mDrawable == NULL )
		return;

	mDrawableSize = calcDrawableSize( mSizeEq );
	mOffset = calcPosition( mPositionX + " " + mPositionY );

	mNeedsUpdate = false;
}

}} // namespace EE::UI
