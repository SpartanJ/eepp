#include <eepp/ui/cuitabwidget.hpp>
#include <eepp/ui/cuimanager.hpp>

namespace EE { namespace UI {

cUITabWidget::cUITabWidget( cUITabWidget::CreateParams& Params ) :
	cUIComplexControl( Params ),
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
	mTabSelected( NULL ),
	mTabSelectedIndex( eeINDEX_NOT_FOUND )
{
	cUIComplexControl::CreateParams TabParams;
	TabParams.Parent( this );
	TabParams.PosSet( 0, 0 );
	TabParams.Flags |= UI_CLIP_ENABLE | UI_ANCHOR_RIGHT;
	TabParams.SizeSet( mSize.Width(), mTabWidgetHeight );

	mTabContainer = eeNew( cUIComplexControl, ( TabParams ) );
	mTabContainer->Visible( true );
	mTabContainer->Enabled( true );

	cUIComplexControl::CreateParams CtrlParams;
	CtrlParams.Parent( this );
	CtrlParams.PosSet( 0, mTabWidgetHeight );
	CtrlParams.SizeSet( mSize.Width(), mSize.Height() - mTabWidgetHeight );
	CtrlParams.Flags |= UI_CLIP_ENABLE | UI_ANCHOR_BOTTOM | UI_ANCHOR_RIGHT;

	mCtrlContainer = eeNew( cUIComplexControl, ( CtrlParams ) );
	mCtrlContainer->Visible( true );
	mCtrlContainer->Enabled( true );

	OnSizeChange();

	ApplyDefaultTheme();
}

cUITabWidget::~cUITabWidget() {
}

Uint32 cUITabWidget::Type() const {
	return UI_TYPE_TABWIDGET;
}

bool cUITabWidget::IsType( const Uint32& type ) const {
	return cUITabWidget::Type() == type ? true : cUIComplexControl::IsType( type );
}

void cUITabWidget::SetTheme( cUITheme * Theme ) {
	mTabContainer->SetThemeControl( Theme, "tabwidget" );

	mCtrlContainer->SetThemeControl( Theme, "tabcontainer" );

	if ( 0 == mTabWidgetHeight ) {
		cUISkin * tSkin		= Theme->GetByName( Theme->Abbr() + "_" + "tab" );

		eeSize tSize1		= GetSkinShapeSize( tSkin );
		eeSize tSize2		= GetSkinShapeSize( tSkin, cUISkinState::StateSelected );

		mTabWidgetHeight	= eemax( tSize1.Height(), tSize2.Height() );

		SetContainerSize();
		OrderTabs();
	}

	DoAfterSetTheme();
}

void cUITabWidget::DoAfterSetTheme() {
	OnSizeChange();
}

void cUITabWidget::SetContainerSize() {
	mTabContainer->Size( mSize.Width(), mTabWidgetHeight );
	mCtrlContainer->Pos( 0, mTabWidgetHeight );
	mCtrlContainer->Size( mSize.Width(), mSize.Height() - mTabWidgetHeight );
}

void cUITabWidget::SetTabContainerSize() {
	Uint32 s = 0;

	if ( mTabs.size() > 0 ) {
		for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
			s += mTabs[i]->Size().Width() + mTabSeparation;
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
			mTabContainer->Pos( mSize.Width() - mTabContainer->Size().Width(), 0 );
			break;
	}
}

void cUITabWidget::PosTabs() {
	Uint32 w	= 0;
	Uint32 h	= 0;
	Uint32 VA	= VAlignGet( mFlags );

	for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
		switch ( VA )
		{
			case UI_VALIGN_BOTTOM:
				h = mTabWidgetHeight - mTabs[i]->Size().Height();
				break;
			case UI_VALIGN_TOP:
				h = 0;
				break;
			case UI_VALIGN_CENTER:
				h = mTabWidgetHeight / 2 - mTabs[i]->Size().Height() / 2;
				break;
		}

		mTabs[i]->Pos( w, h );

		w += mTabs[i]->Size().Width() + mTabSeparation;
	}
}

void cUITabWidget::ZOrderTabs() {
	for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
		mTabs[i]->ToBack();
	}

	if ( NULL != mTabSelected ) {
		mTabSelected->ToFront();
	}
}

void cUITabWidget::OrderTabs() {
	ApplyThemeToTabs();

	ZOrderTabs();

	SetTabContainerSize();

	PosTabs();
}

