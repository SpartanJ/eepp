#include <eepp/ui/cuidragable.hpp>
#include <eepp/ui/cuimanager.hpp>
#include <eepp/window/cinput.hpp>

namespace EE { namespace UI {

cUIDragable::cUIDragable( const cUIControl::CreateParams& Params ) :
	cUIControl( Params ),
	mDragButton( EE_BUTTON_LMASK )
{
	mControlFlags |= UI_CTRL_FLAG_DRAGABLE;
}

cUIDragable::~cUIDragable() {
}

Uint32 cUIDragable::Type() const {
	return UI_TYPE_CONTROL_DRAGABLE;
}

bool cUIDragable::IsType( const Uint32& type ) const {
	return cUIDragable::Type() == type ? true : cUIControl::IsType( type );
}

Uint32 cUIDragable::OnMouseDown( const Vector2i& Pos, const Uint32 Flags ) {
	if ( !( cUIManager::instance()->LastPressTrigger() & mDragButton ) && ( Flags & mDragButton ) && DragEnable() && !Dragging() ) {
		Dragging( true );
		mDragPoint = Pos;
	}

	cUIControl::OnMouseDown( Pos, Flags );
	return 1;
}

Uint32 cUIDragable::OnMouseUp( const Vector2i& Pos, const Uint32 Flags ) {
	if ( DragEnable() && Dragging() && ( Flags & mDragButton ) ) {
		Dragging( false );
	}

	cUIControl::OnMouseUp( Pos, Flags );
	return 1;
}

const Vector2i& cUIDragable::DragPoint() const {
	return mDragPoint;
}

void cUIDragable::DragPoint( const Vector2i& Point ) {
	mDragPoint = Point;
}

void cUIDragable::Update() {
	cUIControl::Update();

	if ( !DragEnable() )
		return;

	if ( Dragging() ) {
		if ( !( cUIManager::instance()->PressTrigger() & mDragButton ) ) {
			Dragging( false );
			cUIManager::instance()->SetControlDragging( false );
			return;
		}

		Vector2i Pos( cUIManager::instance()->GetMousePos() );

		if ( mDragPoint != Pos ) {
			if ( OnDrag( Pos ) ) {
				mPos += -( mDragPoint - Pos );

				mDragPoint = Pos;

				OnPosChange();

				cUIManager::instance()->SetControlDragging( true );
			}
		}
	}
}

Uint32 cUIDragable::OnDrag( const Vector2i& Pos ) {
	return 1;
}

Uint32 cUIDragable::OnDragStart( const Vector2i& Pos ) {
	return 1;
}

Uint32 cUIDragable::OnDragEnd( const Vector2i& Pos ) {
	return 1;
}

bool cUIDragable::DragEnable() const {
	return 0 != ( mFlags & UI_DRAG_ENABLE );
}

void cUIDragable::DragEnable( const bool& enable ) {
	WriteFlag( UI_DRAG_ENABLE, true == enable );
}

bool cUIDragable::Dragging() const {
	return 0 != ( mControlFlags & UI_CTRL_FLAG_DRAGGING );
}

void cUIDragable::Dragging( const bool& dragging ) {
	WriteCtrlFlag( UI_CTRL_FLAG_DRAGGING, true == dragging );

	if ( dragging ) {
		cUIMessage tMsg( this, cUIMessage::MsgDragStart, 0 );
		MessagePost( &tMsg );

		OnDragStart( cUIManager::instance()->GetMousePos() );
	} else {
		cUIMessage tMsg( this, cUIMessage::MsgDragEnd, 0 );
		MessagePost( &tMsg );

		OnDragEnd( cUIManager::instance()->GetMousePos() );
	}
}

void cUIDragable::DragButton( const Uint32& Button ) {
	mDragButton = Button;
}

const Uint32& cUIDragable::DragButton() const {
	return mDragButton;
}

}}
