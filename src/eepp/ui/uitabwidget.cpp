#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/graphics/renderer/gl.hpp>
#include <eepp/graphics/primitives.hpp>

namespace EE { namespace UI {

UITabWidget * UITabWidget::New() {
	return eeNew( UITabWidget, () );
}

UITabWidget::UITabWidget() :
	UIComplexControl(),
	mTabSelected( NULL ),
	mTabSelectedIndex( eeINDEX_NOT_FOUND )
{
	setHorizontalAlign( UI_HALIGN_CENTER );

	UITheme * Theme = UIThemeManager::instance()->getDefaultTheme();

	if ( NULL != Theme ) {
		mStyleConfig = Theme->getTabWidgetStyleConfig();
	}

	mTabContainer = UIComplexControl::New();
	mTabContainer->setParent( this )->setPosition( 0, 0 )->setSize( mSize.getWidth(), mStyleConfig.TabWidgetHeight )->setVisible( true )->setEnabled( true );
	mTabContainer->setFlags( UI_CLIP_ENABLE | UI_ANCHOR_RIGHT );

	mCtrlContainer = UIComplexControl::New();
	mCtrlContainer->setParent( this )->setPosition( 0, mStyleConfig.TabWidgetHeight )
			->setSize( mSize.getWidth(), mSize.getHeight() - mStyleConfig.TabWidgetHeight )->setVisible( true )->setEnabled( true )
			->setFlags( UI_CLIP_ENABLE | UI_ANCHOR_BOTTOM | UI_ANCHOR_RIGHT );

	onSizeChange();

	applyDefaultTheme();
}

UITabWidget::~UITabWidget() {
}

Uint32 UITabWidget::getType() const {
	return UI_TYPE_TABWIDGET;
}

bool UITabWidget::isType( const Uint32& type ) const {
	return UITabWidget::getType() == type ? true : UIComplexControl::isType( type );
}

void UITabWidget::setTheme( UITheme * Theme ) {
	mTabContainer->setThemeControl( Theme, "tabwidget" );

	mCtrlContainer->setThemeControl( Theme, "tabcontainer" );

	if ( 0 == mStyleConfig.TabWidgetHeight ) {
		UISkin * tSkin		= Theme->getByName( Theme->getAbbr() + "_" + "tab" );

		if ( NULL != tSkin ) {
			Sizei tSize1		= getSkinSize( tSkin );
			Sizei tSize2		= getSkinSize( tSkin, UISkinState::StateSelected );

			mStyleConfig.TabWidgetHeight	= eemax( tSize1.getHeight(), tSize2.getHeight() );

			seContainerSize();
			orderTabs();
		}
	}

	doAfterSetTheme();
}

void UITabWidget::doAfterSetTheme() {
	onSizeChange();
}

void UITabWidget::seContainerSize() {
	mTabContainer->setSize( mSize.getWidth(), mStyleConfig.TabWidgetHeight );
	mCtrlContainer->setPosition( 0, mStyleConfig.TabWidgetHeight );
	mCtrlContainer->setSize( mSize.getWidth(), mSize.getHeight() - mStyleConfig.TabWidgetHeight );
}

void UITabWidget::draw() {
	UIComplexControl::draw();

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

TooltipStyleConfig UITabWidget::getFontStyleConfig() const {
	return TooltipStyleConfig( mStyleConfig );
}

void UITabWidget::setFontStyleConfig(const TooltipStyleConfig & fontStyleConfig) {
	mStyleConfig.updateFontStyleConfig( fontStyleConfig );
}

TabWidgetStyleConfig UITabWidget::getStyleConfig() const {
	return mStyleConfig;
}

void UITabWidget::setStyleConfig(const TabWidgetStyleConfig & styleConfig) {
	Uint32		tabWidgetHeight = mStyleConfig.TabWidgetHeight;
	mStyleConfig = styleConfig;
	mStyleConfig.TabWidgetHeight = tabWidgetHeight;
	seContainerSize();
	setTabContainerSize();
	orderTabs();
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
	return mStyleConfig.FontColor;
}

void UITabWidget::setFontColor(const ColorA & fontColor) {
	mStyleConfig.FontColor = fontColor;

	if ( mTabs.size() > 0 ) {
		for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
			((UITab*)mTabs[ i ])->setFontColor( mStyleConfig.FontColor );
		}
	}
}

ColorA UITabWidget::getFontShadowColor() const {
	return mStyleConfig.FontShadowColor;
}

void UITabWidget::setFontShadowColor(const ColorA & fontShadowColor) {
	mStyleConfig.FontShadowColor = fontShadowColor;

	if ( mTabs.size() > 0 ) {
		for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
			((UITab*)mTabs[ i ])->setFontShadowColor( mStyleConfig.FontShadowColor );
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

UITab * UITabWidget::createTab( const String& Text, UIControl * CtrlOwned, SubTexture * Icon ) {
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

Uint32 UITabWidget::add( const String& Text, UIControl * CtrlOwned, SubTexture * Icon ) {
	return add( createTab( Text, CtrlOwned, Icon ) );
}

Uint32 UITabWidget::add( UITab * Tab ) {
	Tab->setParent( mTabContainer );

	mTabs.push_back( Tab );

	if ( NULL == mTabSelected ) {
		setTabSelected( Tab );
	} else {
		orderTabs();
	}

	return mTabs.size() - 1;
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

void UITabWidget::insert( const String& Text, UIControl * CtrlOwned, SubTexture * Icon, const Uint32& Index ) {
	insert( createTab( Text, CtrlOwned, Icon ), Index );
}

void UITabWidget::insert( UITab * Tab, const Uint32& Index ) {
	mTabs.insert( mTabs.begin() + Index, Tab );

	childAddAt( Tab, Index );

	orderTabs();
}

void UITabWidget::setTabSelected( UITab * Tab ) {
	if ( Tab == mTabSelected ) {
		return;
	}

	if ( NULL != mTabSelected ) {
		mTabSelected->unselect();
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

		mTabSelected->getControlOwned()->setVisible( true );
		mTabSelected->getControlOwned()->setSize( mCtrlContainer->getSize() );
		mTabSelected->getControlOwned()->setPosition( 0, 0 );

		orderTabs();

		sendCommonEvent( UIEvent::EventOnTabSelected );
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

	if ( NULL != mTabSelected ) {
		mTabSelected->getControlOwned()->setSize( mCtrlContainer->getSize() );
	}

	UIControl::onSizeChange();
}

void UITabWidget::applyThemeToTabs() {
	if ( mStyleConfig.SpecialBorderTabs ) {
		for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
			mTabs[ i ]->applyDefaultTheme();
		}
	}
}

UIComplexControl * UITabWidget::getTabContainer() const {
	return mTabContainer;
}

UIComplexControl * UITabWidget::getControlContainer() const {
	return mCtrlContainer;
}

}}
