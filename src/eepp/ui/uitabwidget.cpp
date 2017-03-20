#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/helper/pugixml/pugixml.hpp>

namespace EE { namespace UI {

UITabWidget * UITabWidget::New() {
	return eeNew( UITabWidget, () );
}

UITabWidget::UITabWidget() :
	UIWidget(),
	mTabSelected( NULL ),
	mTabSelectedIndex( eeINDEX_NOT_FOUND )
{
	setHorizontalAlign( UI_HALIGN_CENTER );

	UITheme * Theme = UIThemeManager::instance()->getDefaultTheme();

	if ( NULL != Theme ) {
		mStyleConfig = Theme->getTabWidgetStyleConfig();
	}

	mTabContainer = UIWidget::New();
	mTabContainer->setParent( this )->setPosition( 0, 0 )->setSize( mSize.getWidth(), mStyleConfig.TabWidgetHeight );
	mTabContainer->setFlags( UI_CLIP_ENABLE );

	mCtrlContainer = UIWidget::New();
	mCtrlContainer->setParent( this )->setPosition( 0, mStyleConfig.TabWidgetHeight )
			->setSize( mSize.getWidth(), mSize.getHeight() - mStyleConfig.TabWidgetHeight )
			->setFlags( UI_CLIP_ENABLE );

	onSizeChange();

	applyDefaultTheme();
}

UITabWidget::~UITabWidget() {
}

Uint32 UITabWidget::getType() const {
	return UI_TYPE_TABWIDGET;
}

bool UITabWidget::isType( const Uint32& type ) const {
	return UITabWidget::getType() == type ? true : UIWidget::isType( type );
}

void UITabWidget::setTheme( UITheme * Theme ) {
	UIWidget::setTheme( Theme );

	mTabContainer->setThemeControl( Theme, "tabwidget" );

	mCtrlContainer->setThemeControl( Theme, "tabcontainer" );

	if ( 0 == mStyleConfig.TabWidgetHeight ) {
		UISkin * tSkin		= Theme->getSkin( "tab" );

		if ( NULL != tSkin ) {
			Sizei tSize1		= getSkinSize( tSkin );
			Sizei tSize2		= getSkinSize( tSkin, UISkinState::StateSelected );

			mStyleConfig.TabWidgetHeight	= eemax( tSize1.getHeight(), tSize2.getHeight() );

			seContainerSize();
			orderTabs();
		}
	}

	onThemeLoaded();
}

void UITabWidget::onThemeLoaded() {
	onSizeChange();
}

void UITabWidget::seContainerSize() {
	mTabContainer->setSize( mSize.getWidth(), mStyleConfig.TabWidgetHeight );
	mCtrlContainer->setPosition( 0, mStyleConfig.TabWidgetHeight );
	mCtrlContainer->setSize( mSize.getWidth(), mSize.getHeight() - mStyleConfig.TabWidgetHeight );
}

void UITabWidget::draw() {
	UIWidget::draw();

	if ( mStyleConfig.DrawLineBelowTabs ) {
		bool smooth = GLi->isLineSmooth();
		if ( smooth ) GLi->lineSmooth( false );

		Primitives P;
		Vector2i p1( mScreenPos.x, mScreenPos.y + mTabContainer->getRealSize().getHeight() + mStyleConfig.LineBelowTabsYOffset );
		Vector2i p2( mScreenPos.x + mTabContainer->getRealPosition().x, p1.y );

		P.setLineWidth( PixelDensity::dpToPx( 1 ) );
		P.setColor(mStyleConfig.LineBelowTabsColor );
		P.drawLine( Line2f( Vector2f( p1.x, p1.y ), Vector2f( p2.x, p2.y ) ) );

		Vector2i p3( mScreenPos.x + mTabContainer->getRealPosition().x + mTabContainer->getRealSize().getWidth(), mScreenPos.y + mTabContainer->getRealSize().getHeight() + mStyleConfig.LineBelowTabsYOffset );
		Vector2i p4( mScreenPos.x + mRealSize.getWidth(), p3.y );

		P.drawLine( Line2f( Vector2f( p3.x, p3.y ), Vector2f( p4.x, p4.y ) ) );

		if ( smooth ) GLi->lineSmooth( true );
	}
}

UITooltipStyleConfig UITabWidget::getFontStyleConfig() const {
	return UITooltipStyleConfig( mStyleConfig );
}

void UITabWidget::setFontStyleConfig(const UITooltipStyleConfig & fontStyleConfig) {
	mStyleConfig.updateFontStyleConfig( fontStyleConfig );
}

UITabWidgetStyleConfig UITabWidget::getStyleConfig() const {
	return mStyleConfig;
}

const Uint32 &UITabWidget::getFontStyle() const {
	return mStyleConfig.Style;
}

const Float &UITabWidget::getOutlineThickness() const {
	return mStyleConfig.OutlineThickness;
}

UITabWidget * UITabWidget::setOutlineThickness( const Float & outlineThickness ) {
	if ( mStyleConfig.OutlineThickness != outlineThickness ) {
		mStyleConfig.OutlineThickness = outlineThickness;

		if ( mTabs.size() > 0 ) {
			for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
				((UITab*)mTabs[ i ])->setOutlineThickness( outlineThickness );
			}
		}
	}

