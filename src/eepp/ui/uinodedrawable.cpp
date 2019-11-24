#include <eepp/ui/uinodedrawable.hpp>
#include <eepp/core/core.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/drawableresource.hpp>
#include <eepp/ui/css/stylesheetlength.hpp>
#include <eepp/ui/uinode.hpp>
#include <eepp/math/easing.hpp>
using namespace EE::Math::easing;

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

bool UINodeDrawable::layerExists( int index ) {
	return mGroup.find( index ) != mGroup.end();
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
	}
}

void UINodeDrawable::setDrawablePosition( int index, const std::string& positionEq ) {
	getLayer( index )->setPositionEq( positionEq );
}

void UINodeDrawable::setDrawableRepeat( int index, const std::string& repeatRule ) {
	getLayer( index )->setRepeat( repeatFromText( repeatRule ) );
}

void UINodeDrawable::setDrawableSize( int index, const std::string& sizeEq ) {
	getLayer( index )->setSizeEq( sizeEq );
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
	mNeedsUpdate = false;
}

/**** UINodeDrawable::LayerDrawable ****/

UINodeDrawable::LayerDrawable * UINodeDrawable::LayerDrawable::New( UINodeDrawable * container ) {
	return eeNew( UINodeDrawable::LayerDrawable, ( container ) );
}

UINodeDrawable::LayerDrawable::LayerDrawable( UINodeDrawable * container ) :
	Drawable(UINODEDRAWABLE_LAYERDRAWABLE),
	mContainer(container),
	mPositionEq("0px 0px"),
	mDrawableSizeEq("auto"),
	mNeedsUpdate(false),
	mOwnsDrawable(false),
	mDrawable(NULL),
	mResourceChangeCbId(0),
	mMoveAction(NULL),
	mMoveActionCbId(0)
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

	if ( NULL != mMoveAction ) {
		update();
		mMoveAction->update( Time::Zero );
		setOffset( mMoveAction->getOffset() );
	}

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
}

const Sizef& UINodeDrawable::LayerDrawable::getDrawableSize() const {
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
				CSS::StyleSheetLength wl( CSS::StyleSheetLength::fromString( sizePart[0] ) );
				size.x = mContainer->getOwner()->lengthAsPixels( wl, Sizef::Zero, true );

				if ( sizePart[1] == "auto" ) {
					Sizef drawableSize( mDrawable->getSize() );
					size.y = drawableSize.getHeight() * ( size.getWidth() / drawableSize.getWidth() );
				} else {
					CSS::StyleSheetLength hl( CSS::StyleSheetLength::fromString( sizePart[1] ) );
					size.y = mContainer->getOwner()->lengthAsPixels( hl, Sizef::Zero, false );
				}
			} else {
				CSS::StyleSheetLength hl( CSS::StyleSheetLength::fromString( sizePart[1] ) );
				size.y = mContainer->getOwner()->lengthAsPixels( hl, Sizef::Zero, false );

				Sizef drawableSize( mDrawable->getSize() );
				size.x = drawableSize.getWidth() * ( size.getHeight() / drawableSize.getHeight()  );
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

		CSS::StyleSheetLength xl( CSS::StyleSheetLength::fromString( pos[xFloatIndex] ) );
		CSS::StyleSheetLength yl( CSS::StyleSheetLength::fromString( pos[yFloatIndex] ) );
		position.x = mContainer->getOwner()->lengthAsPixels( xl, mDrawableSize, true );
		position.y = mContainer->getOwner()->lengthAsPixels( yl, mDrawableSize, false );
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

		position.x = mContainer->getOwner()->lengthAsPixels( xl1, mDrawableSize, true );

		Float xl2Val = mContainer->getOwner()->lengthAsPixels( xl2, mDrawableSize, true );
		position.x += ( pos[xFloatIndex] == "right" ) ? -xl2Val : xl2Val;

		position.y = mContainer->getOwner()->lengthAsPixels( yl1, mDrawableSize, false );

		Float yl2Val = mContainer->getOwner()->lengthAsPixels( yl2, mDrawableSize, false );
		position.y += ( pos[yFloatIndex] == "bottom" ) ? -yl2Val : yl2Val;
	}

	return position;
}

