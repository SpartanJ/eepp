#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UIDropDownList::UIDropDownList( UIDropDownList::CreateParams& Params ) :
	UITextInput( Params ),
	mListBox( Params.ListBox ),
	mMinNumVisibleItems( Params.MinNumVisibleItems ),
	mPopUpToMainControl( Params.PopUpToMainControl )
{
	AllowEditing( false );

	ApplyDefaultTheme();

	if ( NULL == mListBox ) {
		Uint32 flags = UI_CLIP_ENABLE | UI_AUTO_PADDING;

		if ( Params.Flags & UI_TOUCH_DRAG_ENABLED )
			flags |= UI_TOUCH_DRAG_ENABLED;

		if ( Params.Flags & UI_TEXT_SELECTION_ENABLED )
			flags |= UI_TEXT_SELECTION_ENABLED;

		UITheme * Theme = UIThemeManager::instance()->DefaultTheme();

		if ( NULL != Theme ) {
			mListBox = Theme->CreateListBox( NULL, Sizei( mSize.Width(), mMinNumVisibleItems * mSize.Height() ),Vector2i(), flags );
		} else {
			UIListBox::CreateParams LBParams;
			LBParams.Size 				= Sizei( mSize.Width(), mMinNumVisibleItems * mSize.Height() );
			LBParams.Flags 				= flags;
			LBParams.FontSelectedColor	= ColorA( 255, 255, 255, 255 );
			mListBox = eeNew( UIListBox, ( LBParams ) );
		}
	}

	mListBox->Enabled( false );
	mListBox->Visible( false );

	mListBox->AddEventListener( UIEvent::EventOnComplexControlFocusLoss, cb::Make1( this, &UIDropDownList::OnListBoxFocusLoss ) );
	mListBox->AddEventListener( UIEvent::EventOnItemSelected	, cb::Make1( this, &UIDropDownList::OnItemSelected ) );
	mListBox->AddEventListener( UIEvent::EventOnItemClicked, cb::Make1( this, &UIDropDownList::OnItemClicked ) );
	mListBox->AddEventListener( UIEvent::EventOnItemKeyDown, cb::Make1( this, &UIDropDownList::OnItemKeyDown ) );
	mListBox->AddEventListener( UIEvent::EventKeyDown		, cb::Make1( this, &UIDropDownList::OnItemKeyDown ) );
	mListBox->AddEventListener( UIEvent::EventOnControlClear, cb::Make1( this, &UIDropDownList::OnControlClear ) );
}

UIDropDownList::~UIDropDownList() {
	DestroyListBox();
}

Uint32 UIDropDownList::Type() const {
	return UI_TYPE_DROPDOWNLIST;
}

bool UIDropDownList::IsType( const Uint32& type ) const {
	return UIDropDownList::Type() == type ? true : UITextInput::IsType( type );
}

void UIDropDownList::SetTheme( UITheme * Theme ) {
	UIControl::SetThemeControl( Theme, "dropdownlist" );

	AutoSizeControl();

	AutoPadding();

	OnSizeChange();
}

void UIDropDownList::OnSizeChange() {
	UIComplexControl::OnSizeChange();
}

void UIDropDownList::AutoSizeControl() {
	if ( mFlags & UI_AUTO_SIZE ) {
		Size( mSize.x, GetSkinSize().Height() );
	}
}

void UIDropDownList::AutoSize() {
}

UIListBox * UIDropDownList::ListBox() const {
	return mListBox;
}

Uint32 UIDropDownList::OnMouseClick( const Vector2i& Pos, const Uint32 Flags ) {
	if ( Flags & EE_BUTTON_LMASK )
		ShowListBox();

	return 1;
}

