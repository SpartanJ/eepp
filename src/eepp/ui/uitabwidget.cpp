#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/css/propertydefinition.hpp>

namespace EE { namespace UI {

UITabWidget * UITabWidget::New() {
	return eeNew( UITabWidget, () );
}

UITabWidget::UITabWidget() :
	UIWidget( "tabwidget" ),
	mTabSelected( NULL ),
	mTabSelectedIndex( eeINDEX_NOT_FOUND )
{
	setHorizontalAlign( UI_HALIGN_CENTER );

	mTabContainer = UIWidget::New();
	mTabContainer->setPixelsSize( mSize.getWidth(), mStyleConfig.TabWidgetHeight )
			->setParent( this )->setPosition( 0, 0 );
	mTabContainer->clipEnable();

	mCtrlContainer = UIWidget::New();
	mCtrlContainer->setPixelsSize( mSize.getWidth(), mSize.getHeight() - PixelDensity::dpToPx( mStyleConfig.TabWidgetHeight ) )
			->setParent( this )->setPosition( 0, mStyleConfig.TabWidgetHeight );
	mCtrlContainer->clipEnable();

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

	mTabContainer->setThemeSkin( Theme, "tabwidget" );

	mCtrlContainer->setThemeSkin( Theme, "tabcontainer" );

	if ( 0 == mStyleConfig.TabWidgetHeight ) {
		UISkin * tSkin		= Theme->getSkin( "tab" );

		if ( NULL != tSkin ) {
			Sizef tSize1		= getSkinSize( tSkin );
			Sizef tSize2		= getSkinSize( tSkin, UIState::StateFlagSelected );

			mStyleConfig.TabWidgetHeight	= eemax( tSize1.getHeight(), tSize2.getHeight() );

			setContainerSize();
			orderTabs();
		}
	}

	onThemeLoaded();
}

void UITabWidget::onThemeLoaded() {
	onSizeChange();

	UIWidget::onThemeLoaded();
}

void UITabWidget::setContainerSize() {
	mTabContainer->setPixelsSize( mSize.getWidth() - mRealPadding.Left - mRealPadding.Right, mStyleConfig.TabWidgetHeight );
	mTabContainer->setPosition( mPadding.Left, mPadding.Top );
	mCtrlContainer->setPosition( mPadding.Left, mPadding.Top + mStyleConfig.TabWidgetHeight );
	mCtrlContainer->setPixelsSize( mSize.getWidth() - mRealPadding.Left - mRealPadding.Right, mSize.getHeight() - PixelDensity::dpToPx( mStyleConfig.TabWidgetHeight ) - mRealPadding.Top - mRealPadding.Bottom );
}

void UITabWidget::draw() {
	UIWidget::draw();

	if ( mStyleConfig.DrawLineBelowTabs ) {
		bool smooth = GLi->isLineSmooth();
		if ( smooth ) GLi->lineSmooth( false );

		Primitives P;
		Vector2f p1( mScreenPos.x + mRealPadding.Left, mScreenPos.y + mRealPadding.Top + mTabContainer->getPixelsSize().getHeight() + mStyleConfig.LineBelowTabsYOffset );
		Vector2f p2( mScreenPos.x + mTabContainer->getPixelsPosition().x, p1.y );

		P.setLineWidth( PixelDensity::dpToPx( 1 ) );
		P.setColor( Color( mStyleConfig.LineBelowTabsColor, mAlpha ) );
		P.drawLine( Line2f( Vector2f( (int)p1.x, (int)p1.y ), Vector2f( (int)p2.x, (int)p2.y ) ) );

		Vector2f p3( mScreenPos.x + mTabContainer->getPixelsPosition().x + mTabContainer->getPixelsSize().getWidth(), mScreenPos.y + mRealPadding.Top + mTabContainer->getPixelsSize().getHeight() + mStyleConfig.LineBelowTabsYOffset );
		Vector2f p4( mScreenPos.x + mRealPadding.Left + mCtrlContainer->getPixelsSize().getWidth(), p3.y );

		P.drawLine( Line2f( Vector2f( (int)p3.x, (int)p3.y ), Vector2f( (int)p4.x, (int)p4.y ) ) );

		if ( smooth ) GLi->lineSmooth( true );
	}
}

const UITabWidget::StyleConfig& UITabWidget::getStyleConfig() const {
	return mStyleConfig;
}

void UITabWidget::setStyleConfig(const StyleConfig & styleConfig) {
	Uint32		tabWidgetHeight = mStyleConfig.TabWidgetHeight;
	mStyleConfig = styleConfig;
	mStyleConfig.TabWidgetHeight = tabWidgetHeight;
	setContainerSize();
	setTabContainerSize();
	orderTabs();
}

std::string UITabWidget::getPropertyString( const PropertyDefinition* propertyDef ) {
	if ( NULL == propertyDef ) return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::MaxTextLength:
			return String::toStr( getMaxTextLength() );
		case PropertyId::MinTabWidth:
			return String::format( "%ddp", getMinTabWidth() );
		case PropertyId::MaxTabWidth:
			return String::format( "%ddp", getMaxTabWidth() );
		case PropertyId::TabClosable:
			return getTabsClosable() ? "true" : "false";
		case PropertyId::SpecialBorderTabs:
			return getSpecialBorderTabs() ? "true" : "false";
		case PropertyId::LineBelowTabs:
			return getDrawLineBelowTabs() ? "true" : "false";
		case PropertyId::LineBelowTabsColor:
			return getLineBelowTabsColor().toHexString();
		case PropertyId::LineBelowTabsYOffset:
			return String::format( "%ddp", getLineBelowTabsYOffset() );
		case PropertyId::TabSeparation:
			return String::format( "%ddp", getTabSeparation() );
		default:
			return UIWidget::getPropertyString( propertyDef );
	}
}

bool UITabWidget::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) ) return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::MaxTextLength:
			setMaxTextLength( attribute.asUint(1) );
			break;
		case PropertyId::MinTabWidth:
			setMinTabWidth( attribute.asDpDimensionUint("1") );
			break;
		case PropertyId::MaxTabWidth:
			setMaxTabWidth( attribute.asDpDimensionUint() );
			break;
		case PropertyId::TabClosable:
			setTabsClosable( attribute.asBool() );
			break;
		case PropertyId::SpecialBorderTabs:
			setSpecialBorderTabs( attribute.asBool() );
			break;
		case PropertyId::LineBelowTabs:
			setDrawLineBelowTabs( attribute.asBool() );
			break;
		case PropertyId::LineBelowTabsColor:
			setLineBelowTabsColor( attribute.asColor() );
			break;
		case PropertyId::LineBelowTabsYOffset:
			setLineBelowTabsYOffset( attribute.asDpDimensionI() );
			break;
		case PropertyId::TabSeparation:
			setTabSeparation( attribute.asDpDimensionI() );
			break;
		default:
			return UIWidget::applyProperty( attribute );
	}

	return true;
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
	invalidateDraw();
}

