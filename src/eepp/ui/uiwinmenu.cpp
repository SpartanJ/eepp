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

	setSize( getParent()->getSize().getWidth(), mMenuHeight );

	updateAnchorsDistances();

	applyDefaultTheme();
}

UIWinMenu::~UIWinMenu() {
	destroyMenues();
}

Uint32 UIWinMenu::getType() const {
	return UI_TYPE_WINMENU;
}

bool UIWinMenu::isType( const Uint32& type ) const {
	return UIWinMenu::getType() == type ? true : UIComplexControl::isType( type );
}

void UIWinMenu::addMenuButton( const String& ButtonText, UIPopUpMenu * Menu ) {
	eeASSERT( NULL != Menu );

	UISelectButton::CreateParams ButtonParams;
	ButtonParams.setParent( this );
	ButtonParams.Flags				= UI_HALIGN_CENTER | UI_VALIGN_CENTER;

	if ( mFlags & UI_DRAW_SHADOW )
		ButtonParams.Flags |= UI_DRAW_SHADOW;

	ButtonParams.Font				= mFont;
	ButtonParams.FontColor			= mFontColor;
	ButtonParams.FontShadowColor	= mFontShadowColor;
	ButtonParams.FontOverColor		= mFontOverColor;

	UISelectButton * Button = eeNew( UISelectButton, ( ButtonParams ) );
	Button->setText( ButtonText );
	Button->setVisible( true );
	Button->setEnabled( true );
	Button->setThemeControl( mSkinState->getSkin()->getTheme(), "winmenubutton" );

	Menu->setVisible( false );
	Menu->setEnabled( false );
	Menu->setParent( getParent() );
	Menu->addEventListener( UIEvent::EventOnComplexControlFocusLoss, cb::Make1( this, &UIWinMenu::onMenuFocusLoss ) );

	mButtons.push_back( std::make_pair( Button, Menu ) );

	refreshButtons();
}

void UIWinMenu::setTheme( UITheme * Theme ) {
	UIComplexControl::setThemeControl( Theme, "winmenu" );

	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
		it->first->setThemeControl( Theme, "winmenubutton" );
	}

	if ( 0 == mMenuHeight && NULL != getSkin() && NULL != getSkin()->getSubTexture( UISkinState::StateNormal ) ) {
		mMenuHeight = getSkin()->getSubTexture( UISkinState::StateNormal )->getSize().getHeight();

		setSize( getParent()->getSize().getWidth(), mMenuHeight );

		updateAnchorsDistances();
	}
}

void UIWinMenu::removeMenuButton( const String& ButtonText ) {
	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
		if ( it->first->getText() == ButtonText ) {
			it->second->close();

			mButtons.erase( it );

			break;
		}
	}
}

UISelectButton * UIWinMenu::getButton( const String& ButtonText ) {
	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
		if ( it->first->getText() == ButtonText ) {
			return it->first;
		}
	}

	return NULL;
}

UIPopUpMenu * UIWinMenu::getPopUpMenu( const String& ButtonText ) {
	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
		if ( it->first->getText() == ButtonText ) {
			return it->second;
		}
	}

	return NULL;
}

