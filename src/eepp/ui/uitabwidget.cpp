#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/graphics/renderer/gl.hpp>
#include <eepp/graphics/primitives.hpp>

namespace EE { namespace UI {

UITabWidget::UITabWidget( UITabWidget::CreateParams& Params ) :
	UIComplexControl( Params ),
	mFont( Params.Font ),
	mFontColor( Params.FontColor ),
	mFontShadowColor( Params.FontShadowColor ),
	mFontOverColor( Params.FontOverColor ),
	mFontSelectedColor( Params.FontSelectedColor ),
	mTabSeparation( Params.TabSeparation ),
	mMaxTextLength( Params.MaxTextLength ),
	mTabWidgetHeight( Params.TabWidgetHeight ),
	mMinTabWidth( Params.MinTabWidth ),
	mMaxTabWidth( Params.MaxTabWidth ),
	mTabsClosable( Params.TabsClosable ),
	mSpecialBorderTabs( Params.SpecialBorderTabs ),
	mDrawLineBelowTabs( Params.DrawLineBelowTabs ),
	mLineBelowTabsColor( Params.LineBelowTabsColor ),
	mLineBewowTabsYOffset( Params.LineBewowTabsYOffset ),
	mTabSelected( NULL ),
	mTabSelectedIndex( eeINDEX_NOT_FOUND )
{
	UIComplexControl::CreateParams TabParams;
	TabParams.setParent( this );
	TabParams.setPosition( 0, 0 );
	TabParams.Flags |= UI_CLIP_ENABLE | UI_ANCHOR_RIGHT;
	TabParams.setSize( mSize.getWidth(), mTabWidgetHeight );

	mTabContainer = eeNew( UIComplexControl, ( TabParams ) );
	mTabContainer->setVisible( true );
	mTabContainer->setEnabled( true );

	UIComplexControl::CreateParams CtrlParams;
	CtrlParams.setParent( this );
	CtrlParams.setPosition( 0, mTabWidgetHeight );
	CtrlParams.setSize( mSize.getWidth(), mSize.getHeight() - mTabWidgetHeight );
	CtrlParams.Flags |= UI_CLIP_ENABLE | UI_ANCHOR_BOTTOM | UI_ANCHOR_RIGHT;

	mCtrlContainer = eeNew( UIComplexControl, ( CtrlParams ) );
	mCtrlContainer->setVisible( true );
	mCtrlContainer->setEnabled( true );

	onSizeChange();

	applyDefaultTheme();
}

UITabWidget::UITabWidget() :
	UIComplexControl(),
	mFont( NULL ),
	mFontColor(),
	mFontShadowColor(),
	mFontOverColor(),
	mFontSelectedColor(),
	mTabSeparation( 0 ),
	mMaxTextLength( 32 ),
	mTabWidgetHeight( 64 ),
	mMinTabWidth( 32 ),
	mMaxTabWidth( 210 ),
	mTabsClosable( false ),
	mSpecialBorderTabs( false ),
	mDrawLineBelowTabs( false ),
	mLineBelowTabsColor( ColorA::Black ),
	mLineBewowTabsYOffset( 0 ),
	mTabSelected( NULL ),
	mTabSelectedIndex( eeINDEX_NOT_FOUND )
{
	UITheme * Theme = UIThemeManager::instance()->getDefaultTheme();

	if ( NULL != Theme ) {
		mFont				= Theme->getFont();
		mFontColor			= Theme->getFontColor();
		mFontShadowColor	= Theme->getFontShadowColor();
		mFontOverColor		= Theme->getFontOverColor();
		mFontSelectedColor	= Theme->getFontSelectedColor();
	}

	if ( NULL == mFont )
		mFont = UIThemeManager::instance()->getDefaultFont();

	mTabContainer = eeNew( UIComplexControl, ( ) );
	mTabContainer->setParent( this )->setPosition( 0, 0 )->setSize( mSize.getWidth(), mTabWidgetHeight )->setVisible( true )->setEnabled( true );
	mTabContainer->setFlags( UI_CLIP_ENABLE | UI_ANCHOR_RIGHT );

	mCtrlContainer = eeNew( UIComplexControl, ( ) );
	mCtrlContainer->setParent( this )->setPosition( 0, mTabWidgetHeight )
			->setSize( mSize.getWidth(), mSize.getHeight() - mTabWidgetHeight )->setVisible( true )->setEnabled( true )
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

	if ( 0 == mTabWidgetHeight ) {
		UISkin * tSkin		= Theme->getByName( Theme->getAbbr() + "_" + "tab" );

		if ( NULL != tSkin ) {
			Sizei tSize1		= getSkinSize( tSkin );
			Sizei tSize2		= getSkinSize( tSkin, UISkinState::StateSelected );

			mTabWidgetHeight	= eemax( tSize1.getHeight(), tSize2.getHeight() );

			seContainerSize();
			orderTabs();
		}
	}

	doAftersetTheme();
}

void UITabWidget::doAftersetTheme() {
	onSizeChange();
}

void UITabWidget::seContainerSize() {
	mTabContainer->setSize( mSize.getWidth(), mTabWidgetHeight );
	mCtrlContainer->setPosition( 0, mTabWidgetHeight );
	mCtrlContainer->setSize( mSize.getWidth(), mSize.getHeight() - mTabWidgetHeight );
}

void UITabWidget::draw() {
	if ( mDrawLineBelowTabs ) {
		bool smooth = GLi->isLineSmooth();
		if ( smooth ) GLi->lineSmooth( false );

		Primitives P;
		Vector2i p1( mRealPos.x, mRealPos.y + mTabContainer->getRealSize().getHeight() + mLineBewowTabsYOffset );
		Vector2i p2( mRealPos.x + mTabContainer->getRealPosition().x, p1.y );

		controlToScreen( p1 );
		controlToScreen( p2 );

		P.setLineWidth( 1 );
		P.setColor( mLineBelowTabsColor );
		P.drawLine( Line2f( Vector2f( p1.x, p1.y ), Vector2f( p2.x, p2.y ) ) );

		Vector2i p3( mRealPos.x + mTabContainer->getRealPosition().x + mTabContainer->getRealSize().getWidth(), mRealPos.y + mTabContainer->getRealSize().getHeight() + mLineBewowTabsYOffset );
		Vector2i p4( mRealPos.x + mRealSize.getWidth(), p3.y );

		controlToScreen( p3 );
		controlToScreen( p4 );

		P.drawLine( Line2f( Vector2f( p3.x, p3.y ), Vector2f( p4.x, p4.y ) ) );

		if ( smooth ) GLi->lineSmooth( true );
	}
}

Font * UITabWidget::getFont() const {
	return mFont;
}

void UITabWidget::setFont(Font * font) {
	mFont = font;

	if ( mTabs.size() > 0 ) {
		for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
			((UITab*)mTabs[ i ])->setFont( mFont );
		}
	}
}

ColorA UITabWidget::getFontColor() const {
	return mFontColor;
}

void UITabWidget::setFontColor(const ColorA & fontColor) {
	mFontColor = fontColor;

	if ( mTabs.size() > 0 ) {
		for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
			((UITab*)mTabs[ i ])->setFontColor( mFontColor );
		}
	}
}