Uint32 UITabWidget::getTabWidgetHeight() const {
	return mStyleConfig.TabWidgetHeight;
}

Uint32 UITabWidget::getMinTabWidth() const {
	return mStyleConfig.MinTabWidth;
}

void UITabWidget::setMinTabWidth(const Uint32 & minTabWidth) {
	mStyleConfig.MinTabWidth = minTabWidth;
	invalidateDraw();
}

Uint32 UITabWidget::getMaxTabWidth() const {
	return mStyleConfig.MaxTabWidth;
}

void UITabWidget::setMaxTabWidth(const Uint32 & maxTabWidth) {
	mStyleConfig.MaxTabWidth = maxTabWidth;
	invalidateDraw();
}

bool UITabWidget::getTabsClosable() const {
	return mStyleConfig.TabsClosable;
}

void UITabWidget::setTabsClosable(bool tabsClosable) {
	mStyleConfig.TabsClosable = tabsClosable;
	invalidateDraw();
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
	invalidateDraw();
}

Color UITabWidget::getLineBelowTabsColor() const {
	return mStyleConfig.LineBelowTabsColor;
}

void UITabWidget::setLineBelowTabsColor(const Color & lineBelowTabsColor) {
	mStyleConfig.LineBelowTabsColor = lineBelowTabsColor;
	invalidateDraw();
}

Int32 UITabWidget::getLineBelowTabsYOffset() const {
	return mStyleConfig.LineBelowTabsYOffset;
}

void UITabWidget::setLineBelowTabsYOffset(const Int32 & lineBelowTabsYOffset) {
	mStyleConfig.LineBelowTabsYOffset = lineBelowTabsYOffset;
	invalidateDraw();
}

void UITabWidget::setTabContainerSize() {
	Uint32 s = 0;

	if ( mTabs.size() > 0 ) {
		for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
			s += mTabs[i]->getPixelsSize().getWidth() + mStyleConfig.TabSeparation;
		}

		s -= mStyleConfig.TabSeparation;
	}

	mTabContainer->setPixelsSize( s, PixelDensity::dpToPx( mStyleConfig.TabWidgetHeight ) );

	switch ( HAlignGet( mFlags ) )
	{
		case UI_HALIGN_LEFT:
			mTabContainer->setPosition( 0, 0 );
			break;
		case UI_HALIGN_CENTER:
			mTabContainer->centerHorizontal();
			break;
		case UI_HALIGN_RIGHT:
			mTabContainer->setPosition( getSize().getWidth() - mTabContainer->getSize().getWidth(), 0 );
			break;
	}
}

