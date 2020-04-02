#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitab.hpp>
#include <eepp/ui/uitabwidget.hpp>

namespace EE { namespace UI {

UITab* UITab::New() {
	return eeNew( UITab, () );
}

UITab::UITab() : UISelectButton( "tab" ), mControlOwned( NULL ) {
	mTextBox->setElementTag( "tab::text" );
	mIcon->setElementTag( "tab::icon" );

	applyDefaultTheme();
}

UITab::~UITab() {}

Uint32 UITab::getType() const {
	return UI_TYPE_TAB;
}

bool UITab::isType( const Uint32& type ) const {
	return UITab::getType() == type ? true : UISelectButton::isType( type );
}

UITabWidget* UITab::getTabWidget() {
	if ( NULL != getParent() && NULL != getParent()->getParent() &&
		 getParent()->getParent()->isType( UI_TYPE_TABWIDGET ) ) {
		return getParent()->getParent()->asType<UITabWidget>();
	}

	return NULL;
}

void UITab::onParentChange() {
	applyDefaultTheme();
	UISelectButton::onParentChange();
}

void UITab::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	std::string tabPos = "tab";

	UITabWidget* tTabW = getTabWidget();

	if ( NULL != tTabW ) {
		if ( tTabW->getSpecialBorderTabs() ) {
			if ( 0 == tTabW->getTabIndex( this ) ) {
				tabPos = "tab_left";
			} else if ( tTabW->getTabCount() > 0 &&
						( tTabW->getTabCount() - 1 ) == tTabW->getTabIndex( this ) ) {
				tabPos = "tab_right";
			}
		}
	}

	UINode::setThemeSkin( Theme, tabPos );

	onThemeLoaded();
	onStateChange();
}

Uint32 UITab::onMouseClick( const Vector2i& Pos, const Uint32& Flags ) {
	UISelectButton::onMouseClick( Pos, Flags );

	UITabWidget* tTabW = getTabWidget();

	if ( NULL != tTabW ) {
		if ( Flags & EE_BUTTON_LMASK ) {
			tTabW->setTabSelected( this );
		}
	}

	return UISelectButton::onMouseClick( Pos, Flags );
}

void UITab::onStateChange() {
	UISelectButton::onStateChange();

	UITabWidget* tTabW = getTabWidget();

	if ( NULL != tTabW && NULL != mSkinState ) {
		Int32 skinSize = getSkinSize( getSkin(), mSkinState->getCurrentState() ).getHeight();

		if ( 0 == skinSize ) {
			skinSize = getSkinSize().getHeight();
		}

		setSize( getSize().getWidth(), skinSize );
	}
}

const String& UITab::getText() {
	return UIPushButton::getText();
}

UIPushButton* UITab::setText( const String& text ) {
	mText = text;

	UITabWidget* tTabW = getTabWidget();

	if ( NULL != tTabW ) {
		updateTab();

		tTabW->orderTabs();

		tTabW->setTabSelected( tTabW->getSelectedTab() );
	}

	return this;
}

void UITab::onAutoSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		Uint32 w = PixelDensity::pxToDpI( mTextBox->getTextWidth() ) +
				   mStyleConfig.IconHorizontalMargin +
				   ( NULL != mIcon ? mIcon->getSize().getWidth() : 0 ) + getSkinSize().getWidth();

		UITabWidget* tTabW = getTabWidget();

		if ( NULL != tTabW ) {
			w = eemax( w, tTabW->getMinTabWidth() );
			w = eemin( w, tTabW->getMaxTabWidth() );
		}

		setInternalWidth( w );
	}
}

std::string UITab::getPropertyString( const PropertyDefinition* propertyDef,
									  const Uint32& propertyIndex ) {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Text:
		case PropertyId::Name:
			return getText().toUtf8();
		case PropertyId::Owns:
			return mOwnedName;
		default:
			return UISelectButton::getPropertyString( propertyDef, propertyIndex );
	}
}

bool UITab::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Text:
		case PropertyId::Name:
			if ( NULL != mSceneNode && mSceneNode->isUISceneNode() )
				setText( static_cast<UISceneNode*>( mSceneNode )
							 ->getTranslatorString( attribute.asString() ) );
			break;
		case PropertyId::Owns:
			mOwnedName = attribute.asString();
			setOwnedControl();
			break;
		default:
			return UISelectButton::applyProperty( attribute );
	}

	return true;
}

Uint32 UITab::onMouseUp( const Vector2i& Pos, const Uint32& Flags ) {
	if ( mEnabled && mVisible ) {
		if ( NULL == mControlOwned && !mOwnedName.empty() ) {
			setOwnedControl();
		}

		UITabWidget* tTabW = getTabWidget();

		if ( NULL != tTabW ) {
			if ( Flags & EE_BUTTONS_WUWD ) {
				if ( Flags & EE_BUTTON_WUMASK ) {
					tTabW->selectPreviousTab();
				} else if ( Flags & EE_BUTTON_WDMASK ) {
					tTabW->selectNextTab();
				}
			} else if ( tTabW->getTabsClosable() && ( Flags & EE_BUTTON_MMASK ) ) {
				tTabW->removeTab( this );
			}
		}
	}

	return UISelectButton::onMouseUp( Pos, Flags );
}

void UITab::setOwnedControl() {
	Node* ctrl = getParent()->getParent()->find( mOwnedName );

	if ( NULL != ctrl ) {
		setControlOwned( ctrl );
	}
}

void UITab::updateTab() {
	UITabWidget* tTabW = getTabWidget();

	if ( NULL != tTabW ) {
		if ( mText.size() > tTabW->getMaxTextLength() ) {
			UIPushButton::setText( mText.substr( 0, tTabW->getMaxTextLength() ) );
		} else {
			UIPushButton::setText( mText );
		}

		onAutoSize();
	}
}

Node* UITab::getControlOwned() const {
	return mControlOwned;
}

void UITab::setControlOwned( Node* controlOwned ) {
	mControlOwned = controlOwned;

	UITabWidget* tTabW = getTabWidget();

	if ( NULL != tTabW ) {
		tTabW->refreshControlOwned( this );

		if ( NULL == tTabW->mTabSelected )
			tTabW->setTabSelected( this );
	}
}

}} // namespace EE::UI
