#include <eepp/ui/uidragable.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/window/input.hpp>

namespace EE { namespace UI {

UIDragable::UIDragable( const UIControl::CreateParams& Params ) :
	UIControl( Params ),
	mDragButton( EE_BUTTON_LMASK )
{
	mControlFlags |= UI_CTRL_FLAG_DRAGABLE;
}

UIDragable::~UIDragable() {
}

Uint32 UIDragable::getType() const {
	return UI_TYPE_CONTROL_DRAGABLE;
}

bool UIDragable::isType( const Uint32& type ) const {
	return UIDragable::getType() == type ? true : UIControl::isType( type );
}

Uint32 UIDragable::onMouseDown( const Vector2i& Pos, const Uint32 Flags ) {
	if ( !( UIManager::instance()->getLastPressTrigger() & mDragButton ) && ( Flags & mDragButton ) && isDragEnabled() && !isDragging() ) {
		setDragging( true );
		mDragPoint = Pos;
	}

	UIControl::onMouseDown( Pos, Flags );
	return 1;
}

Uint32 UIDragable::onMouseUp( const Vector2i& Pos, const Uint32 Flags ) {
	if ( isDragEnabled() && isDragging() && ( Flags & mDragButton ) ) {
		setDragging( false );
	}

	UIControl::onMouseUp( Pos, Flags );
	return 1;
}

const Vector2i& UIDragable::getDragPoint() const {
	return mDragPoint;
}

void UIDragable::setDragPoint( const Vector2i& Point ) {
	mDragPoint = Point;
}

void UIDragable::update() {
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

		if ( mDragPoint != Pos ) {
			if ( OnDrag( Pos ) ) {
				mPos += -( mDragPoint - Pos );

				mDragPoint = Pos;

				onPositionChange();

				UIManager::instance()->setControlDragging( true );
			}
		}
	}
}

Uint32 UIDragable::OnDrag( const Vector2i& Pos ) {
	return 1;
}

Uint32 UIDragable::OnDragStart( const Vector2i& Pos ) {
	return 1;
}

Uint32 UIDragable::OnDragEnd( const Vector2i& Pos ) {
	return 1;
}

bool UIDragable::isDragEnabled() const {
	return 0 != ( mFlags & UI_DRAG_ENABLE );
}

void UIDragable::setDragEnabled( const bool& enable ) {
	writeFlag( UI_DRAG_ENABLE, true == enable );
}

bool UIDragable::isDragging() const {
	return 0 != ( mControlFlags & UI_CTRL_FLAG_DRAGGING );
}

void UIDragable::setDragging( const bool& dragging ) {
	writeCtrlFlag( UI_CTRL_FLAG_DRAGGING, true == dragging );

	if ( dragging ) {
		UIMessage tMsg( this, UIMessage::MsgDragStart, 0 );
		messagePost( &tMsg );

		OnDragStart( UIManager::instance()->getMousePos() );
	} else {
		UIMessage tMsg( this, UIMessage::MsgDragEnd, 0 );
		messagePost( &tMsg );

		OnDragEnd( UIManager::instance()->getMousePos() );
	}
}

void UIDragable::setDragButton( const Uint32& Button ) {
	mDragButton = Button;
}

const Uint32& UIDragable::getDragButton() const {
	return mDragButton;
}

}}
