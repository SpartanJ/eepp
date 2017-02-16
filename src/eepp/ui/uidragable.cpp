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
	if ( !( UIManager::instance()->lastPressTrigger() & mDragButton ) && ( Flags & mDragButton ) && dragEnable() && !dragging() ) {
		dragging( true );
		mDragPoint = Pos;
	}

	UIControl::onMouseDown( Pos, Flags );
	return 1;
}

Uint32 UIDragable::onMouseUp( const Vector2i& Pos, const Uint32 Flags ) {
	if ( dragEnable() && dragging() && ( Flags & mDragButton ) ) {
		dragging( false );
	}

	UIControl::onMouseUp( Pos, Flags );
	return 1;
}

const Vector2i& UIDragable::dragPoint() const {
	return mDragPoint;
}

void UIDragable::dragPoint( const Vector2i& Point ) {
	mDragPoint = Point;
}

void UIDragable::update() {
	UIControl::update();

	if ( !dragEnable() )
		return;

	if ( dragging() ) {
		if ( !( UIManager::instance()->pressTrigger() & mDragButton ) ) {
			dragging( false );
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

bool UIDragable::dragEnable() const {
	return 0 != ( mFlags & UI_DRAG_ENABLE );
}

void UIDragable::dragEnable( const bool& enable ) {
	writeFlag( UI_DRAG_ENABLE, true == enable );
}

bool UIDragable::dragging() const {
	return 0 != ( mControlFlags & UI_CTRL_FLAG_DRAGGING );
}

void UIDragable::dragging( const bool& dragging ) {
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

void UIDragable::dragButton( const Uint32& Button ) {
	mDragButton = Button;
}

const Uint32& UIDragable::dragButton() const {
	return mDragButton;
}

}}
