#include <eepp/ui/uitab.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UITab::UITab( UISelectButton::CreateParams& Params, UIControl * CtrlOwned ) :
	UISelectButton( Params ),
	mCtrlOwned( CtrlOwned )
{
	applyDefaultTheme();
}

UITab::~UITab() {
}

Uint32 UITab::getType() const {
	return UI_TYPE_TAB;
}

bool UITab::isType( const Uint32& type ) const {
	return UITab::getType() == type ? true : UISelectButton::isType( type );
}

UITabWidget * UITab::getTabWidget() {
	if ( parent()->parent()->isType( UI_TYPE_TABWIDGET ) ) {
		return reinterpret_cast<UITabWidget*> ( parent()->parent() );
	}

	return NULL;
}

void UITab::setTheme( UITheme * Theme ) {
	std::string tabPos = "tab";

	UITabWidget * tTabW = getTabWidget();

	if ( NULL != tTabW ) {
		if ( tTabW->mSpecialBorderTabs ) {
			if ( 0 == tTabW->getTabIndex( this ) ) {
				tabPos = "tab_left";
			} else if ( tTabW->count() > 0 && ( tTabW->count() - 1 ) == tTabW->getTabIndex( this ) ) {
				tabPos = "tab_right";
			}
		}
	}

	UIControl::setThemeControl( Theme, tabPos );

	doAftersetTheme();
}

Uint32 UITab::onMouseClick( const Vector2i &Pos, const Uint32 Flags ) {
	UISelectButton::onMouseClick( Pos, Flags );

	UITabWidget * tTabW = getTabWidget();

	if ( NULL != tTabW ) {
		if ( Flags & EE_BUTTON_LMASK ) {
			tTabW->setTabSelected( this );
		}
	}

	return 1;
}

void UITab::onStateChange() {
	UISelectButton::onStateChange();

	UITabWidget * tTabW = getTabWidget();

	if ( NULL != tTabW ) {
		size( mSize.getWidth(), getSkinSize( getSkin(), mSkinState->getState() ).getHeight() );

		if ( mSkinState->getState() == UISkinState::StateSelected ) {
			mTextBox->color( tTabW->mFontSelectedColor );
		} else if ( mSkinState->getState() == UISkinState::StateMouseEnter ) {
			mTextBox->color( tTabW->mFontOverColor );
		} else {
			mTextBox->color( tTabW->mFontColor );
		}
	}
}

const String& UITab::text() {
	return UIPushButton::text();
}

void UITab::text( const String &text ) {
	UITabWidget * tTabW = getTabWidget();

	if ( NULL != tTabW ) {
		if ( text.size() > tTabW->mMaxTextLength ) {
			UIPushButton::text( text.substr( 0, tTabW->mMaxTextLength ) );

			setRealSize();

			return;
		}
	}

	UIPushButton::text( text );

	setRealSize();
}

void UITab::setRealSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		Uint32 w = mTextBox->getTextWidth() + getSkinSize().getWidth();

		UITabWidget * tTabW = getTabWidget();

		if ( NULL != tTabW ) {
			w = eemax( w, tTabW->mMinTabWidth );
			w = eemin( w, tTabW->mMaxTabWidth );
		}

		size( w, mSize.getHeight() );
	}
}

UIControl * UITab::ctrlOwned() const {
	return mCtrlOwned;
}

void UITab::update() {
	UISelectButton::update();

	if ( mEnabled && mVisible ) {
		if ( isMouseOver() ) {
			UITabWidget * tTabW	= getTabWidget();

			if ( NULL != tTabW ) {
				Uint32 Flags 			= UIManager::instance()->getInput()->getClickTrigger();

				if ( Flags & EE_BUTTONS_WUWD ) {
					if ( Flags & EE_BUTTON_WUMASK ) {
						tTabW->selectPrev();
					} else if ( Flags & EE_BUTTON_WDMASK ) {
						tTabW->selectNext();
					}
				} else if ( tTabW->mTabsClosable && ( Flags & EE_BUTTON_MMASK ) ) {
					tTabW->remove( this );
				}
			}
		}
	}
}

}}