void UIWinMenu::refreshButtons() {
	Uint32 xpos = mFirstButtonMargin;
	Int32 h = 0, th = 0, ycenter = 0;

	if ( NULL != getSkin() ) {
		SubTexture * subTexture = getSkin()->getSubTexture( UISkinState::StateNormal );

		if ( NULL != subTexture ) {
			h = subTexture->getSize().getHeight();

			if ( mButtons.begin() != mButtons.end() ) {
				UISelectButton * tbut = mButtons.begin()->first;

				if ( NULL != tbut->getSkin() ) {
					SubTexture * tSubTexture2 = tbut->getSkin()->getSubTexture( UISkinState::StateSelected );

					if ( NULL != tSubTexture2 )  {
						th = tSubTexture2->getSize().getHeight();

						switch ( VAlignGet( getFlags() ) ) {
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
		UITextBox * tbox		= pbut->getTextBox();

		pbut->setSize( tbox->getTextWidth() + mButtonMargin, getSize().getHeight() );
		pbut->setPosition( xpos, ycenter );

		xpos += pbut->getSize().getWidth() + mMarginBetweenButtons;
	}
}

Uint32 UIWinMenu::onMessage( const UIMessage * Msg ) {
	switch ( Msg->getMsg() ) {
		case UIMessage::MsgMouseUp:
		case UIMessage::MsgMouseEnter:
		{
			if ( Msg->getSender()->isType( UI_TYPE_SELECTBUTTON ) ) {
				UISelectButton * tbut	= reinterpret_cast<UISelectButton*> ( Msg->getSender() );
				UIPopUpMenu * tpop		= getMenuFromButton( tbut );

				Vector2i pos( tbut->getPosition().x, tbut->getPosition().y + tbut->getSize().getHeight() );
				tpop->setPosition( pos );

				if ( Msg->getMsg() == UIMessage::MsgMouseEnter ) {
					if ( NULL != mCurrentMenu ) {
						mCurrentMenu = tpop;

						tbut->select();
						tpop->show();
					}
				} else {
					if ( Msg->getFlags() & EE_BUTTON_LMASK ) {
						mCurrentMenu = tpop;

						tbut->select();
						tpop->show();
					}
				}

				return 1;
			}

			break;
		}
		case UIMessage::MsgSelected:
		{
			for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
				if ( it->first != Msg->getSender() ) {
					it->first->unselect();
				}
			}

			return 1;
		}
		case UIMessage::MsgFocusLoss:
		{
			UIControl * FocusCtrl = UIManager::instance()->focusControl();

			if ( !isParentOf( FocusCtrl ) && !isPopUpMenuChild( FocusCtrl ) ) {
				onComplexControlFocusLoss();
			}

			return 1;
		}
	}

	return 0;
}

void UIWinMenu::unselectButtons() {
	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
		it->first->unselect();
	}
}

UIPopUpMenu * UIWinMenu::getMenuFromButton( UISelectButton * Button ) {
	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
		if ( it->first == Button ) {
			return it->second;
		}
	}

	return NULL;
}

bool UIWinMenu::isPopUpMenuChild( UIControl * Ctrl ) {
	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
		if ( it->second == Ctrl || it->second->isParentOf( Ctrl ) ) {
			return true;
		}
	}

	return false;
}

void UIWinMenu::onMenuFocusLoss( const UIEvent * Event ) {
	UIControl * FocusCtrl = UIManager::instance()->focusControl();

	if ( !isParentOf( FocusCtrl ) && !isPopUpMenuChild( FocusCtrl ) ) {
		onComplexControlFocusLoss();
	}
}

void UIWinMenu::onComplexControlFocusLoss() {
	UIComplexControl::onComplexControlFocusLoss();

	if ( NULL != mCurrentMenu ) {
		mCurrentMenu->hide();

		mCurrentMenu = NULL;
	}

	unselectButtons();
}


void UIWinMenu::setFontColor( const ColorA& Color ) {
	mFontColor = Color;
}

const ColorA& UIWinMenu::getFontColor() const {
	return mFontColor;
}

void UIWinMenu::setFontOverColor( const ColorA& Color ) {
	mFontOverColor = Color;
}

const ColorA& UIWinMenu::getFontOverColor() const {
	return mFontOverColor;
}

void UIWinMenu::setFontSelectedColor( const ColorA& Color ) {
	mFontSelectedColor = Color;
}

const ColorA& UIWinMenu::getFontSelectedColor() const {
	return mFontSelectedColor;
}

Graphics::Font * UIWinMenu::getFont() const {
	return mFont;
}

void UIWinMenu::destroyMenues() {
	if ( !UIManager::instance()->isShootingDown() ) {
		for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); it++ ) {
			it->second->close();
		}
	}
}

}}
