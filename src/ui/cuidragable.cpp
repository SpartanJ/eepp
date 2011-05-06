#include "cuidragable.hpp"
#include "cuimanager.hpp"
#include "../window/cinput.hpp"

namespace EE { namespace UI {

cUIDragable::cUIDragable( const cUIControl::CreateParams& Params ) :
	cUIControl( Params ),
	mDragButton( EE_BUTTON_LMASK )
{
	mType |= UI_TYPE_GET(UI_TYPE_CONTROL_DRAGABLE);
}

cUIDragable::~cUIDragable() {
}

Uint32 cUIDragable::OnMouseDown( const eeVector2i& Pos, const Uint32 Flags ) {
	if ( !( cUIManager::instance()->LastPressTrigger() & mDragButton ) && ( Flags & mDragButton ) && DragEnable() && !Dragging() ) {
		Dragging( true );
		mDragPoint = Pos;
	}

	cUIControl::OnMouseDown( Pos, Flags );
	return 1;
}

Uint32 cUIDragable::OnMouseUp( const eeVector2i& Pos, const Uint32 Flags ) {
	if ( DragEnable() && Dragging() && ( Flags & mDragButton ) ) {
		Dragging( false );
	}

	cUIControl::OnMouseUp( Pos, Flags );
	return 1;
}

const eeVector2i& cUIDragable::DragPoint() const {
	return mDragPoint;
}

void cUIDragable::DragPoint( const eeVector2i& Point ) {
	mDragPoint = Point;
}

void cUIDragable::Update() {
	cUIControl::Update();

	if ( !DragEnable() )
		return;

	if ( Dragging() ) {
		if ( !( cUIManager::instance()->PressTrigger() & mDragButton ) ) {
			Dragging( false );
			return;
		}

		eeVector2i Pos( cUIManager::instance()->GetMousePos() );

		if ( mDragPoint != Pos ) {
			mPos += -( mDragPoint - Pos );

			mDragPoint = Pos;

			OnPosChange();
		}
	}
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
	WriteCtrlFlag( UI_CTRL_FLAG_DRAGGING_POS, true == dragging );
}

void cUIDragable::DragButton( const Uint32& Button ) {
	mDragButton = Button;
}

const Uint32& cUIDragable::DragButton() const {
	return mDragButton;
}

}}
