#include <eepp/ui/uigridcell.hpp>
#include <eepp/ui/uigenericgrid.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UIGridCell::UIGridCell( UIGridCell::CreateParams& Params ) :
	UIComplexControl( Params )
{
	mCells.resize( GridParent()->CollumnsCount(), NULL );

	ApplyDefaultTheme();
}

UIGridCell::~UIGridCell() {
	if ( UIManager::instance()->FocusControl() == this )
		mParentCtrl->SetFocus();

	if ( UIManager::instance()->OverControl() == this )
		UIManager::instance()->OverControl( mParentCtrl );
}

void UIGridCell::SetTheme( UITheme * Theme ) {
	UIControl::SetThemeControl( Theme, "gridcell" );
}

UIGenericGrid * UIGridCell::GridParent() const {
	return reinterpret_cast<UIGenericGrid*> ( mParentCtrl->Parent() );
}

void UIGridCell::Cell( const Uint32& CollumnIndex, UIControl * Ctrl ) {
	eeASSERT( CollumnIndex < GridParent()->CollumnsCount() );

	UIGenericGrid * P = GridParent();

	mCells[ CollumnIndex ] = Ctrl;

	if ( Ctrl->Parent() != this )
		Ctrl->Parent( this );

	Ctrl->Pos		( P->GetCellPos( CollumnIndex )		, 0					);
	Ctrl->Size		( P->CollumnWidth( CollumnIndex )	, P->RowHeight()	);

	Ctrl->Visible( true );
	Ctrl->Enabled( true );
}

UIControl * UIGridCell::Cell( const Uint32& CollumnIndex ) const {
	eeASSERT( CollumnIndex < GridParent()->CollumnsCount() );

	return mCells[ CollumnIndex ];
}

void UIGridCell::FixCell() {
	AutoSize();

	UIGenericGrid * P = GridParent();

	for ( Uint32 i = 0; i < mCells.size(); i++ ) {
		mCells[i]->Pos		( P->GetCellPos( i )	, 0					);
		mCells[i]->Size		( P->CollumnWidth( i )	, P->RowHeight()	);
	}
}

void UIGridCell::Update() {
	if ( mEnabled && mVisible ) {
		UIGenericGrid * MyParent 	= reinterpret_cast<UIGenericGrid*> ( Parent()->Parent() );
		Uint32 Flags				= UIManager::instance()->GetInput()->ClickTrigger();

		if ( NULL != MyParent && MyParent->Alpha() != mAlpha ) {
			Alpha( MyParent->Alpha() );

			for ( Uint32 i = 0; i < mCells.size(); i++ ) {
				if ( NULL != mCells[i] && mCells[i]->IsAnimated() ) {
					reinterpret_cast<UIControlAnim*>( mCells[i] )->Alpha( MyParent->Alpha() );
				}
			}
		}

		if ( IsMouseOverMeOrChilds() ) {
			if ( ( Flags & EE_BUTTONS_WUWD ) && MyParent->VerticalScrollBar()->Visible() ) {
				MyParent->VerticalScrollBar()->Slider()->ManageClick( Flags );
			}
		}
	}

	UIComplexControl::Update();
}

void UIGridCell::Select() {
	UIGenericGrid * MyParent 	= reinterpret_cast<UIGenericGrid*> ( Parent()->Parent() );

	if ( MyParent->GetItemSelected() != this ) {
		if ( NULL != MyParent->GetItemSelected() )
			MyParent->GetItemSelected()->Unselect();

		bool wasSelected = 0 != ( mControlFlags & UI_CTRL_FLAG_SELECTED );

		SetSkinState( UISkinState::StateSelected );

		mControlFlags |= UI_CTRL_FLAG_SELECTED;

		MyParent->mSelected = MyParent->GetItemIndex( this );

		if ( !wasSelected ) {
			MyParent->OnSelected();
		}
	}
}

void UIGridCell::Unselect() {
	if ( mControlFlags & UI_CTRL_FLAG_SELECTED )
		mControlFlags &= ~UI_CTRL_FLAG_SELECTED;

	SetSkinState( UISkinState::StateNormal );
}

bool UIGridCell::Selected() const {
	return 0 != ( mControlFlags & UI_CTRL_FLAG_SELECTED );
}

Uint32 UIGridCell::OnMouseExit( const Vector2i& Pos, const Uint32 Flags ) {
	UIControl::OnMouseExit( Pos, Flags );

	if ( mControlFlags & UI_CTRL_FLAG_SELECTED )
		SetSkinState( UISkinState::StateSelected );

	return 1;
}

Uint32 UIGridCell::OnMessage( const UIMessage * Msg ) {
	switch( Msg->Msg() ) {
		case UIMessage::MsgMouseEnter:
		{
			OnMouseEnter( Vector2i(), Msg->Flags() );
			break;
		}
		case UIMessage::MsgMouseExit:
		{
			OnMouseExit( Vector2i(), Msg->Flags() );
			break;
		}
		case UIMessage::MsgClick:
		{
			if ( Msg->Flags() & EE_BUTTONS_LRM ) {
				Select();

				UIMessage tMsg( this, UIMessage::MsgCellClicked, Msg->Flags() );

				MessagePost( &tMsg );

				return 1;
			}
		}
	}

	return 0;
}

void UIGridCell::AutoSize() {
	UIGenericGrid * MyParent 	= reinterpret_cast<UIGenericGrid*> ( Parent()->Parent() );

	Size( MyParent->mTotalWidth, MyParent->mRowHeight );
}

void UIGridCell::OnStateChange() {
	if ( Selected() && mSkinState->GetState() != UISkinState::StateSelected ) {
		SetSkinState( UISkinState::StateSelected );
	}
}

}}