	return this;
}

const ColorA &UITabWidget::getOutlineColor() const {
	return mStyleConfig.OutlineColor;
}

UITabWidget * UITabWidget::setOutlineColor(const ColorA & outlineColor) {
	if ( mStyleConfig.OutlineColor != outlineColor ) {
		mStyleConfig.OutlineColor = outlineColor;

		if ( mTabs.size() > 0 ) {
			for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
				((UITab*)mTabs[ i ])->setOutlineColor( outlineColor );
			}
		}
	}

	return this;
}

UITabWidget * UITabWidget::setFontStyle(const Uint32 & fontStyle) {
	if ( mStyleConfig.Style != fontStyle ) {
		mStyleConfig.Style = fontStyle;

		if ( mTabs.size() > 0 ) {
			for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
				((UITab*)mTabs[ i ])->setFontStyle( fontStyle );
			}
		}
	}

	return this;
}

void UITabWidget::setStyleConfig(const UITabWidgetStyleConfig & styleConfig) {
	Uint32		tabWidgetHeight = mStyleConfig.TabWidgetHeight;
	mStyleConfig = styleConfig;
	mStyleConfig.TabWidgetHeight = tabWidgetHeight;
	seContainerSize();
	setTabContainerSize();
	orderTabs();
}

void UITabWidget::loadFromXmlNode(const pugi::xml_node & node) {
	UIWidget::loadFromXmlNode( node );

	for (pugi::xml_attribute_iterator ait = node.attributes_begin(); ait != node.attributes_end(); ++ait) {
		std::string name = ait->name();
		String::toLowerInPlace( name );

		if ( "textcolor" == name ) {
			setFontColor( ColorA::fromString( ait->as_string() ) );
		} else if ( "textshadowcolor" == name ) {
			setFontShadowColor( ColorA::fromString( ait->as_string() ) );
		} else if ( "textovercolor" == name ) {
			setFontOverColor( ColorA::fromString( ait->as_string() ) );
		} else if ( "textselectedcolor" == name ) {
			setFontSelectedColor( ColorA::fromString( ait->as_string() ) );
		} else if ( "fontfamily" == name || "fontname" == name ) {
			Font * font = FontManager::instance()->getByName( ait->as_string() );

			if ( NULL != font )
				setFont( font );
		} else if ( "textsize" == name || "fontsize" == name || "charactersize" == name ) {
			setCharacterSize( PixelDensity::toDpFromStringI( ait->as_string() ) );
		} else if ( "textstyle" == name || "fontstyle" == name ) {
			std::string valStr = ait->as_string();
			String::toLowerInPlace( valStr );
			std::vector<std::string> strings = String::split( valStr, '|' );
			Uint32 flags = Text::Regular;

			if ( strings.size() ) {
				for ( std::size_t i = 0; i < strings.size(); i++ ) {
					std::string cur = strings[i];
					String::toLowerInPlace( cur );

					if ( "underlined" == cur || "underline" == cur )
						flags |= Text::Underlined;
					else if ( "bold" == cur )
						flags |= Text::Bold;
					else if ( "italic" == cur )
						flags |= Text::Italic;
					else if ( "strikethrough" == cur )
						flags |= Text::StrikeThrough;
					else if ( "shadowed" == cur || "shadow" == cur )
						flags |= Text::Shadow;
				}

				setFontStyle( flags );
			}
		} else if ( "fontoutlinethickness" == name ) {
			setOutlineThickness( PixelDensity::toDpFromString( ait->as_string() ) );
		} else if ( "fontoutlinecolor" == name ) {
			setOutlineColor( ColorA::fromString( ait->as_string() ) );
		} else if ( "maxtextlength" == name ) {
			setMaxTextLength( ait->as_uint(1) );
		} else if ( "mintabwidth" == name ) {
			setMinTabWidth( ait->as_uint(1) );
		} else if ( "maxtabwidth" == name ) {
			setMaxTabWidth( ait->as_uint() );
		} else if ( "tabclosable" == name ) {
			setTabsClosable( ait->as_bool() );
		} else if ( "specialbordertabs" == name ) {
			setSpecialBorderTabs( ait->as_bool() );
		} else if ( "drawlinebelowtabs" == name ) {
			setDrawLineBelowTabs( ait->as_bool() );
		} else if ( "linebelowtabscolor" == name ) {
			setLineBelowTabsColor( ColorA::fromString( ait->as_string() ) );
		} else if ( "linebelowtabsyoffset" == name ) {
			setLineBelowTabsYOffset( ait->as_int() );
		}
	}
}

