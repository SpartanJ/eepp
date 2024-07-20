#include <eepp/core/core.hpp>
#include <eepp/graphics/drawableresource.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/math/easing.hpp>
#include <eepp/system/functionstring.hpp>
#include <eepp/ui/css/stylesheetlength.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/uinode.hpp>
#include <eepp/ui/uinodedrawable.hpp>
#include <eepp/ui/uiwidget.hpp>
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
	mBackgroundColor( owner ),
	mNeedsUpdate( true ),
	mClipEnabled( false ),
	mSmooth( false ) {
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

void UINodeDrawable::setBorderRadius( const Uint32& radius ) {
	mBackgroundColor.setRadius( radius );
}

Uint32 UINodeDrawable::getBorderRadius() const {
	return mBackgroundColor.getRadius();
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

void UINodeDrawable::setDrawable( int index, const std::string& drawable ) {
	if ( drawable != getLayer( index )->getDrawableRef() ) {
		getLayer( index )->setDrawable( drawable );
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

void UINodeDrawable::setDrawableColor( int index, const Color& color ) {
	getLayer( index )->setColor( color );
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
	mBackgroundColor.invalidate();
}

UINode* UINodeDrawable::getOwner() const {
	return mOwner;
}

UIBackgroundDrawable& UINodeDrawable::getBackgroundDrawable() {
	return mBackgroundColor;
}

bool UINodeDrawable::isSmooth() const {
	return mSmooth;
}

void UINodeDrawable::setSmooth( bool smooth ) {
	mSmooth = smooth;
	getBackgroundDrawable().setSmooth( smooth );
}

Sizef UINodeDrawable::getSize() {
	return mSize;
}

Sizef UINodeDrawable::getPixelsSize() {
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

	bool masked = false;
	ClippingMask* clippingMask = GLi->getClippingMask();
	if ( !mGroup.empty() && mBackgroundColor.hasRadius() ) {
		masked = true;
		clippingMask->setMaskMode( ClippingMask::Inclusive );
		clippingMask->clearMasks();
		clippingMask->appendMask( mBackgroundColor );
		clippingMask->stencilMaskEnable();
	}

	bool isPolySmooth = GLi->isPolygonSmooth();
	bool isLineSmooth = GLi->isLineSmooth();

	if ( mSmooth ) {
		if ( !isPolySmooth )
			GLi->polygonSmooth( true );

		if ( !isLineSmooth )
			GLi->lineSmooth( true );
	}

	// Draw in reverse order to respect the background-image specification:
	// "The background images are drawn on stacking context layers on top of each other. The first
	// layer specified is drawn as if it is closest to the user."
	for ( auto drawableIt = mGroup.rbegin(); drawableIt != mGroup.rend(); ++drawableIt ) {
		UINodeDrawable::LayerDrawable* drawable = drawableIt->second;

		if ( alpha != 255 ) {
			Color color = drawable->getColor();
			drawable->setAlpha( alpha * color.a / 255 );
			drawable->draw( position, size );
			drawable->setAlpha( color.a );
		} else {
			drawable->draw( position, size );
		}
	}

	if ( mSmooth ) {
		if ( !isPolySmooth )
			GLi->polygonSmooth( isPolySmooth );

		if ( !isLineSmooth )
			GLi->lineSmooth( isLineSmooth );
	}

	if ( masked ) {
		clippingMask->stencilMaskDisable();
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
	if ( drawableSize.getHeight() > 0 ) {
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

	RGB prevColor = mDrawable->getColorFilter();
	if ( mColorWasSet )
		mDrawable->setColorFilter( getColor() );
	mDrawable->setAlpha( getAlpha() );

	switch ( mRepeat ) {
		case Repeat::NoRepeat:
			mDrawable->draw( mPosition + mOffset, mDrawableSize );
			break;
		case Repeat::RepeatX: {
			if ( mDrawableSize.getWidth() > 0 ) {
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
			}
			break;
		}
		case Repeat::RepeatY: {
			repeatYdraw( mDrawable, mPosition, mOffset, mSize, mDrawableSize );
			break;
		}
		case Repeat::RepeatXY: {
			if ( mDrawableSize.getWidth() > 0 ) {
				Float startX = mPosition.x + mOffset.x - mDrawableSize.getWidth();
				while ( startX > mPosition.x - mDrawableSize.getWidth() ) {
					repeatYdraw( mDrawable, mPosition, Vector2f( startX - mPosition.x, mOffset.y ),
								 mSize, mDrawableSize );
					startX -= mDrawableSize.getWidth();
				};
				repeatYdraw( mDrawable, mPosition, mOffset, mSize, mDrawableSize );
				startX = mPosition.x + mOffset.x + mDrawableSize.getWidth();
				while ( startX < mPosition.x + mSize.getWidth() ) {
					repeatYdraw( mDrawable, mPosition, Vector2f( startX - mPosition.x, mOffset.y ),
								 mSize, mDrawableSize );
					startX += mDrawableSize.getWidth();
				};
			}
			break;
		}
	}
	if ( mColorWasSet )
		mDrawable->setColorFilter( prevColor );
}

Sizef UINodeDrawable::LayerDrawable::getSize() {
	return mSize;
}

Sizef UINodeDrawable::LayerDrawable::getPixelsSize() {
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

const std::string& UINodeDrawable::LayerDrawable::getDrawableRef() const {
	return mDrawableRef;
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
	mDrawableRef = "";
	mOwnsDrawable = ownIt;
	invalidate();

	if ( NULL != mDrawable && mDrawable->isDrawableResource() ) {
		mResourceChangeCbId =
			reinterpret_cast<DrawableResource*>( mDrawable )
				->pushResourceChangeCallback(
					[this]( Uint32, DrawableResource::Event event, DrawableResource* ) {
						invalidate();
						if ( event == DrawableResource::Event::Unload ) {
							mResourceChangeCbId = 0;
							mDrawable = NULL;
							mOwnsDrawable = false;
						}
					} );
	}
}

void UINodeDrawable::LayerDrawable::setDrawable( const std::string& drawableRef ) {
	bool ownIt;
	Drawable* drawable = createDrawable( drawableRef, mSize, ownIt );

	setDrawable( drawable, ownIt );

	mDrawableRef = drawableRef;
}

Drawable* UINodeDrawable::LayerDrawable::createDrawable( const std::string& value,
														 const Sizef& size, bool& ownIt ) {
	return CSS::StyleSheetSpecification::instance()->getDrawableImageParser().createDrawable(
		value, size, ownIt, mContainer->getOwner() );
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

	if ( NULL == mDrawable )
		return Sizef::Zero;

	if ( drawableSizeEq == "auto" ) {
		if ( mDrawable->getDrawableType() == Drawable::RECTANGLE ) {
			size = mSize;
		} else {
			size = mDrawable->getPixelsSize();
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

Vector2f UINodeDrawable::LayerDrawable::calcPosition( std::string positionXEq,
													  std::string positionYEq ) {
	Vector2f position( Vector2f::Zero );

	if ( positionXEq.empty() )
		positionXEq = "center";

	if ( positionYEq.empty() )
		positionXEq = "center";

	auto posX = String::split( positionXEq, ' ' );
	auto posY = String::split( positionYEq, ' ' );

	if ( posX.empty() || posY.empty() )
		return position;

	bool needsRoundingX = positionXEq.back() == '%';
	bool needsRoundingY = positionYEq.back() == '%';

	if ( posX.size() == 2 ) {
		CSS::StyleSheetLength xl1( posX[0] );
		CSS::StyleSheetLength xl2( posX[1] );

		position.x = mContainer->getOwner()->convertLength(
			xl1, mContainer->getOwner()->getPixelsSize().getWidth() - mDrawableSize.getWidth() );

		Float xl2Val = mContainer->getOwner()->convertLength(
			xl2, mContainer->getOwner()->getPixelsSize().getWidth() - mDrawableSize.getWidth() );

		position.x += ( posX[0] == "right" ) ? -xl2Val : xl2Val;
	} else {
		CSS::StyleSheetLength xl( posX[0] );
		position.x = mContainer->getOwner()->convertLength(
			xl, mContainer->getOwner()->getPixelsSize().getWidth() - mDrawableSize.getWidth() );
	}

	if ( posY.size() == 2 ) {
		CSS::StyleSheetLength yl1( posY[0] );
		CSS::StyleSheetLength yl2( posY[1] );

		position.y = mContainer->getOwner()->convertLength(
			yl1, mContainer->getOwner()->getPixelsSize().getHeight() - mDrawableSize.getHeight() );

		Float xl2Val = mContainer->getOwner()->convertLength(
			yl2, mContainer->getOwner()->getPixelsSize().getHeight() - mDrawableSize.getHeight() );

		position.y += ( posY[0] == "bottom" ) ? -xl2Val : xl2Val;
	} else {
		CSS::StyleSheetLength yl( posY[0] );
		position.y = mContainer->getOwner()->convertLength(
			yl, mContainer->getOwner()->getPixelsSize().getHeight() - mDrawableSize.getHeight() );
	}

	if ( needsRoundingX )
		position.x = Math::round( position.x );

	if ( needsRoundingY )
		position.y = Math::round( position.y );

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

void UINodeDrawable::LayerDrawable::onColorFilterChange() {
	mColorWasSet = true;
	invalidate();
}

void UINodeDrawable::LayerDrawable::update() {
	if ( mDrawable == NULL )
		return;

	if ( !mDrawableRef.empty() ) {
		setDrawable( mDrawableRef );
	}

	mDrawableSize = calcDrawableSize( mSizeEq );
	mOffset = calcPosition( mPositionX, mPositionY );

	mNeedsUpdate = false;
}

}} // namespace EE::UI
