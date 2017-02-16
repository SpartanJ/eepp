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
	TabParams.Parent( this );
	TabParams.PosSet( 0, 0 );
	TabParams.Flags |= UI_CLIP_ENABLE | UI_ANCHOR_RIGHT;
	TabParams.SizeSet( mSize.width(), mTabWidgetHeight );

	mTabContainer = eeNew( UIComplexControl, ( TabParams ) );
	mTabContainer->Visible( true );
	mTabContainer->Enabled( true );

	UIComplexControl::CreateParams CtrlParams;
	CtrlParams.Parent( this );
	CtrlParams.PosSet( 0, mTabWidgetHeight );
	CtrlParams.SizeSet( mSize.width(), mSize.height() - mTabWidgetHeight );
	CtrlParams.Flags |= UI_CLIP_ENABLE | UI_ANCHOR_BOTTOM | UI_ANCHOR_RIGHT;

	mCtrlContainer = eeNew( UIComplexControl, ( CtrlParams ) );
	mCtrlContainer->Visible( true );
	mCtrlContainer->Enabled( true );

	OnSizeChange();

	ApplyDefaultTheme();
}

UITabWidget::~UITabWidget() {
}

Uint32 UITabWidget::Type() const {
	return UI_TYPE_TABWIDGET;
}

bool UITabWidget::IsType( const Uint32& type ) const {
	return UITabWidget::Type() == type ? true : UIComplexControl::IsType( type );
}

void UITabWidget::SetTheme( UITheme * Theme ) {
	mTabContainer->SetThemeControl( Theme, "tabwidget" );

	mCtrlContainer->SetThemeControl( Theme, "tabcontainer" );

	if ( 0 == mTabWidgetHeight ) {
		UISkin * tSkin		= Theme->getByName( Theme->Abbr() + "_" + "tab" );

		Sizei tSize1		= GetSkinSize( tSkin );
		Sizei tSize2		= GetSkinSize( tSkin, UISkinState::StateSelected );

		mTabWidgetHeight	= eemax( tSize1.height(), tSize2.height() );

		SeContainerSize();
		OrderTabs();
	}

	DoAfterSetTheme();
}

void UITabWidget::DoAfterSetTheme() {
	OnSizeChange();
}

void UITabWidget::SeContainerSize() {
	mTabContainer->Size( mSize.width(), mTabWidgetHeight );
	mCtrlContainer->Pos( 0, mTabWidgetHeight );
	mCtrlContainer->Size( mSize.width(), mSize.height() - mTabWidgetHeight );
}

void UITabWidget::Draw() {
	if ( mDrawLineBelowTabs ) {
		bool smooth = GLi->isLineSmooth();
		if ( smooth ) GLi->lineSmooth( false );

		Primitives P;
		Vector2i p1( mPos.x, mPos.y + mTabContainer->Size().height() + mLineBewowTabsYOffset );
		Vector2i p2( mPos.x + mTabContainer->Pos().x, p1.y );

		ControlToScreen( p1 );
		ControlToScreen( p2 );

		P.lineWidth( 1 );
		P.setColor( mLineBelowTabsColor );
		P.drawLine( Line2f( Vector2f( p1.x, p1.y ), Vector2f( p2.x, p2.y ) ) );

		Vector2i p3( mPos.x + mTabContainer->Pos().x + mTabContainer->Size().width(), mPos.y + mTabContainer->Size().height() + mLineBewowTabsYOffset );
		Vector2i p4( mPos.x + mSize.width(), p3.y );

		ControlToScreen( p3 );
		ControlToScreen( p4 );

		P.drawLine( Line2f( Vector2f( p3.x, p3.y ), Vector2f( p4.x, p4.y ) ) );

		if ( smooth ) GLi->lineSmooth( true );
	}
}

