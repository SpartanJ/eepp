#include <eepp/ui/uidragablecontrol.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/window/input.hpp>

namespace EE { namespace UI {

UIDragableControl * UIDragableControl::New() {
	return eeNew( UIDragableControl, () );
}

UIDragableControl::UIDragableControl() :
	UIControl(),
	mDragButton( EE_BUTTON_LMASK )
{
	mControlFlags |= UI_CTRL_FLAG_DRAGABLE;
}

UIDragableControl::~UIDragableControl() {
}

Uint32 UIDragableControl::getType() const {
	return UI_TYPE_CONTROL_DRAGABLE;
}

bool UIDragableControl::isType( const Uint32& type ) const {
	return UIDragableControl::getType() == type ? true : UIControl::isType( type );
}

Uint32 UIDragableControl::onMouseDown( const Vector2i& Pos, const Uint32 Flags ) {
	if ( !( UIManager::instance()->getLastPressTrigger() & mDragButton ) && ( Flags & mDragButton ) && isDragEnabled() && !isDragging() ) {
		setDragging( true );
		mDragPoint = Pos;
	}

	UIControl::onMouseDown( Pos, Flags );
	return 1;
}

Uint32 UIDragableControl::onMouseUp( const Vector2i& Pos, const Uint32 Flags ) {
	if ( isDragEnabled() && isDragging() && ( Flags & mDragButton ) ) {
		setDragging( false );
	}

	UIControl::onMouseUp( Pos, Flags );
	return 1;
}

const Vector2i& UIDragableControl::getDragPoint() const {
	return mDragPoint;
}

void UIDragableControl::setDragPoint( const Vector2i& Point ) {
	mDragPoint = Point;
}

void UIDragableControl::update() {
	UIControl::update();

	if ( !isDragEnabled() )
		return;

	if ( isDragging() ) {
		if ( !( UIManager::instance()->getPressTrigger() & mDragButton ) ) {
			setDragging( false );
			UIManager::instance()->setControlDragging( false );
			return;
		}

		Vector2i Pos( UIManager::instance()->getMousePos() );

		if ( mDragPoint != Pos && ( abs( mDragPoint.x - Pos.x ) > PixelDensity::getPixelDensity() || abs( mDragPoint.y - Pos.y ) > PixelDensity::getPixelDensity() ) ) {
			if ( onDrag( Pos ) ) {
				Sizei dragDiff;

				dragDiff.x = (Int32)( (Float)( mDragPoint.x - Pos.x ) / PixelDensity::getPixelDensity() );
				dragDiff.y = (Int32)( (Float)( mDragPoint.y - Pos.y ) / PixelDensity::getPixelDensity() );

				setInternalPosition( mPos - dragDiff );

				mDragPoint = Pos;

				onPositionChange();

				UIManager::instance()->setControlDragging( true );
			}
		}
	}
}

Uint32 UIDragableControl::onDrag( const Vector2i& Pos ) {
	return 1;
}

Uint32 UIDragableControl::onDragStart( const Vector2i& Pos ) {
	return 1;
}

Uint32 UIDragableControl::onDragEnd( const Vector2i& Pos ) {
	return 1;
}

bool UIDragableControl::isDragEnabled() const {
	return 0 != ( mFlags & UI_DRAG_ENABLE );
}

void UIDragableControl::setDragEnabled( const bool& enable ) {
	writeFlag( UI_DRAG_ENABLE, true == enable );
}

bool UIDragableControl::isDragging() const {
	return 0 != ( mControlFlags & UI_CTRL_FLAG_DRAGGING );
}

void UIDragableControl::setDragging( const bool& dragging ) {
	writeCtrlFlag( UI_CTRL_FLAG_DRAGGING, true == dragging );

	if ( dragging ) {
		UIMessage tMsg( this, UIMessage::MsgDragStart, 0 );
		messagePost( &tMsg );

		onDragStart( UIManager::instance()->getMousePos() );
	} else {
		UIMessage tMsg( this, UIMessage::MsgDragEnd, 0 );
		messagePost( &tMsg );

		onDragEnd( UIManager::instance()->getMousePos() );
	}
}

void UIDragableControl::setDragButton( const Uint32& Button ) {
	mDragButton = Button;
}

const Uint32& UIDragableControl::getDragButton() const {
	return mDragButton;
}

}}
