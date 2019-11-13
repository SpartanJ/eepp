#include <eepp/ui/uinodedrawable.hpp>
#include <eepp/core/core.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/drawableresource.hpp>

namespace EE { namespace UI {

UINodeDrawable::Repeat UINodeDrawable::repeatFromText( const std::string& text ) {
	if ( "repeat" == text ) return UINodeDrawable::Repeat::RepeatXY;
	if ( "repeat-x" == text ) return UINodeDrawable::Repeat::RepeatX;
	if ( "repeat-y" == text ) return UINodeDrawable::Repeat::RepeatY;
	return UINodeDrawable::Repeat::NoRepeat;
}

UINodeDrawable * UINodeDrawable::New() {
	return eeNew( UINodeDrawable, () );
}

UINodeDrawable::UINodeDrawable() :
	Drawable( Drawable::UINODEDRAWABLE ),
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
		mNeedsUpdate = true;
	}

	if ( size != mSize ) {
		mSize = size;
		mNeedsUpdate = true;
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
	mNeedsUpdate = true;
}

void UINodeDrawable::onSizeChange() {
	mNeedsUpdate = true;
}

void UINodeDrawable::update() {
	mBackgroundColor.setPosition( mPosition );
	mBackgroundColor.setSize( mSize );

	for ( size_t i = 0; i < mGroup.size(); i++ ) {
		UINodeDrawable::LayerDrawable * drawable = mGroup[i];
		drawable->setPosition( mPosition );
		drawable->setPositionEq( mPosEq[i] );
		drawable->setSize( mSize );
	}

	mNeedsUpdate = false;
}

UINodeDrawable::LayerDrawable * UINodeDrawable::LayerDrawable::New( UINodeDrawable * container ) {
	return eeNew( UINodeDrawable::LayerDrawable, ( container ) );
}

UINodeDrawable::LayerDrawable::LayerDrawable( UINodeDrawable * container ) :
	Drawable(UINODEDRAWABLE_LAYERDRAWABLE),
	mContainer(container),
	mNeedsUpdate(false),
	mUpdatePosEq(false),
	mOwnsDrawable(false),
	mDrawable(NULL),
	mResourceChangeCbId(0)
{}

UINodeDrawable::LayerDrawable::~LayerDrawable() {
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
		mNeedsUpdate = true;
	}

	if ( size != mSize ) {
		mSize = size;
		mNeedsUpdate = true;
	}

	if ( mDrawable == NULL )
		return;

	if ( mNeedsUpdate )
		update();

	mDrawable->draw( mPosition + mOffset, mSize );
}

Sizef UINodeDrawable::LayerDrawable::getSize() {
	return mSize;
}

void UINodeDrawable::LayerDrawable::setSize(const Sizef& size) {
	if ( size != mSize ) {
		mSize = size;
		mNeedsUpdate = true;
		mUpdatePosEq = true;
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
		mResourceChangeCbId = reinterpret_cast<DrawableResource*>( mDrawable )->pushResourceChangeCallback( [&] ( DrawableResource::Event, DrawableResource* ) {
			mNeedsUpdate = true;
		} );
	}
}

const Vector2f& UINodeDrawable::LayerDrawable::getOffset() const {
	return mOffset;
}

void UINodeDrawable::LayerDrawable::setPositionEq( const std::string& positionEq ) {
	if ( mPositionEq != positionEq ) {
		mPositionEq = positionEq;
		mUpdatePosEq = true;
		mNeedsUpdate = true;
	}
}

const UINodeDrawable::Repeat& UINodeDrawable::LayerDrawable::getRepeat() const {
	return mRepeat;
}

void UINodeDrawable::LayerDrawable::setRepeat( const UINodeDrawable::Repeat& repeat ) {
	mRepeat = repeat;
	mNeedsUpdate = true;
	mUpdatePosEq = true;
}

void UINodeDrawable::LayerDrawable::onPositionChange() {
	mNeedsUpdate = true;
	mUpdatePosEq = true;
}

void UINodeDrawable::LayerDrawable::update() {
	// TODO: Implement background-position and background-repeat
	if ( mUpdatePosEq ) {

		mUpdatePosEq = false;
	}
}

}}