Font * UITabWidget::getFont() const {
	return mStyleConfig.Font;
}

void UITabWidget::setFont(Font * font) {
	mStyleConfig.Font = font;

	if ( mTabs.size() > 0 ) {
		for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
			((UITab*)mTabs[ i ])->setFont( mStyleConfig.Font );
		}
	}
}

ColorA UITabWidget::getFontColor() const {
	return mStyleConfig.Color;
}

void UITabWidget::setFontColor(const ColorA & fontColor) {
	mStyleConfig.Color = fontColor;

	if ( mTabs.size() > 0 ) {
		for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
			((UITab*)mTabs[ i ])->setFontColor( mStyleConfig.Color );
		}
	}
}

ColorA UITabWidget::getFontShadowColor() const {
	return mStyleConfig.ShadowColor;
}

void UITabWidget::setFontShadowColor(const ColorA & fontShadowColor) {
	mStyleConfig.ShadowColor = fontShadowColor;

	if ( mTabs.size() > 0 ) {
		for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
			((UITab*)mTabs[ i ])->setFontShadowColor( mStyleConfig.ShadowColor );
		}
	}
}

ColorA UITabWidget::getFontOverColor() const {
	return mStyleConfig.FontOverColor;
}

void UITabWidget::setFontOverColor(const ColorA & fontOverColor) {
	mStyleConfig.FontOverColor = fontOverColor;

	if ( mTabs.size() > 0 ) {
		for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
			((UITab*)mTabs[ i ])->setFontOverColor( mStyleConfig.FontOverColor );
		}
	}
}

ColorA UITabWidget::getFontSelectedColor() const {
	return mStyleConfig.FontSelectedColor;
}

void UITabWidget::setFontSelectedColor(const ColorA & fontSelectedColor) {
	mStyleConfig.FontSelectedColor = fontSelectedColor;

	if ( mTabs.size() > 0 ) {
		for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
			((UITab*)mTabs[ i ])->setFontSelectedColor( mStyleConfig.FontSelectedColor );
		}
	}
}

Uint32 UITabWidget::getCharacterSize() {
	return mStyleConfig.CharacterSize;
}

void UITabWidget::setCharacterSize( const Uint32& characterSize ) {
	mStyleConfig.CharacterSize = characterSize;

	if ( mTabs.size() > 0 ) {
		for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
			((UITab*)mTabs[ i ])->setCharacterSize( mStyleConfig.CharacterSize );
		}
	}
}

