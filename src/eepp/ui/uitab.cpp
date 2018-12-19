#include <eepp/ui/uitab.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UITab *UITab::New() {
	return eeNew( UITab, () );
}

UITab::UITab() :
	UISelectButton(),
	mControlOwned( NULL )
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
	if ( NULL != getParent() && NULL != getParent()->getParent() && getParent()->getParent()->isType( UI_TYPE_TABWIDGET ) ) {
		return reinterpret_cast<UITabWidget*> ( getParent()->getParent() );
	}

	return NULL;
}

void UITab::onParentChange() {
	applyDefaultTheme();
	UISelectButton::onParentChange();
}

void UITab::setTheme( UITheme * Theme ) {
	UIWidget::setTheme( Theme );

	std::string tabPos = "tab";

	UITabWidget * tTabW = getTabWidget();

	if ( NULL != tTabW ) {
		if ( tTabW->getSpecialBorderTabs() ) {
			if ( 0 == tTabW->getTabIndex( this ) ) {
				tabPos = "tab_left";
			} else if ( tTabW->getCount() > 0 && ( tTabW->getCount() - 1 ) == tTabW->getTabIndex( this ) ) {
				tabPos = "tab_right";
			}
		}
	}

	UINode::setThemeSkin( Theme, tabPos );

	onThemeLoaded();
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

	if ( NULL != tTabW && NULL != mSkinState ) {
		Int32 skinSize = getSkinSize( getSkin(), mSkinState->getCurrentState() ).getHeight();

		if ( 0 == skinSize ) {
			skinSize = getSkinSize().getHeight();
		}

		setSize( mDpSize.getWidth(), skinSize );

		if ( mSkinState->getState() & UISkinState::StateSelected ) {
			mTextBox->setFontColor( tTabW->getFontSelectedColor() );
		} else if ( mSkinState->getState() & UISkinState::StateHover ) {
			mTextBox->setFontColor( tTabW->getFontOverColor() );
		} else {
			mTextBox->setFontColor( tTabW->getFontColor() );
		}
	}
}

const String& UITab::getText() {
	return UIPushButton::getText();
}

UIPushButton * UITab::setText( const String &text ) {
	UITabWidget * tTabW = getTabWidget();

	if ( NULL != tTabW ) {
		if ( text.size() > tTabW->getMaxTextLength() ) {
			UIPushButton::setText( text.substr( 0, tTabW->getMaxTextLength() ) );

			onAutoSize();

			tTabW->orderTabs();

			tTabW->setTabSelected( tTabW->getSelectedTab() );

			return this;
		}

	}

	UIPushButton::setText( text );

	onAutoSize();

	tTabW->orderTabs();

	tTabW->setTabSelected( tTabW->getSelectedTab() );

	return this;
}

void UITab::onAutoSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		Uint32 w = PixelDensity::pxToDpI( mTextBox->getTextWidth() ) + mStyleConfig.IconHorizontalMargin + ( NULL != mIcon ? mIcon->getSize().getWidth() : 0 ) + getSkinSize().getWidth();

		UITabWidget * tTabW = getTabWidget();

		if ( NULL != tTabW ) {
			w = eemax( w, tTabW->getMinTabWidth() );
			w = eemin( w, tTabW->getMaxTabWidth() );
		}

		setSize( w, mDpSize.getHeight() );
	}
}

void UITab::update( const Time& time ) {
	UISelectButton::update( time );

	if ( mEnabled && mVisible ) {
		if ( NULL == mControlOwned && !mOwnedName.empty() ) {
			setOwnedControl();
		}

		if ( isMouseOver() ) {
			UITabWidget * tTabW	= getTabWidget();

			if ( NULL != tTabW && NULL != getEventDispatcher() ) {
				Uint32 Flags 			= getEventDispatcher()->getClickTrigger();

				if ( Flags & EE_BUTTONS_WUWD ) {
					if ( Flags & EE_BUTTON_WUMASK ) {
						tTabW->selectPrev();
					} else if ( Flags & EE_BUTTON_WDMASK ) {
						tTabW->selectNext();
					}
				} else if ( tTabW->getTabsClosable() && ( Flags & EE_BUTTON_MMASK ) ) {
					tTabW->remove( this );
				}
			}
		}
	}
}

bool UITab::setAttribute( const NodeAttribute& attribute ) {
	std::string name = attribute.getName();

	if ( "name" == name || "text" == name ) {
		if ( NULL != mSceneNode && mSceneNode->isUISceneNode() )
			setText( static_cast<UISceneNode*>( mSceneNode )->getTranslatorString( attribute.asString() ) );
	} else if ( "controlowned" == name || "owns" == name ) {
		mOwnedName = attribute.asString();
		setOwnedControl();
	} else {
		return UISelectButton::setAttribute( attribute );
	}

	return true;
}

void UITab::setOwnedControl() {
	Node * ctrl = getParent()->getParent()->find( mOwnedName );

	if ( NULL != ctrl ) {
		setControlOwned( ctrl );
	}
}

Node * UITab::getControlOwned() const {
	return mControlOwned;
}

void UITab::setControlOwned( Node * controlOwned ) {
	mControlOwned = controlOwned;

	UITabWidget * tTabW = getTabWidget();

	if ( NULL != tTabW ) {
		tTabW->refreshControlOwned( this );

		if ( NULL == tTabW->mTabSelected )
			tTabW->setTabSelected( this );
	}
}

}}
