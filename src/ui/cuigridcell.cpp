#include "cuigridcell.hpp"
#include "cuigenericgrid.hpp"
#include "cuimanager.hpp"

namespace EE { namespace UI {

cUIGridCell::cUIGridCell( cUIGridCell::CreateParams& Params ) :
	cUIComplexControl( Params )
{
	mCells.resize( GridParent()->CollumnsCount(), NULL );

	ApplyDefaultTheme();
}

cUIGridCell::~cUIGridCell() {
	if ( cUIManager::instance()->FocusControl() == this )
		mParentCtrl->SetFocus();

	if ( cUIManager::instance()->OverControl() == this )
		cUIManager::instance()->OverControl( mParentCtrl );
}

void cUIGridCell::SetTheme( cUITheme * Theme ) {
	cUIControl::SetTheme( Theme, "gridcell" );
}

cUIGenericGrid * cUIGridCell::GridParent() const {
	return reinterpret_cast<cUIGenericGrid*> ( mParentCtrl->Parent() );
}

void cUIGridCell::Cell( const Uint32& CollumnIndex, cUIControl * Ctrl ) {
	eeASSERT( CollumnIndex < GridParent()->CollumnsCount() );

	cUIGenericGrid * P = GridParent();

	mCells[ CollumnIndex ] = Ctrl;

	if ( Ctrl->Parent() != this )
		Ctrl->Parent( this );

	Ctrl->Pos		( P->GetCellPos( CollumnIndex )		, 0					);
	Ctrl->Size		( P->CollumnWidth( CollumnIndex )	, P->RowHeight()	);

	Ctrl->Visible( true );
	Ctrl->Enabled( true );
}

cUIControl * cUIGridCell::Cell( const Uint32& CollumnIndex ) const {
	eeASSERT( CollumnIndex < GridParent()->CollumnsCount() );

	return mCells[ CollumnIndex ];
}

void cUIGridCell::FixCell() {
	AutoSize();

	cUIGenericGrid * P = GridParent();

	for ( Uint32 i = 0; i < mCells.size(); i++ ) {
		mCells[i]->Pos		( P->GetCellPos( i )	, 0					);
		mCells[i]->Size		( P->CollumnWidth( i )	, P->RowHeight()	);
	}
}

void cUIGridCell::Update() {
	if ( mEnabled && mVisible ) {
		cUIGenericGrid * MyParent 	= reinterpret_cast<cUIGenericGrid*> ( Parent()->Parent() );
		Uint32 Flags				= cUIManager::instance()->GetInput()->ClickTrigger();

		if ( NULL != MyParent && MyParent->Alpha() != mAlpha ) {
			Alpha( MyParent->Alpha() );

			for ( Uint32 i = 0; i < mCells.size(); i++ ) {
				if ( NULL != mCells[i] && mCells[i]->IsAnimated() ) {
					reinterpret_cast<cUIControlAnim*>( mCells[i] )->Alpha( MyParent->Alpha() );
				}
			}
		}

		if ( IsMouseOverMeOrChilds() ) {
			if ( ( Flags & EE_BUTTONS_WUWD ) && MyParent->VerticalScrollBar()->Visible() ) {
				MyParent->VerticalScrollBar()->Slider()->ManageClick( Flags );
			}
		}
	}

	cUIComplexControl::Update();
}

void cUIGridCell::Select() {
	cUIGenericGrid * MyParent 	= reinterpret_cast<cUIGenericGrid*> ( Parent()->Parent() );

	if ( MyParent->GetItemSelected() != this ) {
		if ( NULL != MyParent->GetItemSelected() )
			MyParent->GetItemSelected()->Unselect();

		bool wasSelected = 0 != ( mControlFlags & UI_CTRL_FLAG_SELECTED );

		SetSkinState( cUISkinState::StateSelected );

		mControlFlags |= UI_CTRL_FLAG_SELECTED;

		MyParent->mSelected = MyParent->GetItemIndex( this );

		if ( !wasSelected ) {
			MyParent->OnSelected();
		}
	}
}

void cUIGridCell::Unselect() {
	if ( mControlFlags & UI_CTRL_FLAG_SELECTED )
		mControlFlags &= ~UI_CTRL_FLAG_SELECTED;

	SetSkinState( cUISkinState::StateNormal );
}

bool cUIGridCell::Selected() const {
	return 0 != ( mControlFlags & UI_CTRL_FLAG_SELECTED );
}

Uint32 cUIGridCell::OnMouseExit( const eeVector2i& Pos, const Uint32 Flags ) {
	cUIControl::OnMouseExit( Pos, Flags );

	if ( mControlFlags & UI_CTRL_FLAG_SELECTED )
		SetSkinState( cUISkinState::StateSelected );

	return 1;
}

Uint32 cUIGridCell::OnMessage( const cUIMessage * Msg ) {
	switch( Msg->Msg() ) {
		case cUIMessage::MsgMouseEnter:
		{
			OnMouseEnter( eeVector2i(), Msg->Flags() );
			break;
		}
		case cUIMessage::MsgMouseExit:
		{
			OnMouseExit( eeVector2i(), Msg->Flags() );
			break;
		}
		case cUIMessage::MsgClick:
		{
			if ( Msg->Flags() & EE_BUTTONS_LRM ) {
				Select();

				cUIMessage tMsg( this, cUIMessage::MsgCellClicked, Msg->Flags() );

				MessagePost( &tMsg );

				return 1;
			}
		}
	}

	return 0;
}

void cUIGridCell::AutoSize() {
	cUIGenericGrid * MyParent 	= reinterpret_cast<cUIGenericGrid*> ( Parent()->Parent() );

	Size( MyParent->mTotalWidth, MyParent->mRowHeight );
}

}}