Int32 UITabWidget::getTabSeparation() const {
	return mStyleConfig.TabSeparation;
}

void UITabWidget::setTabSeparation(const Int32 & tabSeparation) {
	mStyleConfig.TabSeparation = tabSeparation;
	setTabContainerSize();
	posTabs();
}

Uint32 UITabWidget::getMaxTextLength() const {
	return mStyleConfig.MaxTextLength;
}

void UITabWidget::setMaxTextLength(const Uint32 & maxTextLength) {
	mStyleConfig.MaxTextLength = maxTextLength;
}

Uint32 UITabWidget::getTabWidgetHeight() const {
	return mStyleConfig.TabWidgetHeight;
}

Uint32 UITabWidget::getMinTabWidth() const {
	return mStyleConfig.MinTabWidth;
}

void UITabWidget::setMinTabWidth(const Uint32 & minTabWidth) {
	mStyleConfig.MinTabWidth = minTabWidth;
}

Uint32 UITabWidget::getMaxTabWidth() const {
	return mStyleConfig.MaxTabWidth;
}

void UITabWidget::setMaxTabWidth(const Uint32 & maxTabWidth) {
	mStyleConfig.MaxTabWidth = maxTabWidth;
}

bool UITabWidget::getTabsClosable() const {
	return mStyleConfig.TabsClosable;
}

void UITabWidget::setTabsClosable(bool tabsClosable) {
	mStyleConfig.TabsClosable = tabsClosable;
}

bool UITabWidget::getSpecialBorderTabs() const {
	return mStyleConfig.SpecialBorderTabs;
}

void UITabWidget::setSpecialBorderTabs(bool specialBorderTabs) {
	mStyleConfig.SpecialBorderTabs = specialBorderTabs;
	applyThemeToTabs();
}

bool UITabWidget::getDrawLineBelowTabs() const {
	return mStyleConfig.DrawLineBelowTabs;
}

void UITabWidget::setDrawLineBelowTabs(bool drawLineBelowTabs) {
	mStyleConfig.DrawLineBelowTabs = drawLineBelowTabs;
}

ColorA UITabWidget::getLineBelowTabsColor() const {
	return mStyleConfig.LineBelowTabsColor;
}

void UITabWidget::setLineBelowTabsColor(const ColorA & lineBelowTabsColor) {
	mStyleConfig.LineBelowTabsColor = lineBelowTabsColor;
}

Int32 UITabWidget::getLineBelowTabsYOffset() const {
	return mStyleConfig.LineBelowTabsYOffset;
}

void UITabWidget::setLineBelowTabsYOffset(const Int32 & lineBelowTabsYOffset) {
	mStyleConfig.LineBelowTabsYOffset = lineBelowTabsYOffset;
}

void UITabWidget::setTabContainerSize() {
	Uint32 s = 0;

	if ( mTabs.size() > 0 ) {
		for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
			s += mTabs[i]->getSize().getWidth() + mStyleConfig.TabSeparation;
		}

		s -= mStyleConfig.TabSeparation;
	}

	mTabContainer->setSize( s, mStyleConfig.TabWidgetHeight );

	switch ( HAlignGet( mFlags ) )
	{
		case UI_HALIGN_LEFT:
			mTabContainer->setPosition( 0, 0 );
			break;
		case UI_HALIGN_CENTER:
			mTabContainer->centerHorizontal();
			break;
		case UI_HALIGN_RIGHT:
			mTabContainer->setPosition( mSize.getWidth() - mTabContainer->getSize().getWidth(), 0 );
			break;
	}
}

