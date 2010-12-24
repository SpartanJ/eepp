#include "cuidropdownlist.hpp"
#include "cuimanager.hpp"

namespace EE { namespace UI {

cUIDropDownList::cUIDropDownList( cUIDropDownList::CreateParams& Params ) :
	cUITextInput( Params ),
	mListBox( Params.ListBox ),
	mMinNumVisibleItems( Params.MinNumVisibleItems ),
	mPopUpToMainControl( Params.PopUpToMainControl )
{
	mType |= UI_TYPE_GET( UI_TYPE_DROPDOWNLIST );

	if ( NULL == mListBox ) {
		cUIListBox::CreateParams LBParams;
		LBParams.Size 				= eeSize( mSize.Width(), mMinNumVisibleItems * mSize.Height() );
		LBParams.Flags 				= UI_CLIP_ENABLE | UI_AUTO_PADDING;
		LBParams.FontSelectedColor	= eeColorA( 255, 255, 255, 255 );
		mListBox = eeNew( cUIListBox, ( LBParams ) );
	}

	mListBox->Enabled( false );
	mListBox->Visible( false );

	AllowEditing( false );

	ApplyDefaultTheme();

	mListBox->AddEventListener( cUIEvent::EventOnComplexControlFocusLoss, cb::Make1( this, &cUIDropDownList::OnListBoxFocusLoss ) );
	mListBox->AddEventListener( cUIEvent::EventOnSelected, cb::Make1( this, &cUIDropDownList::OnItemSelected ) );
	mListBox->AddEventListener( cUIEvent::EventOnItemClicked, cb::Make1( this, &cUIDropDownList::OnItemClicked ) );
	mListBox->AddEventListener( cUIEvent::EventOnItemKeyDown, cb::Make1( this, &cUIDropDownList::OnItemKeyDown ) );
	mListBox->AddEventListener( cUIEvent::EventKeyDown, cb::Make1( this, &cUIDropDownList::OnItemKeyDown ) );
}

cUIDropDownList::~cUIDropDownList() {
}

void cUIDropDownList::SetTheme( cUITheme * Theme ) {
	cUIControl::SetTheme( Theme, "dropdownlist" );

	AutoPadding();

	OnSizeChange();
}

cUIListBox * cUIDropDownList::ListBox() const {
	return mListBox;
}

Uint32 cUIDropDownList::OnMouseClick( const eeVector2i& Pos, const Uint32 Flags ) {
	if ( Flags & EE_BUTTON_LMASK )
		ShowListBox();

	return 1;
}

void cUIDropDownList::ShowListBox() {
	if ( !mListBox->Visible() ) {
		if ( !mPopUpToMainControl )
			mListBox->Parent( Parent() );
		else
			mListBox->Parent( cUIManager::instance()->MainControl() );

		mListBox->ToFront();

		eeVector2i Pos = mScreenPos;
		Pos.y += mSize.Height();

		mListBox->Parent()->ScreenToControl( Pos );
		mListBox->Pos( Pos );

		eeRecti tPadding = mListBox->PaddingContainer();

		if ( mMinNumVisibleItems < mListBox->Count() )
			mListBox->Size( mSize.Width(), (Int32)( mMinNumVisibleItems * mSize.Height() ) + tPadding.Top );
		else
			mListBox->Size( mSize.Width(), (Int32)( mListBox->Count() * mSize.Height() ) + tPadding.Top );

		Show();

		mListBox->SetFocus();
	} else {
		Hide();
	}
}

void cUIDropDownList::OnItemKeyDown( const cUIEvent * Event ) {
	const cUIEventKey * KEvent = reinterpret_cast<const cUIEventKey*> ( Event );

	if ( KEvent->KeyCode() == KEY_RETURN )
		OnItemClicked( Event );
}

void cUIDropDownList::OnListBoxFocusLoss( const cUIEvent * Event ) {
	if ( cUIManager::instance()->FocusControl() != this ) {
		Hide();
	}
}

void cUIDropDownList::OnItemClicked( const cUIEvent * Event ) {
	Hide();
	SetFocus();
}

void cUIDropDownList::OnItemSelected( const cUIEvent * Event ) {
	mTextBuffer.Buffer( mListBox->GetItemSelectedText() );
}

void cUIDropDownList::Show() {
	mListBox->Enabled( true );
	mListBox->Visible( true );

	if ( 255.f == mListBox->Alpha() )
		mListBox->StartAlphaAnim( 0.f, 255.f, cUIThemeManager::instance()->ControlsFadeInTime() );
	else
		mListBox->CreateFadeIn( cUIThemeManager::instance()->ControlsFadeInTime() );
}

void cUIDropDownList::Hide() {
	if ( cUIThemeManager::instance()->DefaultEffectsEnabled() ) {
		mListBox->DisableFadeOut( cUIThemeManager::instance()->ControlsFadeOutTime() );
	} else {
		mListBox->Enabled( false );
		mListBox->Visible( false );
	}
}

void cUIDropDownList::Update() {
	cUITextInput::Update();

	if ( mEnabled && mVisible ) {
		if ( ( mControlFlags & UI_CTRL_FLAG_HAS_FOCUS ) )
			mListBox->ManageKeyboard();

		if ( IsMouseOver() ) {
			Uint32 Flags 			= cUIManager::instance()->GetInput()->ClickTrigger();

			if ( Flags & EE_BUTTONS_WUWD ) {
				if ( Flags & EE_BUTTON_WUMASK ) {
					mListBox->SelectPrev();
				} else if ( Flags & EE_BUTTON_WDMASK ) {
					mListBox->SelectNext();
				}
			}
		}
	}
}

}}
