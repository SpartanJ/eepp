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
		UISkin * tSkin		= Theme->getByName( Theme->abbr() + "_" + "tab" );

		Sizei tSize1		= getSkinSize( tSkin );
		Sizei tSize2		= getSkinSize( tSkin, UISkinState::StateSelected );

		mTabWidgetHeight	= eemax( tSize1.getHeight(), tSize2.getHeight() );

		seContainerSize();
		orderTabs();
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
		Vector2i p1( mPos.x, mPos.y + mTabContainer->getSize().getHeight() + mLineBewowTabsYOffset );
		Vector2i p2( mPos.x + mTabContainer->getPosition().x, p1.y );

		controlToScreen( p1 );
		controlToScreen( p2 );

		P.setLineWidth( 1 );
		P.setColor( mLineBelowTabsColor );
		P.drawLine( Line2f( Vector2f( p1.x, p1.y ), Vector2f( p2.x, p2.y ) ) );

		Vector2i p3( mPos.x + mTabContainer->getPosition().x + mTabContainer->getSize().getWidth(), mPos.y + mTabContainer->getSize().getHeight() + mLineBewowTabsYOffset );
		Vector2i p4( mPos.x + mSize.getWidth(), p3.y );

		controlToScreen( p3 );
		controlToScreen( p4 );

		P.drawLine( Line2f( Vector2f( p3.x, p3.y ), Vector2f( p4.x, p4.y ) ) );

		if ( smooth ) GLi->lineSmooth( true );
	}
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

		w += mTabs[i]->getSize().getHeight() + mTabSeparation;
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
	UITab::CreateParams Params;
	Params.setParent( mTabContainer );
	Params.Font 			= mFont;
	Params.FontColor 		= mFontColor;
	Params.FontShadowColor 	= mFontShadowColor;
	Params.FontOverColor 	= mFontOverColor;
	Params.Icon				= Icon;
	Params.Flags			= UI_VALIGN_CENTER | UI_HALIGN_CENTER | UI_AUTO_SIZE;

	UITab * tCtrl 	= eeNew( UITab, ( Params, CtrlOwned ) );

	tCtrl->text( Text );
	tCtrl->setVisible( true );
	tCtrl->setEnabled( true );

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

			if ( tTab->text() == Text )
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

Uint32 UITabWidget::count() const {
	return mTabs.size();
}

void UITabWidget::remove( const Uint32& Index ) {
	eeASSERT( Index < mTabs.size() );

	if ( mTabs[ Index ] == mTabSelected ) {
		mTabSelected->ctrlOwned()->setVisible( false );
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
		mTabSelected->ctrlOwned()->setVisible( false );
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

		mTabSelected->ctrlOwned()->setVisible( true );
		mTabSelected->ctrlOwned()->setSize( mCtrlContainer->getSize() );
		mTabSelected->ctrlOwned()->setPosition( 0, 0 );

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
		mTabSelected->ctrlOwned()->setSize( mCtrlContainer->getSize() );
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