void UITabWidget::posTabs() {
	Uint32 w	= 0;
	Int32 h	= 0;
	Int32 VA	= VAlignGet( mFlags );

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

	invalidateDraw();
}

UITab * UITabWidget::createTab( const String& Text, UINode * CtrlOwned, Drawable * Icon ) {
	UITab * tCtrl 	= UITab::New();
	tCtrl->setParent( mTabContainer );
	tCtrl->setFlags( UI_VALIGN_CENTER | UI_HALIGN_CENTER | UI_AUTO_SIZE );
	tCtrl->setIcon( Icon );
	tCtrl->setText( Text );
	tCtrl->setVisible( true );
	tCtrl->setEnabled( true );
	tCtrl->setControlOwned( CtrlOwned );
	CtrlOwned->setParent( mCtrlContainer );
	CtrlOwned->setVisible( false );
	CtrlOwned->setEnabled( true );

	if ( CtrlOwned->isWidget() ) {
		UIWidget * widgetOwned = static_cast<UIWidget*>( CtrlOwned );

		widgetOwned->setLayoutSizeRules( FIXED, FIXED );
	}

	return tCtrl;
}

UITabWidget * UITabWidget::add( const String& Text, UINode * CtrlOwned, Drawable * Icon ) {
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
			UITab * tTab = mTabs[i]->asType<UITab>();

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
		mTabSelected->getControlOwned()->setEnabled( false );
	}

	mTabs[ Index ]->close();
	mTabs[ Index ]->setVisible( false );
	mTabs[ Index ]->setEnabled( false );
	mTabs[ Index ] = NULL;

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
		if ( NULL != mTabs[ i ] ) {
			mTabs[ i ]->close();
			mTabs[ i ]->setVisible( false );
			mTabs[ i ]->setEnabled( false );
		}
	}

	mTabs.clear();

	mTabSelected		= NULL;
	mTabSelectedIndex	= eeINDEX_NOT_FOUND;

	orderTabs();
}

void UITabWidget::insert( const String& Text, UINode * CtrlOwned, Drawable * Icon, const Uint32& Index ) {
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

	invalidateDraw();

	if ( Tab == mTabSelected ) {
		refreshControlOwned( Tab );
		return;
	}

	if ( NULL != mTabSelected ) {
		mTabSelected->unselect();

		if ( NULL != mTabSelected->getControlOwned() ) {
			mTabSelected->getControlOwned()->setVisible( false );
			mTabSelected->getControlOwned()->setEnabled( false );
		}
	}

	Tab->select();

	Uint32 TabIndex		= getTabIndex( Tab );

	if ( eeINDEX_NOT_FOUND != TabIndex ) {
		mTabSelected		= Tab;
		mTabSelectedIndex	= TabIndex;

		refreshControlOwned( mTabSelected );

		orderTabs();

		sendCommonEvent( Event::OnTabSelected );
	}
}

void UITabWidget::refreshControlOwned( UITab * tab ) {
	if ( NULL != tab && NULL != tab->getControlOwned() ) {
		tab->getControlOwned()->setParent( mCtrlContainer );
		tab->getControlOwned()->setVisible( tab == mTabSelected );
		tab->getControlOwned()->setEnabled( tab == mTabSelected );
		tab->getControlOwned()->setSize( mCtrlContainer->getSize() );

		if ( tab->getControlOwned()->isWidget() ) {
			UIWidget * widget = static_cast<UIWidget*>( tab->getControlOwned() );

			widget->setPosition( widget->getLayoutMargin().Left, widget->getLayoutMargin().Top );
		} else {
			tab->getControlOwned()->setPosition( 0, 0 );
		}
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
	setContainerSize();
	setTabContainerSize();
	posTabs();

	if ( NULL != mTabSelected && NULL != mTabSelected->getControlOwned() ) {
		mTabSelected->getControlOwned()->setSize( mCtrlContainer->getSize() );
	}

	UIWidget::onSizeChange();
}

void UITabWidget::onChildCountChange() {
	Node * child = mChild;
	bool found = false;

	while ( NULL != child ) {
		if ( !( child == mTabContainer || child == mCtrlContainer ) ) {
			found = true;
			break;
		}

		child = child->getNextNode();
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

	UIWidget::onChildCountChange();
}

void UITabWidget::onPaddingChange() {
	onSizeChange();

	UIWidget::onPaddingChange();
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