void UITabWidget::posTabs() {
	Uint32 w	= 0;
	Uint32 h	= 0;
	Uint32 VA	= VAlignGet( mFlags );

	for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
		switch ( VA )
		{
			case UI_VALIGN_BOTTOM:
				h = mStyleConfig.TabWidgetHeight - mTabs[i]->getSize().getHeight();
				break;
			case UI_VALIGN_TOP:
				h = 0;
				break;
			case UI_VALIGN_CENTER:
				h = mStyleConfig.TabWidgetHeight / 2 - mTabs[i]->getSize().getHeight() / 2;
				break;
		}

		mTabs[i]->setPosition( w, h );

		w += mTabs[i]->getSize().getWidth() + mStyleConfig.TabSeparation;
	}
}

void UITabWidget::zorderTabs() {
	for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
		mTabs[i]->toBack();
	}

	if ( NULL != mTabSelected ) {
		mTabSelected->toFront();
	}
}

void UITabWidget::orderTabs() {
	applyThemeToTabs();

	zorderTabs();

	setTabContainerSize();

	posTabs();
}

UITab * UITabWidget::createTab( const String& Text, UIControl * CtrlOwned, Drawable * Icon ) {
	UITab * tCtrl 	= UITab::New();
	tCtrl->setParent( mTabContainer );
	tCtrl->setFlags( UI_VALIGN_CENTER | UI_HALIGN_CENTER | UI_AUTO_SIZE );
	tCtrl->setStyleConfig( mStyleConfig );
	tCtrl->setIcon( Icon );
	tCtrl->setText( Text );
	tCtrl->setVisible( true );
	tCtrl->setEnabled( true );
	tCtrl->setControlOwned( CtrlOwned );
	CtrlOwned->setParent( mCtrlContainer );
	CtrlOwned->setVisible( false );
	CtrlOwned->setEnabled( true );

	return tCtrl;
}

UITabWidget * UITabWidget::add( const String& Text, UIControl * CtrlOwned, Drawable * Icon ) {
	return add( createTab( Text, CtrlOwned, Icon ) );
}

UITabWidget * UITabWidget::add( UITab * Tab ) {
	Tab->setParent( mTabContainer );

	mTabs.push_back( Tab );

	if ( NULL == mTabSelected ) {
		setTabSelected( Tab );
	} else {
		orderTabs();
	}

	return this;
}

UITab * UITabWidget::getTab( const Uint32& Index ) {
	eeASSERT( Index < mTabs.size() );
	return mTabs[ Index ];
}

UITab * UITabWidget::getTab( const String& Text ) {
	for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
		if ( mTabs[i]->isType( UI_TYPE_TAB ) ) {
			UITab * tTab = reinterpret_cast<UITab*>( mTabs[i] );

			if ( tTab->getText() == Text )
				return tTab;
		}
	}

	return NULL;
}

Uint32 UITabWidget::getTabIndex( UITab * Tab ) {
	for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
		if ( mTabs[i] == Tab )
			return i;
	}

	return eeINDEX_NOT_FOUND;
}

Uint32 UITabWidget::getCount() const {
	return mTabs.size();
}

void UITabWidget::remove( const Uint32& Index ) {
	eeASSERT( Index < mTabs.size() );

	if ( mTabs[ Index ] == mTabSelected ) {
		mTabSelected->getControlOwned()->setVisible( false );
	}

	eeSAFE_DELETE( mTabs[ Index ] );

	mTabs.erase( mTabs.begin() + Index );

	mTabSelected = NULL;

	if ( Index == mTabSelectedIndex ) {
		if ( mTabs.size() > 0 ) {
			if ( mTabSelectedIndex < mTabs.size() ) {
				setTabSelected( mTabs[ mTabSelectedIndex ] );
			} else {
				if ( mTabSelectedIndex > 0 && mTabSelectedIndex - 1 < mTabs.size() ) {
					setTabSelected( mTabs[ mTabSelectedIndex - 1 ] );
				} else {
					setTabSelected( mTabs[ 0 ] );
				}
			}
		} else {
			mTabSelected		= NULL;
			mTabSelectedIndex	= eeINDEX_NOT_FOUND;
		}
	}

	orderTabs();
}

void UITabWidget::remove( UITab * Tab ) {
	remove( getTabIndex( Tab ) );
}