ColorA UITabWidget::getFontShadowColor() const {
	return mFontShadowColor;
}

void UITabWidget::setFontShadowColor(const ColorA & fontShadowColor) {
	mFontShadowColor = fontShadowColor;

	if ( mTabs.size() > 0 ) {
		for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
			((UITab*)mTabs[ i ])->setFontShadowColor( mFontShadowColor );
		}
	}
}

ColorA UITabWidget::getFontOverColor() const {
	return mFontOverColor;
}

void UITabWidget::setFontOverColor(const ColorA & fontOverColor) {
	mFontOverColor = fontOverColor;

	if ( mTabs.size() > 0 ) {
		for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
			((UITab*)mTabs[ i ])->setFontOverColor( mFontOverColor );
		}
	}
}

ColorA UITabWidget::getFontSelectedColor() const {
	return mFontSelectedColor;
}

void UITabWidget::setFontSelectedColor(const ColorA & fontSelectedColor) {
	mFontSelectedColor = fontSelectedColor;

	if ( mTabs.size() > 0 ) {
		for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
			((UITab*)mTabs[ i ])->setFontSelectedColor( mFontSelectedColor );
		}
	}
}

Int32 UITabWidget::getTabSeparation() const {
	return mTabSeparation;
}

void UITabWidget::setTabSeparation(const Int32 & tabSeparation) {
	mTabSeparation = tabSeparation;
	setTabContainerSize();
	posTabs();
}

