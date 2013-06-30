#include <eepp/ui/cuiwinmenu.hpp>
#include <eepp/ui/cuimanager.hpp>
#include <eepp/graphics/csubtexture.hpp>

namespace EE { namespace UI {

cUIWinMenu::cUIWinMenu( const cUIWinMenu::CreateParams& Params ) :
	cUIComplexControl( Params ),
	mFont( Params.Font ),
	mFontColor( Params.FontColor ),
	mFontShadowColor( Params.FontShadowColor ),
	mFontOverColor( Params.FontOverColor ),
	mFontSelectedColor( Params.FontSelectedColor ),
	mCurrentMenu( NULL ),
	mMarginBetweenButtons( Params.MarginBetweenButtons ),
	mButtonMargin( Params.ButtonMargin ),
	mFirstButtonMargin( Params.FirstButtonMargin ),
	mMenuHeight( Params.MenuHeight )
{
	if ( !(mFlags & UI_ANCHOR_RIGHT) )
		mFlags |= UI_ANCHOR_RIGHT;

	Size( Parent()->Size().Width(), mMenuHeight );

	UpdateAnchorsDistances();

	ApplyDefaultTheme();
}

cUIWinMenu::~cUIWinMenu() {
	DestroyMenues();
}

Uint32 cUIWinMenu::Type() const {
	return UI_TYPE_WINMENU;
}

bool cUIWinMenu::IsType( const Uint32& type ) const {
	return cUIWinMenu::Type() == type ? true : cUIComplexControl::IsType( type );
}

void cUIWinMenu::AddMenuButton( const String& ButtonText, cUIPopUpMenu * Menu ) {
	eeASSERT( NULL != Menu );

	cUISelectButton::CreateParams ButtonParams;
	ButtonParams.Parent( this );
	ButtonParams.Flags				= UI_HALIGN_CENTER | UI_VALIGN_CENTER;

	if ( mFlags & UI_DRAW_SHADOW )
		ButtonParams.Flags |= UI_DRAW_SHADOW;

	ButtonParams.Font				= mFont;
	ButtonParams.FontColor			= mFontColor;
	ButtonParams.FontShadowColor	= mFontShadowColor;
	ButtonParams.FontOverColor		= mFontOverColor;

	cUISelectButton * Button = eeNew( cUISelectButton, ( ButtonParams ) );
	Button->Text( ButtonText );
	Button->Visible( true );
	Button->Enabled( true );
	Button->SetThemeControl( mSkinState->GetSkin()->Theme(), "winmenubutton" );

	Menu->Visible( false );
	Menu->Enabled( false );
	Menu->Parent( cUIManager::instance()->MainControl() );
	Menu->AddEventListener( cUIEvent::EventOnComplexControlFocusLoss, cb::Make1( this, &cUIWinMenu::OnMenuFocusLoss ) );

	mButtons.push_back( std::make_pair( Button, Menu ) );

	RefreshButtons();
}

void cUIWinMenu::SetTheme( cUITheme * Theme ) {
	cUIComplexControl::SetThemeControl( Theme, "winmenu" );

	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
		it->first->SetThemeControl( Theme, "winmenubutton" );
	}

	if ( 0 == mMenuHeight && NULL != GetSkin() && NULL != GetSkin()->GetSubTexture( cUISkinState::StateNormal ) ) {
		mMenuHeight = GetSkin()->GetSubTexture( cUISkinState::StateNormal )->Size().Height();

		Size( Parent()->Size().Width(), mMenuHeight );

		UpdateAnchorsDistances();
	}
}

void cUIWinMenu::RemoveMenuButton( const String& ButtonText ) {
	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
		if ( it->first->Text() == ButtonText ) {
			it->second->Close();

			mButtons.erase( it );

			break;
		}
	}
}

cUISelectButton * cUIWinMenu::GetButton( const String& ButtonText ) {
	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
		if ( it->first->Text() == ButtonText ) {
			return it->first;
		}
	}

	return NULL;
}

cUIPopUpMenu * cUIWinMenu::GetPopUpMenu( const String& ButtonText ) {
	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
		if ( it->first->Text() == ButtonText ) {
			return it->second;
		}
	}

	return NULL;
}