void UITabWidget::SetTabContainerSize() {
	Uint32 s = 0;

	if ( mTabs.size() > 0 ) {
		for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
			s += mTabs[i]->Size().width() + mTabSeparation;
		}

		s -= mTabSeparation;
	}

	mTabContainer->Size( s, mTabWidgetHeight );

	switch ( HAlignGet( mFlags ) )
	{
		case UI_HALIGN_LEFT:
			mTabContainer->Pos( 0, 0 );
			break;
		case UI_HALIGN_CENTER:
			mTabContainer->CenterHorizontal();
			break;
		case UI_HALIGN_RIGHT:
			mTabContainer->Pos( mSize.width() - mTabContainer->Size().width(), 0 );
			break;
	}
}

void UITabWidget::PosTabs() {
	Uint32 w	= 0;
	Uint32 h	= 0;
	Uint32 VA	= VAlignGet( mFlags );

	for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
		switch ( VA )
		{
			case UI_VALIGN_BOTTOM:
				h = mTabWidgetHeight - mTabs[i]->Size().height();
				break;
			case UI_VALIGN_TOP:
				h = 0;
				break;
			case UI_VALIGN_CENTER:
				h = mTabWidgetHeight / 2 - mTabs[i]->Size().height() / 2;
				break;
		}

		mTabs[i]->Pos( w, h );

		w += mTabs[i]->Size().width() + mTabSeparation;
	}
}

void UITabWidget::ZOrderTabs() {
	for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
		mTabs[i]->ToBack();
	}

	if ( NULL != mTabSelected ) {
		mTabSelected->ToFront();
	}
}

void UITabWidget::OrderTabs() {
	ApplyThemeToTabs();

	ZOrderTabs();

	SetTabContainerSize();

	PosTabs();
}

UITab * UITabWidget::CreateTab( const String& Text, UIControl * CtrlOwned, SubTexture * Icon ) {
	UITab::CreateParams Params;
	Params.Parent( mTabContainer );
	Params.Font 			= mFont;
	Params.FontColor 		= mFontColor;
	Params.FontShadowColor 	= mFontShadowColor;
	Params.FontOverColor 	= mFontOverColor;
	Params.Icon				= Icon;
	Params.Flags			= UI_VALIGN_CENTER | UI_HALIGN_CENTER | UI_AUTO_SIZE;

	UITab * tCtrl 	= eeNew( UITab, ( Params, CtrlOwned ) );

	tCtrl->Text( Text );
	tCtrl->Visible( true );
	tCtrl->Enabled( true );

	CtrlOwned->Parent( mCtrlContainer );
	CtrlOwned->Visible( false );
	CtrlOwned->Enabled( true );

	return tCtrl;
}

Uint32 UITabWidget::Add( const String& Text, UIControl * CtrlOwned, SubTexture * Icon ) {
	return Add( CreateTab( Text, CtrlOwned, Icon ) );
}

Uint32 UITabWidget::Add( UITab * Tab ) {
	Tab->Parent( mTabContainer );

	mTabs.push_back( Tab );

	if ( NULL == mTabSelected ) {
		SetTabSelected( Tab );
	} else {
		OrderTabs();
	}

	return mTabs.size() - 1;
}

UITab * UITabWidget::GetTab( const Uint32& Index ) {
	eeASSERT( Index < mTabs.size() );
	return mTabs[ Index ];
}

UITab * UITabWidget::GetTab( const String& Text ) {
	for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
		if ( mTabs[i]->IsType( UI_TYPE_TAB ) ) {
			UITab * tTab = reinterpret_cast<UITab*>( mTabs[i] );

			if ( tTab->Text() == Text )
				return tTab;
		}
	}

	return NULL;
}

Uint32 UITabWidget::GetTabIndex( UITab * Tab ) {
	for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
		if ( mTabs[i] == Tab )
			return i;
	}

	return eeINDEX_NOT_FOUND;
}

Uint32 UITabWidget::Count() const {
	return mTabs.size();
}