Uint32 UITabWidget::getMaxTextLength() const {
	return mMaxTextLength;
}

void UITabWidget::setMaxTextLength(const Uint32 & maxTextLength) {
	mMaxTextLength = maxTextLength;
}

Uint32 UITabWidget::getTabWidgetHeight() const {
	return mTabWidgetHeight;
}

Uint32 UITabWidget::getMinTabWidth() const
{
	return mMinTabWidth;
}

void UITabWidget::setMinTabWidth(const Uint32 & minTabWidth) {
	mMinTabWidth = minTabWidth;
}

Uint32 UITabWidget::getMaxTabWidth() const {
	return mMaxTabWidth;
}

void UITabWidget::setMaxTabWidth(const Uint32 & maxTabWidth) {
	mMaxTabWidth = maxTabWidth;
}

bool UITabWidget::getTabsClosable() const {
	return mTabsClosable;
}

void UITabWidget::setTabsClosable(bool tabsClosable) {
	mTabsClosable = tabsClosable;
}

bool UITabWidget::getSpecialBorderTabs() const {
	return mSpecialBorderTabs;
}

void UITabWidget::setSpecialBorderTabs(bool specialBorderTabs) {
	mSpecialBorderTabs = specialBorderTabs;
	applyThemeToTabs();
}

bool UITabWidget::getDrawLineBelowTabs() const {
	return mDrawLineBelowTabs;
}

void UITabWidget::setDrawLineBelowTabs(bool drawLineBelowTabs) {
	mDrawLineBelowTabs = drawLineBelowTabs;
}

ColorA UITabWidget::getLineBelowTabsColor() const {
	return mLineBelowTabsColor;
}

void UITabWidget::setLineBelowTabsColor(const ColorA & lineBelowTabsColor) {
	mLineBelowTabsColor = lineBelowTabsColor;
}

Int32 UITabWidget::getLineBewowTabsYOffset() const {
	return mLineBewowTabsYOffset;
}

void UITabWidget::setLineBewowTabsYOffset(const Int32 & lineBewowTabsYOffset) {
	mLineBewowTabsYOffset = lineBewowTabsYOffset;
}

void UITabWidget::setTabContainerSize() {
	Uint32 s = 0;

	if ( mTabs.size() > 0 ) {
		for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
			s += mTabs[i]->getSize().getWidth() + mTabSeparation;
		}

		s -= mTabSeparation;
	}

	mTabContainer->setSize( s, mTabWidgetHeight );

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
				h = mTabWidgetHeight - mTabs[i]->getSize().getHeight();
				break;
			case UI_VALIGN_TOP:
				h = 0;
				break;
			case UI_VALIGN_CENTER:
				h = mTabWidgetHeight / 2 - mTabs[i]->getSize().getHeight() / 2;
				break;
		}

		mTabs[i]->setPosition( w, h );

		w += mTabs[i]->getSize().getWidth() + mTabSeparation;
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
	UITab * tCtrl 	= eeNew( UITab, ( ) );
	tCtrl->setParent( mTabContainer );
	tCtrl->setFlags( UI_VALIGN_CENTER | UI_HALIGN_CENTER | UI_AUTO_SIZE );
	tCtrl->setFont( mFont );
	tCtrl->setFontColor( mFontColor );
	tCtrl->setFontShadowColor( mFontShadowColor );
	tCtrl->setFontOverColor( mFontOverColor );
	tCtrl->setFontSelectedColor( mFontSelectedColor );
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
	if ( mSpecialBorderTabs ) {
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
