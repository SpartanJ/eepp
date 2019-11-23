#include <eepp/ui/uinodedrawable.hpp>
#include <eepp/core/core.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/drawableresource.hpp>
#include <eepp/ui/css/stylesheetlength.hpp>
#include <eepp/ui/uinode.hpp>

namespace EE { namespace UI {

UINodeDrawable::Repeat UINodeDrawable::repeatFromText( const std::string& text ) {
	if ( "repeat" == text ) return UINodeDrawable::Repeat::RepeatXY;
	if ( "repeat-x" == text ) return UINodeDrawable::Repeat::RepeatX;
	if ( "repeat-y" == text ) return UINodeDrawable::Repeat::RepeatY;
	return UINodeDrawable::Repeat::NoRepeat;
}

UINodeDrawable * UINodeDrawable::New( UINode * owner ) {
	return eeNew( UINodeDrawable, ( owner ) );
}

UINodeDrawable::UINodeDrawable( UINode * owner ) :
	Drawable( Drawable::UINODEDRAWABLE ),
	mOwner( owner ),
	mNeedsUpdate(true),
	mClipEnabled(false)
{
	mBackgroundColor.setColor(Color::Transparent);
}

UINodeDrawable::~UINodeDrawable() {
	clearDrawables();
}

void UINodeDrawable::clearDrawables() {
	for ( auto& drawable : mGroup ) {
		eeDelete( drawable.second );
	}

	mGroup.clear();
	mBackgroundColor.setColor(Color::Transparent);
}

void UINodeDrawable::setBorderRadius( const Uint32& corners ) {
	mBackgroundColor.setCorners( corners );
}

Uint32 UINodeDrawable::getBorderRadius() const {
	return mBackgroundColor.getCorners();
}

UINodeDrawable::LayerDrawable* UINodeDrawable::getLayer( int index ) {
	auto it = mGroup.find(index);

	if ( it == mGroup.end() )
		mGroup[ index ] = UINodeDrawable::LayerDrawable::New( this );

	return mGroup[ index ];
}

void UINodeDrawable::setDrawable( int index, Drawable* drawable, bool ownIt ) {
	if ( drawable != getLayer( index )->getDrawable() ) {
		getLayer( index )->setDrawable( drawable, ownIt );
		mNeedsUpdate = true;
	}
}

void UINodeDrawable::setDrawablePosition( int index, const std::string& positionEq ) {
	if ( mPosEq[index] != positionEq ) {
		mPosEq[index] = positionEq;
		mNeedsUpdate = true;
	}
}

void UINodeDrawable::setDrawableRepeat( int index, const std::string& repeatRule ) {
	getLayer( index )->setRepeat( repeatFromText( repeatRule ) );
}

void UINodeDrawable::setDrawableSize( int index, const std::string& sizeEq ) {
	if ( mSizeEq[index] != sizeEq ) {
		mSizeEq[index] = sizeEq;
		mNeedsUpdate = true;
	}
}

void UINodeDrawable::setBackgroundColor(const Color& color) {
	mBackgroundColor.setColor( color );
}

Color UINodeDrawable::getBackgroundColor() const {
	return mBackgroundColor.getColor();
}

bool UINodeDrawable::getClipEnabled() const {
	return mClipEnabled;
}

void UINodeDrawable::setClipEnabled(bool clipEnabled) {
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
		UINodeDrawable::LayerDrawable * drawable = drawableIt.second;

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

void UINodeDrawable::draw(const Vector2f & position) {
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

	for ( size_t i = 0; i < mGroup.size(); i++ ) {
		UINodeDrawable::LayerDrawable * drawable = mGroup[i];
		drawable->setPosition( mPosition );
		drawable->setSizeEq( mSizeEq[i].empty() ? "auto" : mSizeEq[i] );
		drawable->setPositionEq( mPosEq[i] );
		drawable->setSize( mSize );
	}

	mNeedsUpdate = false;
}

/**** UINodeDrawable::LayerDrawable ****/

UINodeDrawable::LayerDrawable * UINodeDrawable::LayerDrawable::New( UINodeDrawable * container ) {
	return eeNew( UINodeDrawable::LayerDrawable, ( container ) );
}

UINodeDrawable::LayerDrawable::LayerDrawable( UINodeDrawable * container ) :
	Drawable(UINODEDRAWABLE_LAYERDRAWABLE),
	mContainer(container),
	mDrawableSizeEq("auto"),
	mNeedsUpdate(false),
	mUpdatePosEq(false),
	mOwnsDrawable(false),
	mDrawable(NULL),
	mResourceChangeCbId(0)
{}

UINodeDrawable::LayerDrawable::~LayerDrawable() {
	if ( NULL != mDrawable && 0 != mResourceChangeCbId && mDrawable->isDrawableResource() ) {
		reinterpret_cast<DrawableResource*>( mDrawable )->popResourceChangeCallback( mResourceChangeCbId );
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

	mDrawable->draw( mPosition + mOffset, mDrawableSize );
}

Sizef UINodeDrawable::LayerDrawable::getSize() {
	return mSize;
}

void UINodeDrawable::LayerDrawable::setSize(const Sizef& size) {
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
			reinterpret_cast<DrawableResource*>( mDrawable )->popResourceChangeCallback( mResourceChangeCbId );
		}

		if ( mOwnsDrawable ) {
			eeSAFE_DELETE( mDrawable );
		}
	}

	mDrawable = drawable;
	mOwnsDrawable = ownIt;

	if ( mDrawable->isDrawableResource() ) {
		mResourceChangeCbId = reinterpret_cast<DrawableResource*>( mDrawable )->pushResourceChangeCallback( [&] ( DrawableResource::Event event, DrawableResource* ) {
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
	return mOffset;
}

void UINodeDrawable::LayerDrawable::setPositionEq( const std::string& positionEq ) {
	if ( mPositionEq != positionEq ) {
		mPositionEq = positionEq;
		invalidate();
	}
}

void UINodeDrawable::LayerDrawable::setSizeEq(const std::string& size) {
	if ( mDrawableSizeEq != size ) {
		mDrawableSizeEq = String::trim( size );
		invalidate();
	}
}

const UINodeDrawable::Repeat& UINodeDrawable::LayerDrawable::getRepeat() const {
	return mRepeat;
}

void UINodeDrawable::LayerDrawable::setRepeat( const UINodeDrawable::Repeat& repeat ) {
	mRepeat = repeat;
	invalidate();
}

void UINodeDrawable::LayerDrawable::invalidate() {
	mNeedsUpdate = true;
	mUpdatePosEq = true;
}

void UINodeDrawable::LayerDrawable::onPositionChange() {
	invalidate();
}

void UINodeDrawable::LayerDrawable::update() {
	if ( mDrawable == NULL )
		return;

	if ( mDrawableSizeEq == "auto" ) {
		if ( mDrawable->getDrawableType() == Drawable::RECTANGLE ) {
			mDrawableSize = mSize;
		} else {
			mDrawableSize = mDrawable->getSize();
		}
	} else if ( mDrawableSizeEq == "expand" ) {
		mDrawableSize = mSize;
	} else if ( mDrawableSizeEq == "contain" ) {
		Sizef drawableSize( mDrawable->getSize() );
		Float Scale1 = mSize.getWidth() / drawableSize.getWidth();
		Float Scale2 = mSize.getHeight() / drawableSize.getHeight();
		if ( Scale1 < 1 || Scale2 < 1 ) {
			Scale1 = eemin( Scale1, Scale2 );
			mDrawableSize = Sizef( drawableSize.getWidth() * Scale1, drawableSize.getHeight() * Scale1 );
		} else {
			mDrawableSize = drawableSize;
		}
	} else if ( mDrawableSizeEq == "cover" ) {
		Sizef drawableSize( mDrawable->getSize() );
		Float Scale1 = mSize.getWidth() / drawableSize.getWidth();
		Float Scale2 = mSize.getHeight() / drawableSize.getHeight();
		Scale1 = eemax( Scale1, Scale2 );
		mDrawableSize = Sizef( drawableSize.getWidth() * Scale1, drawableSize.getHeight() * Scale1 );
	} else {
		std::vector<std::string> sizePart = String::split( mDrawableSizeEq, ' ' );

		if ( sizePart.size() == 1 ) {
			sizePart.push_back( "auto" );
		}

		if ( sizePart.size() == 2 ) {
			if ( sizePart[0] == "auto" && sizePart[1] == "auto" ) {
				if ( mDrawable->getDrawableType() == Drawable::RECTANGLE ) {
					mDrawableSize = mSize;
				} else {
					mDrawableSize = mDrawable->getSize();
				}
			} else if ( sizePart[0] != "auto" ) {
				CSS::StyleSheetLength wl( CSS::StyleSheetLength::fromString( sizePart[0] ) );
				mDrawableSize.x = mContainer->getOwner()->lengthAsPixels( wl, Sizef::Zero, true );

				if ( sizePart[1] == "auto" ) {
					Sizef drawableSize( mDrawable->getSize() );
					mDrawableSize.y = drawableSize.getHeight() * ( mDrawableSize.getWidth() / drawableSize.getWidth() );
				} else {
					CSS::StyleSheetLength hl( CSS::StyleSheetLength::fromString( sizePart[1] ) );
					mDrawableSize.y = mContainer->getOwner()->lengthAsPixels( hl, Sizef::Zero, false );
				}
			} else {
				CSS::StyleSheetLength hl( CSS::StyleSheetLength::fromString( sizePart[1] ) );
				mDrawableSize.y = mContainer->getOwner()->lengthAsPixels( hl, Sizef::Zero, false );

				Sizef drawableSize( mDrawable->getSize() );
				mDrawableSize.x = drawableSize.getWidth() * ( mDrawableSize.getHeight() / drawableSize.getHeight()  );
			}
		}
	}

	if ( mUpdatePosEq ) {
		std::vector<std::string> pos = String::split( mPositionEq, ' ' );

		if ( pos.size() == 1 ) {
			pos.push_back( "center" );
		}

		if ( pos.size() == 2 ) {
			CSS::StyleSheetLength xl( CSS::StyleSheetLength::fromString( pos[0] ) );
			CSS::StyleSheetLength yl( CSS::StyleSheetLength::fromString( pos[1] ) );
			mOffset.x = mContainer->getOwner()->lengthAsPixels( xl, mDrawableSize, true );
			mOffset.y = mContainer->getOwner()->lengthAsPixels( yl, mDrawableSize, false );
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

			CSS::StyleSheetLength xl1( CSS::StyleSheetLength::fromString( pos[xFloatIndex] ) );
			CSS::StyleSheetLength xl2( CSS::StyleSheetLength::fromString( pos[xFloatIndex+1] ) );
			CSS::StyleSheetLength yl1( CSS::StyleSheetLength::fromString( pos[yFloatIndex] ) );
			CSS::StyleSheetLength yl2( CSS::StyleSheetLength::fromString( pos[yFloatIndex+1] ) );

			mOffset.x = mContainer->getOwner()->lengthAsPixels( xl1, mDrawableSize, true );

			Float xl2Val = mContainer->getOwner()->lengthAsPixels( xl2, mDrawableSize, true );
			mOffset.x += ( pos[xFloatIndex] == "right" ) ? -xl2Val : xl2Val;

			mOffset.y = mContainer->getOwner()->lengthAsPixels( yl1, mDrawableSize, false );

			Float yl2Val = mContainer->getOwner()->lengthAsPixels( yl2, mDrawableSize, false );
			mOffset.y += ( pos[yFloatIndex] == "bottom" ) ? -yl2Val : yl2Val;
		}

		mUpdatePosEq = false;
	}

	mNeedsUpdate = false;
}

}}