void UIDropDownList::ShowListBox() {
	if ( !mListBox->Visible() ) {
		if ( !mPopUpToMainControl )
			mListBox->Parent( Parent() );
		else
			mListBox->Parent( UIManager::instance()->MainControl() );

		mListBox->ToFront();

		Vector2i Pos( mPos.x, mPos.y + mSize.Height() );

		if ( mPopUpToMainControl ) {
			Parent()->ControlToWorld( Pos );
		}

		mListBox->Pos( Pos );

		if ( mListBox->Count() ) {
			Recti tPadding = mListBox->PaddingContainer();

			Float sliderValue = mListBox->VerticalScrollBar()->Value();

			if ( mMinNumVisibleItems < mListBox->Count() )
				mListBox->Size( mSize.Width(), (Int32)( mMinNumVisibleItems * mListBox->RowHeight() ) + tPadding.Top + tPadding.Bottom );
			else {
				mListBox->Size( mSize.Width(), (Int32)( mListBox->Count() * mListBox->RowHeight() ) + tPadding.Top + tPadding.Bottom );
			}

			mListBox->UpdateQuad();

			Rectf aabb( mListBox->GetPolygon().ToAABB() );
			Recti aabbi( aabb.Left, aabb.Top, aabb.Right, aabb.Bottom );

			if ( !UIManager::instance()->MainControl()->GetScreenRect().Contains( aabbi ) ) {
				Pos = Vector2i( mPos.x, mPos.y );

				if ( mPopUpToMainControl ) {
					Parent()->ControlToWorld( Pos );
				}

				Pos.y -= mListBox->Size().Height();

				mListBox->Pos( Pos );
			}

			mListBox->VerticalScrollBar()->Value( sliderValue );

			Show();

			mListBox->SetFocus();
		}
	} else {
		Hide();
	}
}

void UIDropDownList::OnControlClear( const UIEvent * Event ) {
	Text( "" );
}

void UIDropDownList::OnItemKeyDown( const UIEvent * Event ) {
	const UIEventKey * KEvent = reinterpret_cast<const UIEventKey*> ( Event );

	if ( KEvent->KeyCode() == KEY_RETURN )
		OnItemClicked( Event );
}

void UIDropDownList::OnListBoxFocusLoss( const UIEvent * Event ) {
	if ( UIManager::instance()->FocusControl() != this && !IsChild( UIManager::instance()->FocusControl() ) ) {
		Hide();
	}
}

void UIDropDownList::OnItemClicked( const UIEvent * Event ) {
	Hide();
	SetFocus();
}

void UIDropDownList::OnItemSelected( const UIEvent * Event ) {
	Text( mListBox->GetItemSelectedText() );

	UIMessage Msg( this, UIMessage::MsgSelected, mListBox->GetItemSelectedIndex() );
	MessagePost( &Msg );

	SendCommonEvent( UIEvent::EventOnItemSelected );
}

void UIDropDownList::Show() {
	mListBox->Enabled( true );
	mListBox->Visible( true );

	if ( UIThemeManager::instance()->DefaultEffectsEnabled() ) {
		mListBox->StartAlphaAnim( 255.f == mListBox->Alpha() ? 0.f : mListBox->Alpha(), 255.f, UIThemeManager::instance()->ControlsFadeInTime() );
	}
}

void UIDropDownList::Hide() {
	if ( UIThemeManager::instance()->DefaultEffectsEnabled() ) {
		mListBox->DisableFadeOut( UIThemeManager::instance()->ControlsFadeOutTime() );
	} else {
		mListBox->Enabled( false );
		mListBox->Visible( false );
	}
}

void UIDropDownList::Update() {
	if ( mEnabled && mVisible ) {
		if ( IsMouseOver() ) {
			Uint32 Flags 			= UIManager::instance()->GetInput()->ClickTrigger();

			if ( Flags & EE_BUTTONS_WUWD ) {
				if ( Flags & EE_BUTTON_WUMASK ) {
					mListBox->SelectPrev();
				} else if ( Flags & EE_BUTTON_WDMASK ) {
					mListBox->SelectNext();
				}
			}
		}
	}

	UITextInput::Update();
}

Uint32 UIDropDownList::OnKeyDown( const UIEventKey &Event ) {
	mListBox->OnKeyDown( Event );

	return UITextInput::OnKeyDown( Event );
}

void UIDropDownList::DestroyListBox() {
	if ( !UIManager::instance()->IsShootingDown() ) {
		mListBox->Close();
	}
}

}}
