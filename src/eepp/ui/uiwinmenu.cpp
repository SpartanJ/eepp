#include <eepp/ui/uiwinmenu.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/graphics/subtexture.hpp>

namespace EE { namespace UI {

UIWinMenu::UIWinMenu( const UIWinMenu::CreateParams& Params ) :
	UIComplexControl( Params ),
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

UIWinMenu::~UIWinMenu() {
	DestroyMenues();
}

Uint32 UIWinMenu::Type() const {
	return UI_TYPE_WINMENU;
}

bool UIWinMenu::IsType( const Uint32& type ) const {
	return UIWinMenu::Type() == type ? true : UIComplexControl::IsType( type );
}

void UIWinMenu::AddMenuButton( const String& ButtonText, UIPopUpMenu * Menu ) {
	eeASSERT( NULL != Menu );

	UISelectButton::CreateParams ButtonParams;
	ButtonParams.Parent( this );
	ButtonParams.Flags				= UI_HALIGN_CENTER | UI_VALIGN_CENTER;

	if ( mFlags & UI_DRAW_SHADOW )
		ButtonParams.Flags |= UI_DRAW_SHADOW;

	ButtonParams.Font				= mFont;
	ButtonParams.FontColor			= mFontColor;
	ButtonParams.FontShadowColor	= mFontShadowColor;
	ButtonParams.FontOverColor		= mFontOverColor;

	UISelectButton * Button = eeNew( UISelectButton, ( ButtonParams ) );
	Button->Text( ButtonText );
	Button->Visible( true );
	Button->Enabled( true );
	Button->SetThemeControl( mSkinState->GetSkin()->Theme(), "winmenubutton" );

	Menu->Visible( false );
	Menu->Enabled( false );
	Menu->Parent( Parent() );
	Menu->AddEventListener( UIEvent::EventOnComplexControlFocusLoss, cb::Make1( this, &UIWinMenu::OnMenuFocusLoss ) );

	mButtons.push_back( std::make_pair( Button, Menu ) );

	RefreshButtons();
}

void UIWinMenu::SetTheme( UITheme * Theme ) {
	UIComplexControl::SetThemeControl( Theme, "winmenu" );

	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
		it->first->SetThemeControl( Theme, "winmenubutton" );
	}

	if ( 0 == mMenuHeight && NULL != GetSkin() && NULL != GetSkin()->GetSubTexture( UISkinState::StateNormal ) ) {
		mMenuHeight = GetSkin()->GetSubTexture( UISkinState::StateNormal )->Size().Height();

		Size( Parent()->Size().Width(), mMenuHeight );

		UpdateAnchorsDistances();
	}
}

void UIWinMenu::RemoveMenuButton( const String& ButtonText ) {
	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
		if ( it->first->Text() == ButtonText ) {
			it->second->Close();

			mButtons.erase( it );

			break;
		}
	}
}

UISelectButton * UIWinMenu::GetButton( const String& ButtonText ) {
	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
		if ( it->first->Text() == ButtonText ) {
			return it->first;
		}
	}

	return NULL;
}

UIPopUpMenu * UIWinMenu::GetPopUpMenu( const String& ButtonText ) {
	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
		if ( it->first->Text() == ButtonText ) {
			return it->second;
		}
	}

	return NULL;
}

void UIWinMenu::RefreshButtons() {
	Uint32 xpos = mFirstButtonMargin;
	Int32 h = 0, th = 0, ycenter = 0;

	if ( NULL != GetSkin() ) {
		SubTexture * subTexture = GetSkin()->GetSubTexture( UISkinState::StateNormal );

		if ( NULL != subTexture ) {
			h = subTexture->Size().Height();

			if ( mButtons.begin() != mButtons.end() ) {
				UISelectButton * tbut = mButtons.begin()->first;

				if ( NULL != tbut->GetSkin() ) {
					SubTexture * tSubTexture2 = tbut->GetSkin()->GetSubTexture( UISkinState::StateSelected );

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
		UISelectButton * pbut	= it->first;
		UITextBox * tbox		= pbut->TextBox();

		pbut->Size( tbox->GetTextWidth() + mButtonMargin, Size().Height() );
		pbut->Pos( xpos, ycenter );

		xpos += pbut->Size().Width() + mMarginBetweenButtons;
	}
}

Uint32 UIWinMenu::OnMessage( const UIMessage * Msg ) {
	switch ( Msg->Msg() ) {
		case UIMessage::MsgMouseUp:
		case UIMessage::MsgMouseEnter:
		{
			if ( Msg->Sender()->IsType( UI_TYPE_SELECTBUTTON ) ) {
				UISelectButton * tbut	= reinterpret_cast<UISelectButton*> ( Msg->Sender() );
				UIPopUpMenu * tpop		= GetMenuFromButton( tbut );

				Vector2i pos( tbut->Pos().x, tbut->Pos().y + tbut->Size().Height() );
				tpop->Pos( pos );

				if ( Msg->Msg() == UIMessage::MsgMouseEnter ) {
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
		case UIMessage::MsgSelected:
		{
			for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
				if ( it->first != Msg->Sender() ) {
					it->first->Unselect();
				}
			}

			return 1;
		}
		case UIMessage::MsgFocusLoss:
		{
			UIControl * FocusCtrl = UIManager::instance()->FocusControl();

			if ( !IsParentOf( FocusCtrl ) && !IsPopUpMenuChild( FocusCtrl ) ) {
				OnComplexControlFocusLoss();
			}

			return 1;
		}
	}

	return 0;
}

void UIWinMenu::UnselectButtons() {
	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
		it->first->Unselect();
	}
}

UIPopUpMenu * UIWinMenu::GetMenuFromButton( UISelectButton * Button ) {
	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
		if ( it->first == Button ) {
			return it->second;
		}
	}

	return NULL;
}

bool UIWinMenu::IsPopUpMenuChild( UIControl * Ctrl ) {
	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
		if ( it->second == Ctrl || it->second->IsParentOf( Ctrl ) ) {
			return true;
		}
	}

	return false;
}

void UIWinMenu::OnMenuFocusLoss( const UIEvent * Event ) {
	UIControl * FocusCtrl = UIManager::instance()->FocusControl();

	if ( !IsParentOf( FocusCtrl ) && !IsPopUpMenuChild( FocusCtrl ) ) {
		OnComplexControlFocusLoss();
	}
}

void UIWinMenu::OnComplexControlFocusLoss() {
	UIComplexControl::OnComplexControlFocusLoss();

	if ( NULL != mCurrentMenu ) {
		mCurrentMenu->Hide();

		mCurrentMenu = NULL;
	}

	UnselectButtons();
}


void UIWinMenu::FontColor( const ColorA& Color ) {
	mFontColor = Color;
}

const ColorA& UIWinMenu::FontColor() const {
	return mFontColor;
}

void UIWinMenu::FontOverColor( const ColorA& Color ) {
	mFontOverColor = Color;
}

const ColorA& UIWinMenu::FontOverColor() const {
	return mFontOverColor;
}

void UIWinMenu::FontSelectedColor( const ColorA& Color ) {
	mFontSelectedColor = Color;
}

const ColorA& UIWinMenu::FontSelectedColor() const {
	return mFontSelectedColor;
}

Graphics::Font * UIWinMenu::Font() const {
	return mFont;
}

void UIWinMenu::DestroyMenues() {
	if ( !UIManager::instance()->IsShootingDown() ) {
		for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
			it->second->Close();
		}
	}
}

}}
