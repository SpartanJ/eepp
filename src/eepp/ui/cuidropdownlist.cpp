#include <eepp/ui/cuidropdownlist.hpp>
#include <eepp/ui/cuimanager.hpp>

namespace EE { namespace UI {

cUIDropDownList::cUIDropDownList( cUIDropDownList::CreateParams& Params ) :
	cUITextInput( Params ),
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

		cUITheme * Theme = cUIThemeManager::instance()->DefaultTheme();

		if ( NULL != Theme ) {
			mListBox = Theme->CreateListBox( NULL, eeSize( mSize.Width(), mMinNumVisibleItems * mSize.Height() ),eeVector2i(), flags );
		} else {
			cUIListBox::CreateParams LBParams;
			LBParams.Size 				= eeSize( mSize.Width(), mMinNumVisibleItems * mSize.Height() );
			LBParams.Flags 				= flags;
			LBParams.FontSelectedColor	= eeColorA( 255, 255, 255, 255 );
			mListBox = eeNew( cUIListBox, ( LBParams ) );
		}
	}

	mListBox->Enabled( false );
	mListBox->Visible( false );

	mListBox->AddEventListener( cUIEvent::EventOnComplexControlFocusLoss, cb::Make1( this, &cUIDropDownList::OnListBoxFocusLoss ) );
	mListBox->AddEventListener( cUIEvent::EventOnItemSelected	, cb::Make1( this, &cUIDropDownList::OnItemSelected ) );
	mListBox->AddEventListener( cUIEvent::EventOnItemClicked, cb::Make1( this, &cUIDropDownList::OnItemClicked ) );
	mListBox->AddEventListener( cUIEvent::EventOnItemKeyDown, cb::Make1( this, &cUIDropDownList::OnItemKeyDown ) );
	mListBox->AddEventListener( cUIEvent::EventKeyDown		, cb::Make1( this, &cUIDropDownList::OnItemKeyDown ) );
	mListBox->AddEventListener( cUIEvent::EventOnControlClear, cb::Make1( this, &cUIDropDownList::OnControlClear ) );
}

cUIDropDownList::~cUIDropDownList() {
	DestroyListBox();
}

Uint32 cUIDropDownList::Type() const {
	return UI_TYPE_DROPDOWNLIST;
}

bool cUIDropDownList::IsType( const Uint32& type ) const {
	return cUIDropDownList::Type() == type ? true : cUITextInput::IsType( type );
}

void cUIDropDownList::SetTheme( cUITheme * Theme ) {
	cUIControl::SetThemeControl( Theme, "dropdownlist" );

	AutoSizeControl();

	AutoPadding();

	OnSizeChange();
}

void cUIDropDownList::OnSizeChange() {
	cUIComplexControl::OnSizeChange();
}

void cUIDropDownList::AutoSizeControl() {
	if ( mFlags & UI_AUTO_SIZE ) {
		Size( mSize.x, GetSkinSize().Height() );
	}
}

void cUIDropDownList::AutoSize() {
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

		eeVector2i Pos( mPos.x, mPos.y + mSize.Height() );

		if ( mPopUpToMainControl ) {
			Parent()->ControlToWorld( Pos );
		}

		mListBox->Pos( Pos );

		if ( mListBox->Count() ) {
			eeRecti tPadding = mListBox->PaddingContainer();

			eeFloat sliderValue = mListBox->VerticalScrollBar()->Value();

			if ( mMinNumVisibleItems < mListBox->Count() )
				mListBox->Size( mSize.Width(), (Int32)( mMinNumVisibleItems * mListBox->RowHeight() ) + tPadding.Top + tPadding.Bottom );
			else {
				mListBox->Size( mSize.Width(), (Int32)( mListBox->Count() * mListBox->RowHeight() ) + tPadding.Top + tPadding.Bottom );
			}

			mListBox->UpdateQuad();

			eeRectf aabb( mListBox->GetPolygon().ToAABB() );
			eeRecti aabbi( aabb.Left, aabb.Top, aabb.Right, aabb.Bottom );

			if ( !cUIManager::instance()->MainControl()->GetScreenRect().Contains( aabbi ) ) {
				Pos = eeVector2i( mPos.x, mPos.y );

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

void cUIDropDownList::OnControlClear( const cUIEvent * Event ) {
	Text( "" );
}

void cUIDropDownList::OnItemKeyDown( const cUIEvent * Event ) {
	const cUIEventKey * KEvent = reinterpret_cast<const cUIEventKey*> ( Event );

	if ( KEvent->KeyCode() == KEY_RETURN )
		OnItemClicked( Event );
}

void cUIDropDownList::OnListBoxFocusLoss( const cUIEvent * Event ) {
	if ( cUIManager::instance()->FocusControl() != this && !IsChild( cUIManager::instance()->FocusControl() ) ) {
		Hide();
	}
}

void cUIDropDownList::OnItemClicked( const cUIEvent * Event ) {
	Hide();
	SetFocus();
}

void cUIDropDownList::OnItemSelected( const cUIEvent * Event ) {
	Text( mListBox->GetItemSelectedText() );

	cUIMessage Msg( this, cUIMessage::MsgSelected, mListBox->GetItemSelectedIndex() );
	MessagePost( &Msg );

	SendCommonEvent( cUIEvent::EventOnItemSelected );
}

void cUIDropDownList::Show() {
	mListBox->Enabled( true );
	mListBox->Visible( true );

	if ( cUIThemeManager::instance()->DefaultEffectsEnabled() ) {
		if ( 255.f == mListBox->Alpha() )
			mListBox->StartAlphaAnim( 0.f, 255.f, cUIThemeManager::instance()->ControlsFadeInTime() );
		else
			mListBox->CreateFadeIn( cUIThemeManager::instance()->ControlsFadeInTime() );
	}
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
	if ( mEnabled && mVisible ) {
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

	cUITextInput::Update();
}

Uint32 cUIDropDownList::OnKeyDown( const cUIEventKey &Event ) {
	mListBox->OnKeyDown( Event );

	return cUITextInput::OnKeyDown( Event );
}

void cUIDropDownList::DestroyListBox() {
	if ( !cUIManager::instance()->IsShootingDown() ) {
		mListBox->Close();
	}
}

}}