cUITab * cUITabWidget::CreateTab( const String& Text, cUIControl * CtrlOwned, cShape * Icon ) {
	cUITab::CreateParams Params;
	Params.Parent( mTabContainer );
	Params.Font 			= mFont;
	Params.FontColor 		= mFontColor;
	Params.FontShadowColor 	= mFontShadowColor;
	Params.FontOverColor 	= mFontOverColor;
	Params.Icon				= Icon;
	Params.Flags			= UI_VALIGN_CENTER | UI_HALIGN_CENTER | UI_AUTO_SIZE;

	cUITab * tCtrl 	= eeNew( cUITab, ( Params, CtrlOwned ) );

	tCtrl->Text( Text );
	tCtrl->Visible( true );
	tCtrl->Enabled( true );

	CtrlOwned->Parent( mCtrlContainer );
	CtrlOwned->Visible( false );
	CtrlOwned->Enabled( true );

	return tCtrl;
}

Uint32 cUITabWidget::Add( const String& Text, cUIControl * CtrlOwned, cShape * Icon ) {
	return Add( CreateTab( Text, CtrlOwned, Icon ) );
}

Uint32 cUITabWidget::Add( cUITab * Tab ) {
	Tab->Parent( mTabContainer );

	mTabs.push_back( Tab );

	if ( NULL == mTabSelected ) {
		SetTabSelected( Tab );
	} else {
		OrderTabs();
	}

	return mTabs.size() - 1;
}

cUITab * cUITabWidget::GetTab( const Uint32& Index ) {
	eeASSERT( Index < mTabs.size() );
	return mTabs[ Index ];
}

cUITab * cUITabWidget::GetTab( const String& Text ) {
	for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
		if ( mTabs[i]->IsType( UI_TYPE_TAB ) ) {
			cUITab * tTab = reinterpret_cast<cUITab*>( mTabs[i] );

			if ( tTab->Text() == Text )
				return tTab;
		}
	}

	return NULL;
}

Uint32 cUITabWidget::GetTabIndex( cUITab * Tab ) {
	for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
		if ( mTabs[i] == Tab )
			return i;
	}

	return eeINDEX_NOT_FOUND;
}

Uint32 cUITabWidget::Count() const {
	return mTabs.size();
}

void cUITabWidget::Remove( const Uint32& Index ) {
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

void cUITabWidget::Remove( cUITab * Tab ) {
	Remove( GetTabIndex( Tab ) );
}

void cUITabWidget::RemoveAll() {
	for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
		eeSAFE_DELETE( mTabs[ i ] );
	}

	mTabs.clear();

	mTabSelected		= NULL;
	mTabSelectedIndex	= eeINDEX_NOT_FOUND;

	OrderTabs();
}

void cUITabWidget::Insert( const String& Text, cUIControl * CtrlOwned, cShape * Icon, const Uint32& Index ) {
	Insert( CreateTab( Text, CtrlOwned, Icon ), Index );
}

void cUITabWidget::Insert( cUITab * Tab, const Uint32& Index ) {
	mTabs.insert( mTabs.begin() + Index, Tab );

	ChildAddAt( Tab, Index );

	OrderTabs();
}

void cUITabWidget::SetTabSelected( cUITab * Tab ) {
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

		SendCommonEvent( cUIEvent::EventOnTabSelected );
	}
}

void cUITabWidget::SelectPrev() {
	if ( eeINDEX_NOT_FOUND != mTabSelectedIndex && mTabSelectedIndex > 0 ) {
		SetTabSelected( GetTab( mTabSelectedIndex - 1 ) );
	}
}

void cUITabWidget::SelectNext() {
	if ( mTabSelectedIndex + 1 < mTabs.size() ) {
		SetTabSelected( GetTab( mTabSelectedIndex + 1 ) );
	}
}

cUITab * cUITabWidget::GetSelectedTab() const {
	return mTabSelected;
}

Uint32 cUITabWidget::GetSelectedTabIndex() const {
	return mTabSelectedIndex;
}

void cUITabWidget::OnSizeChange() {
	SetContainerSize();
	SetTabContainerSize();
	PosTabs();

	if ( NULL != mTabSelected ) {
		mTabSelected->CtrlOwned()->Size( mCtrlContainer->Size() );
	}

	cUIControl::OnSizeChange();
}

void cUITabWidget::ApplyThemeToTabs() {
	if ( mSpecialBorderTabs ) {
		for ( Uint32 i = 0; i < mTabs.size(); i++ ) {
			mTabs[ i ]->ApplyDefaultTheme();
		}
	}
}

cUIComplexControl * cUITabWidget::TabContainer() const {
	return mTabContainer;
}

cUIComplexControl * cUITabWidget::ControlContainer() const {
	return mCtrlContainer;
}


}}