UINodeDrawable::MoveAction * UINodeDrawable::LayerDrawable::getMoveAction() const {
	return mMoveAction;
}

void UINodeDrawable::LayerDrawable::setMoveAction(MoveAction * moveAction) {
	mMoveAction = moveAction;
	if ( mMoveAction != NULL ) {
		auto delCb = [&] ( Action *, const Action::ActionType& ) {
			mMoveAction = NULL;
		};
		mMoveAction->addEventListener( Action::OnDone, delCb );
		mMoveAction->addEventListener( Action::OnDelete, delCb );
	}
}

void UINodeDrawable::LayerDrawable::setOffset( const Vector2f& offset ) {
	mOffset = offset;
}

void UINodeDrawable::LayerDrawable::onPositionChange() {
	invalidate();
}

void UINodeDrawable::LayerDrawable::update() {
	if ( mDrawable == NULL )
		return;

	mDrawableSize = calcDrawableSize( mDrawableSizeEq );
	mOffset = calcPosition( mPositionEq );

	mNeedsUpdate = false;
}

UINodeDrawable::MoveAction * UINodeDrawable::MoveAction::New( const std::string& posEqStart, const std::string& posEqEnd, const Time& duration, const Ease::Interpolation& type ) {
	return eeNew( MoveAction, ( posEqStart, posEqEnd, duration, type ) );
}

UINodeDrawable::MoveAction::MoveAction( const std::string& posEqStart, const std::string& posEqEnd, const Time& duration, const Ease::Interpolation& type ) :
	mStart( posEqStart ),
	mEnd( posEqEnd ),
	mDuration( duration ),
	mType( type )
{}

Action * UINodeDrawable::MoveAction::clone() const {
	return MoveAction::New( mStart, mEnd, mDuration, mType );
}

Action * UINodeDrawable::MoveAction::reverse() const {
	return MoveAction::New( mEnd, mStart, mDuration, mType );
}

void UINodeDrawable::MoveAction::start() {
	onStart();

	sendEvent( ActionType::OnStart );
}

void UINodeDrawable::MoveAction::stop() {
	onStop();

	sendEvent( ActionType::OnStop );
}

void UINodeDrawable::MoveAction::update( const Time& time ) {
	mElapsed += time;

	onUpdate( time );
}

bool UINodeDrawable::MoveAction::isDone() {
	return mElapsed.asMicroseconds() >= mDuration.asMicroseconds();
}

void UINodeDrawable::MoveAction::onStart() {
	UINode * node = mNode->asType<UINode>();
	LayerDrawable * layer = node->getBackground()->getLayer(0);
	layer->setMoveAction( this );
	onUpdate( Time::Zero );
}

void UINodeDrawable::MoveAction::onUpdate( const Time& ) {
	if ( NULL != mNode && mNode->isUINode() ) {
		UINode * node = mNode->asType<UINode>();
		LayerDrawable * layer = node->getBackground()->getLayer(0);
		Vector2f start = layer->calcPosition( mStart );
		Vector2f end = layer->calcPosition( mEnd );
		Time time = mElapsed.asMicroseconds() > mDuration.asMicroseconds() ? mDuration : mElapsed;
		Float x = easingCb[ mType ]( time.asMilliseconds(), start.x, end.x - start.x, mDuration.asMilliseconds() );
		Float y = easingCb[ mType ]( time.asMilliseconds(), start.y, end.y - start.y, mDuration.asMilliseconds() );
		mOffset = Vector2f( x, y );
		if ( isDone() ) {
			layer->setMoveAction( NULL );
			layer->setPositionEq( mEnd );
		}
	}
}

}}