void UITabWidget::removeAll() {
	for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
		eeSAFE_DELETE( mTabs[ i ] );
	}

	mTabs.clear();

	mTabSelected		= NULL;
	mTabSelectedIndex	= eeINDEX_NOT_FOUND;

	orderTabs();
}

void UITabWidget::insert( const String& Text, UIControl * CtrlOwned, Drawable * Icon, const Uint32& Index ) {
	insert( createTab( Text, CtrlOwned, Icon ), Index );
}

void UITabWidget::insert( UITab * Tab, const Uint32& Index ) {
	mTabs.insert( mTabs.begin() + Index, Tab );

	childAddAt( Tab, Index );

	orderTabs();
}

void UITabWidget::setTabSelected( UITab * Tab ) {
	if ( NULL == Tab )
		return;

	if ( Tab == mTabSelected ) {
		refreshControlOwned( Tab );
		return;
	}

	if ( NULL != mTabSelected ) {
		mTabSelected->unselect();

		if ( NULL != mTabSelected->getControlOwned() )
			mTabSelected->getControlOwned()->setVisible( false );
	}

	if ( NULL != Tab ) {
		Tab->select();
	} else {
		return;
	}

	Uint32 TabIndex		= getTabIndex( Tab );

	if ( eeINDEX_NOT_FOUND != TabIndex ) {
		mTabSelected		= Tab;
		mTabSelectedIndex	= TabIndex;

		refreshControlOwned( mTabSelected );

		orderTabs();

		sendCommonEvent( UIEvent::EventOnTabSelected );
	}
}

void UITabWidget::refreshControlOwned( UITab * tab ) {
	if ( NULL != tab && NULL != tab->getControlOwned() ) {
		tab->getControlOwned()->setParent( mCtrlContainer );
		tab->getControlOwned()->setVisible( tab == mTabSelected );
		tab->getControlOwned()->setSize( mCtrlContainer->getSize() );
		tab->getControlOwned()->setPosition( 0, 0 );
	}
}

void UITabWidget::selectPrev() {
	if ( eeINDEX_NOT_FOUND != mTabSelectedIndex && mTabSelectedIndex > 0 ) {
		setTabSelected( getTab( mTabSelectedIndex - 1 ) );
	}
}

void UITabWidget::selectNext() {
	if ( mTabSelectedIndex + 1 < mTabs.size() ) {
		setTabSelected( getTab( mTabSelectedIndex + 1 ) );
	}
}

UITab * UITabWidget::getSelectedTab() const {
	return mTabSelected;
}

Uint32 UITabWidget::getSelectedTabIndex() const {
	return mTabSelectedIndex;
}

void UITabWidget::onSizeChange() {
	seContainerSize();
	setTabContainerSize();
	posTabs();

	if ( NULL != mTabSelected && NULL != mTabSelected->getControlOwned() ) {
		mTabSelected->getControlOwned()->setSize( mCtrlContainer->getSize() );
	}

	UIControl::onSizeChange();
}

void UITabWidget::onChildCountChange() {
	UIControl * child = mChild;
	bool found = false;

	while ( NULL != child ) {
		if ( !( child == mTabContainer || child == mCtrlContainer ) ) {
			found = true;
			break;
		}

		child = child->getNextControl();
	}

	if ( found ) {
		if ( child->isType( UI_TYPE_TAB ) ) {
			UITab * Tab = static_cast<UITab*>( child );

			Tab->setParent( mTabContainer );

			mTabs.push_back( Tab );

			if ( NULL == mTabSelected ) {
				setTabSelected( Tab );
			} else {
				orderTabs();
			}
		} else {
			child->setParent( mCtrlContainer );
			child->setVisible( false );
			child->setEnabled( true );
		}
	}
}

void UITabWidget::applyThemeToTabs() {
	if ( mStyleConfig.SpecialBorderTabs ) {
		for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
			mTabs[ i ]->applyDefaultTheme();
		}
	}
}

UIWidget * UITabWidget::getTabContainer() const {
	return mTabContainer;
}

UIWidget * UITabWidget::getControlContainer() const {
	return mCtrlContainer;
}

}}
