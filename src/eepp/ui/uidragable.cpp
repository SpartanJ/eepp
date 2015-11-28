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

Uint32 UIDragable::Type() const {
	return UI_TYPE_CONTROL_DRAGABLE;
}

bool UIDragable::IsType( const Uint32& type ) const {
	return UIDragable::Type() == type ? true : UIControl::IsType( type );
}

Uint32 UIDragable::OnMouseDown( const Vector2i& Pos, const Uint32 Flags ) {
	if ( !( UIManager::instance()->LastPressTrigger() & mDragButton ) && ( Flags & mDragButton ) && DragEnable() && !Dragging() ) {
		Dragging( true );
		mDragPoint = Pos;
	}

	UIControl::OnMouseDown( Pos, Flags );
	return 1;
}

Uint32 UIDragable::OnMouseUp( const Vector2i& Pos, const Uint32 Flags ) {
	if ( DragEnable() && Dragging() && ( Flags & mDragButton ) ) {
		Dragging( false );
	}

	UIControl::OnMouseUp( Pos, Flags );
	return 1;
}

const Vector2i& UIDragable::DragPoint() const {
	return mDragPoint;
}

void UIDragable::DragPoint( const Vector2i& Point ) {
	mDragPoint = Point;
}

void UIDragable::Update() {
	UIControl::Update();

	if ( !DragEnable() )
		return;

	if ( Dragging() ) {
		if ( !( UIManager::instance()->PressTrigger() & mDragButton ) ) {
			Dragging( false );
			UIManager::instance()->SetControlDragging( false );
			return;
		}

		Vector2i Pos( UIManager::instance()->GetMousePos() );

		if ( mDragPoint != Pos ) {
			if ( OnDrag( Pos ) ) {
				mPos += -( mDragPoint - Pos );

				mDragPoint = Pos;

				OnPosChange();

				UIManager::instance()->SetControlDragging( true );
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

bool UIDragable::DragEnable() const {
	return 0 != ( mFlags & UI_DRAG_ENABLE );
}

void UIDragable::DragEnable( const bool& enable ) {
	WriteFlag( UI_DRAG_ENABLE, true == enable );
}

bool UIDragable::Dragging() const {
	return 0 != ( mControlFlags & UI_CTRL_FLAG_DRAGGING );
}

void UIDragable::Dragging( const bool& dragging ) {
	WriteCtrlFlag( UI_CTRL_FLAG_DRAGGING, true == dragging );

	if ( dragging ) {
		UIMessage tMsg( this, UIMessage::MsgDragStart, 0 );
		MessagePost( &tMsg );

		OnDragStart( UIManager::instance()->GetMousePos() );
	} else {
		UIMessage tMsg( this, UIMessage::MsgDragEnd, 0 );
		MessagePost( &tMsg );

		OnDragEnd( UIManager::instance()->GetMousePos() );
	}
}

void UIDragable::DragButton( const Uint32& Button ) {
	mDragButton = Button;
}

const Uint32& UIDragable::DragButton() const {
	return mDragButton;
}

}}
