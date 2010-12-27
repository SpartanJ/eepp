#include "cuidragable.hpp"

namespace EE { namespace UI {

cUIDragable::cUIDragable( const cUIControl::CreateParams& Params ) :
	cUIControl( Params ),
	mDragEnable( false ),
	mDragging( false ),
	mDragButton( EE_BUTTON_LMASK )
{
	mType |= UI_TYPE_CONTROL_DRAGABLE;
}

cUIDragable::~cUIDragable() {
}

Uint32 cUIDragable::OnMouseDown( const eeVector2i& Pos, const Uint32 Flags ) {
	if ( !( cInput::instance()->LastPressTrigger() & mDragButton ) && ( Flags & mDragButton ) && mDragEnable && !mDragging ) {
		mDragging = true;
		mDragPoint = mDraggingPoint = Pos;
	}

	cUIControl::OnMouseDown( Pos, Flags );
	return 1;
}

Uint32 cUIDragable::OnMouseUp( const eeVector2i& Pos, const Uint32 Flags ) {
	if ( mDragEnable && mDragging && ( Flags & mDragButton ) )
		mDragging = false;

	cUIControl::OnMouseUp( Pos, Flags );
	return 1;
}

bool cUIDragable::Dragging() const {
	return mDragging;
}

void cUIDragable::Dragging( const bool& dragging ) {
	mDragging = dragging;
}

const eeVector2i& cUIDragable::DragPoint() const {
	return mDragPoint;
}

void cUIDragable::DragPoint( const eeVector2i& Point ) {
	mDragPoint = Point;
}

const eeVector2i& cUIDragable::DraggingPoint() const {
	return mDraggingPoint;
}

void cUIDragable::DraggingPoint( const eeVector2i& Point ) {
	mDraggingPoint = Point;
}

void cUIDragable::Update() {
	cUIControl::Update();

	if ( !mDragEnable )
		return;

	if ( mDragging ) {
		if ( !( cInput::instance()->PressTrigger() & mDragButton ) ) {
			mDragging = false;
			return;
		}

		eeVector2i Pos( cInput::instance()->GetMousePos() );

		if ( mDraggingPoint.x != Pos.x || mDraggingPoint.y != Pos.y ) {
			mDragPoint 		= mDraggingPoint;
			mDraggingPoint 	= Pos;

			mPos += ( mDragPoint - mDraggingPoint ) * (eeInt)-1;

			OnPosChange();
		} else
			mDragPoint = mDraggingPoint;
	}
}

const bool& cUIDragable::DragEnable() const {
	return mDragEnable;
}

void cUIDragable::DragEnable( const bool& enable ) {
	mDragEnable = enable;
}

void cUIDragable::DragButton( const Uint32& Button ) {
	mDragButton = Button;
}

const Uint32& cUIDragable::DragButton() const {
	return mDragButton;
}

}}