void cUIWinMenu::RefreshButtons() {
	Uint32 xpos = mFirstButtonMargin;
	Int32 h = 0, th = 0, ycenter = 0;

	if ( NULL != GetSkin() ) {
		cSubTexture * subTexture = GetSkin()->GetSubTexture( cUISkinState::StateNormal );

		if ( NULL != subTexture ) {
			h = subTexture->Size().Height();

			if ( mButtons.begin() != mButtons.end() ) {
				cUISelectButton * tbut = mButtons.begin()->first;

				if ( NULL != tbut->GetSkin() ) {
					cSubTexture * tSubTexture2 = tbut->GetSkin()->GetSubTexture( cUISkinState::StateSelected );

					if ( NULL != tSubTexture2 )  {
						th = tSubTexture2->Size().Height();

						switch ( VAlignGet( Flags() ) ) {
							case UI_VALIGN_CENTER:
								ycenter = ( h - th ) / 2;
								break;
							case UI_VALIGN_BOTTOM:
								ycenter = ( h - th );
								break;
							case UI_VALIGN_TOP:
								ycenter = 0;
								break;
						}
					}
				}
			}
		}
	}

	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
		cUISelectButton * pbut	= it->first;
		cUITextBox * tbox		= pbut->TextBox();

		pbut->Size( tbox->GetTextWidth() + mButtonMargin, Size().Height() );
		pbut->Pos( xpos, ycenter );

		xpos += pbut->Size().Width() + mMarginBetweenButtons;
	}
}

Uint32 cUIWinMenu::OnMessage( const cUIMessage * Msg ) {
	switch ( Msg->Msg() ) {
		case cUIMessage::MsgMouseUp:
		case cUIMessage::MsgMouseEnter:
		{
			if ( Msg->Sender()->IsType( UI_TYPE_SELECTBUTTON ) ) {
				cUISelectButton * tbut	= reinterpret_cast<cUISelectButton*> ( Msg->Sender() );
				cUIPopUpMenu * tpop		= GetMenuFromButton( tbut );

				eeVector2i pos( tbut->Pos().x, tbut->Size().Height() );
				tbut->ControlToScreen( pos );
				tpop->Pos( pos );

				if ( Msg->Msg() == cUIMessage::MsgMouseEnter ) {
					if ( NULL != mCurrentMenu ) {
						mCurrentMenu = tpop;

						tbut->Select();
						tpop->Show();
					}
				} else {
					if ( Msg->Flags() & EE_BUTTON_LMASK ) {
						mCurrentMenu = tpop;

						tbut->Select();
						tpop->Show();
					}
				}

				return 1;
			}

			break;
		}
		case cUIMessage::MsgSelected:
		{
			for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
				if ( it->first != Msg->Sender() ) {
					it->first->Unselect();
				}
			}

			return 1;
		}
		case cUIMessage::MsgFocusLoss:
		{
			cUIControl * FocusCtrl = cUIManager::instance()->FocusControl();

			if ( !IsParentOf( FocusCtrl ) && !IsPopUpMenuChild( FocusCtrl ) ) {
				OnComplexControlFocusLoss();
			}

			return 1;
		}
	}

	return 0;
}

void cUIWinMenu::UnselectButtons() {
	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
		it->first->Unselect();
	}
}

cUIPopUpMenu * cUIWinMenu::GetMenuFromButton( cUISelectButton * Button ) {
	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
		if ( it->first == Button ) {
			return it->second;
		}
	}

	return NULL;
}

bool cUIWinMenu::IsPopUpMenuChild( cUIControl * Ctrl ) {
	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
		if ( it->second == Ctrl || it->second->IsParentOf( Ctrl ) ) {
			return true;
		}
	}

	return false;
}

void cUIWinMenu::OnMenuFocusLoss( const cUIEvent * Event ) {
	cUIControl * FocusCtrl = cUIManager::instance()->FocusControl();

	if ( !IsParentOf( FocusCtrl ) && !IsPopUpMenuChild( FocusCtrl ) ) {
		OnComplexControlFocusLoss();
	}
}

void cUIWinMenu::OnComplexControlFocusLoss() {
	cUIComplexControl::OnComplexControlFocusLoss();

	if ( NULL != mCurrentMenu ) {
		mCurrentMenu->Hide();

		mCurrentMenu = NULL;
	}

	UnselectButtons();
}


void cUIWinMenu::FontColor( const eeColorA& Color ) {
	mFontColor = Color;
}

const eeColorA& cUIWinMenu::FontColor() const {
	return mFontColor;
}

void cUIWinMenu::FontOverColor( const eeColorA& Color ) {
	mFontOverColor = Color;
}

const eeColorA& cUIWinMenu::FontOverColor() const {
	return mFontOverColor;
}

void cUIWinMenu::FontSelectedColor( const eeColorA& Color ) {
	mFontSelectedColor = Color;
}

const eeColorA& cUIWinMenu::FontSelectedColor() const {
	return mFontSelectedColor;
}

cFont * cUIWinMenu::Font() const {
	return mFont;
}

void cUIWinMenu::DestroyMenues() {
	if ( !cUIManager::instance()->IsShootingDown() ) {
		for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
			it->second->Close();
		}
	}
}

}}