void UITabWidget::Remove( const Uint32& Index ) {
	eeASSERT( Index < mTabs.size() );

	if ( mTabs[ Index ] == mTabSelected ) {
		mTabSelected->CtrlOwned()->Visible( false );
	}

	eeSAFE_DELETE( mTabs[ Index ] );

	mTabs.erase( mTabs.begin() + Index );

	mTabSelected = NULL;

	if ( Index == mTabSelectedIndex ) {
		if ( mTabs.size() > 0 ) {
			if ( mTabSelectedIndex < mTabs.size() ) {
				SetTabSelected( mTabs[ mTabSelectedIndex ] );
			} else {
				if ( mTabSelectedIndex > 0 && mTabSelectedIndex - 1 < mTabs.size() ) {
					SetTabSelected( mTabs[ mTabSelectedIndex - 1 ] );
				} else {
					SetTabSelected( mTabs[ 0 ] );
				}
			}
		} else {
			mTabSelected		= NULL;
			mTabSelectedIndex	= eeINDEX_NOT_FOUND;
		}
	}

	OrderTabs();
}

void UITabWidget::Remove( UITab * Tab ) {
	Remove( GetTabIndex( Tab ) );
}

void UITabWidget::RemoveAll() {
	for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
		eeSAFE_DELETE( mTabs[ i ] );
	}

	mTabs.clear();

	mTabSelected		= NULL;
	mTabSelectedIndex	= eeINDEX_NOT_FOUND;

	OrderTabs();
}

void UITabWidget::Insert( const String& Text, UIControl * CtrlOwned, SubTexture * Icon, const Uint32& Index ) {
	Insert( CreateTab( Text, CtrlOwned, Icon ), Index );
}

void UITabWidget::Insert( UITab * Tab, const Uint32& Index ) {
	mTabs.insert( mTabs.begin() + Index, Tab );

	ChildAddAt( Tab, Index );

	OrderTabs();
}

void UITabWidget::SetTabSelected( UITab * Tab ) {
	if ( Tab == mTabSelected ) {
		return;
	}

	if ( NULL != mTabSelected ) {
		mTabSelected->Unselect();
		mTabSelected->CtrlOwned()->Visible( false );
	}

	if ( NULL != Tab ) {
		Tab->Select();
	} else {
		return;
	}

	Uint32 TabIndex		= GetTabIndex( Tab );

	if ( eeINDEX_NOT_FOUND != TabIndex ) {
		mTabSelected		= Tab;
		mTabSelectedIndex	= TabIndex;

		mTabSelected->CtrlOwned()->Visible( true );
		mTabSelected->CtrlOwned()->Size( mCtrlContainer->Size() );
		mTabSelected->CtrlOwned()->Pos( 0, 0 );

		OrderTabs();

		SendCommonEvent( UIEvent::EventOnTabSelected );
	}
}

void UITabWidget::SelectPrev() {
	if ( eeINDEX_NOT_FOUND != mTabSelectedIndex && mTabSelectedIndex > 0 ) {
		SetTabSelected( GetTab( mTabSelectedIndex - 1 ) );
	}
}

void UITabWidget::SelectNext() {
	if ( mTabSelectedIndex + 1 < mTabs.size() ) {
		SetTabSelected( GetTab( mTabSelectedIndex + 1 ) );
	}
}

UITab * UITabWidget::GetSelectedTab() const {
	return mTabSelected;
}

Uint32 UITabWidget::GetSelectedTabIndex() const {
	return mTabSelectedIndex;
}

void UITabWidget::OnSizeChange() {
	SeContainerSize();
	SetTabContainerSize();
	PosTabs();

	if ( NULL != mTabSelected ) {
		mTabSelected->CtrlOwned()->Size( mCtrlContainer->Size() );
	}

	UIControl::OnSizeChange();
}

void UITabWidget::ApplyThemeToTabs() {
	if ( mSpecialBorderTabs ) {
		for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
			mTabs[ i ]->ApplyDefaultTheme();
		}
	}
}

UIComplexControl * UITabWidget::TabContainer() const {
	return mTabContainer;
}

UIComplexControl * UITabWidget::ControlContainer() const {
	return mCtrlContainer;
}


}}
