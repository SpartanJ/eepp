#include <eepp/ui/cuitab.hpp>
#include <eepp/ui/cuitabwidget.hpp>
#include <eepp/ui/cuimanager.hpp>

namespace EE { namespace UI {

cUITab::cUITab( cUISelectButton::CreateParams& Params, cUIControl * CtrlOwned ) :
	cUISelectButton( Params ),
	mCtrlOwned( CtrlOwned )
{
	ApplyDefaultTheme();
}

cUITab::~cUITab() {
}

Uint32 cUITab::Type() const {
	return UI_TYPE_TAB;
}

bool cUITab::IsType( const Uint32& type ) const {
	return cUITab::Type() == type ? true : cUISelectButton::IsType( type );
}

cUITabWidget * cUITab::GetTabWidget() {
	if ( Parent()->Parent()->IsType( UI_TYPE_TABWIDGET ) ) {
		return reinterpret_cast<cUITabWidget*> ( Parent()->Parent() );
	}

	return NULL;
}

void cUITab::SetTheme( cUITheme * Theme ) {
	std::string tabPos = "tab";

	cUITabWidget * tTabW = GetTabWidget();

	if ( NULL != tTabW ) {
		if ( tTabW->mSpecialBorderTabs ) {
			if ( 0 == tTabW->GetTabIndex( this ) ) {
				tabPos = "tab_left";
			} else if ( tTabW->Count() > 0 && ( tTabW->Count() - 1 ) == tTabW->GetTabIndex( this ) ) {
				tabPos = "tab_right";
			}
		}
	}

	cUIControl::SetThemeControl( Theme, tabPos );

	DoAfterSetTheme();
}

Uint32 cUITab::OnMouseClick( const eeVector2i &Pos, const Uint32 Flags ) {
	cUISelectButton::OnMouseClick( Pos, Flags );

	cUITabWidget * tTabW = GetTabWidget();

	if ( NULL != tTabW ) {
		if ( Flags & EE_BUTTON_LMASK ) {
			tTabW->SetTabSelected( this );
		}
	}

	return 1;
}

void cUITab::OnStateChange() {
	cUISelectButton::OnStateChange();

	cUITabWidget * tTabW = GetTabWidget();

	if ( NULL != tTabW ) {
		Size( mSize.Width(), GetSkinSize( GetSkin(), mSkinState->GetState() ).Height() );

		if ( mSkinState->GetState() == cUISkinState::StateSelected ) {
			mTextBox->Color( tTabW->mFontSelectedColor );
		} else if ( mSkinState->GetState() == cUISkinState::StateMouseEnter ) {
			mTextBox->Color( tTabW->mFontOverColor );
		} else {
			mTextBox->Color( tTabW->mFontColor );
		}
	}
}

const String& cUITab::Text() {
	return cUIPushButton::Text();
}

void cUITab::Text( const String &text ) {
	cUITabWidget * tTabW = GetTabWidget();

	if ( NULL != tTabW ) {
		if ( text.size() > tTabW->mMaxTextLength ) {
			cUIPushButton::Text( text.substr( 0, tTabW->mMaxTextLength ) );

			SetRealSize();

			return;
		}
	}

	cUIPushButton::Text( text );

	SetRealSize();
}

void cUITab::SetRealSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		Uint32 w = mTextBox->GetTextWidth() + GetSkinSize().Width();

		cUITabWidget * tTabW = GetTabWidget();

		if ( NULL != tTabW ) {
			w = eemax( w, tTabW->mMinTabWidth );
			w = eemin( w, tTabW->mMaxTabWidth );
		}

		Size( w, mSize.Height() );
	}
}

cUIControl * cUITab::CtrlOwned() const {
	return mCtrlOwned;
}

void cUITab::Update() {
	cUISelectButton::Update();

	if ( mEnabled && mVisible ) {
		if ( IsMouseOver() ) {
			cUITabWidget * tTabW	= GetTabWidget();

			if ( NULL != tTabW ) {
				Uint32 Flags 			= cUIManager::instance()->GetInput()->ClickTrigger();

				if ( Flags & EE_BUTTONS_WUWD ) {
					if ( Flags & EE_BUTTON_WUMASK ) {
						tTabW->SelectPrev();
					} else if ( Flags & EE_BUTTON_WDMASK ) {
						tTabW->SelectNext();
					}
				} else if ( tTabW->mTabsClosable && ( Flags & EE_BUTTON_MMASK ) ) {
					tTabW->Remove( this );
				}
			}
		}
	}
}

}}
