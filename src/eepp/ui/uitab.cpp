#include <eepp/ui/uitab.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UITab::UITab( UISelectButton::CreateParams& Params, UIControl * CtrlOwned ) :
	UISelectButton( Params ),
	mCtrlOwned( CtrlOwned )
{
	ApplyDefaultTheme();
}

UITab::~UITab() {
}

Uint32 UITab::Type() const {
	return UI_TYPE_TAB;
}

bool UITab::IsType( const Uint32& type ) const {
	return UITab::Type() == type ? true : UISelectButton::IsType( type );
}

UITabWidget * UITab::GetTabWidget() {
	if ( Parent()->Parent()->IsType( UI_TYPE_TABWIDGET ) ) {
		return reinterpret_cast<UITabWidget*> ( Parent()->Parent() );
	}

	return NULL;
}

void UITab::SetTheme( UITheme * Theme ) {
	std::string tabPos = "tab";

	UITabWidget * tTabW = GetTabWidget();

	if ( NULL != tTabW ) {
		if ( tTabW->mSpecialBorderTabs ) {
			if ( 0 == tTabW->GetTabIndex( this ) ) {
				tabPos = "tab_left";
			} else if ( tTabW->Count() > 0 && ( tTabW->Count() - 1 ) == tTabW->GetTabIndex( this ) ) {
				tabPos = "tab_right";
			}
		}
	}

	UIControl::SetThemeControl( Theme, tabPos );

	DoAfterSetTheme();
}

Uint32 UITab::OnMouseClick( const Vector2i &Pos, const Uint32 Flags ) {
	UISelectButton::OnMouseClick( Pos, Flags );

	UITabWidget * tTabW = GetTabWidget();

	if ( NULL != tTabW ) {
		if ( Flags & EE_BUTTON_LMASK ) {
			tTabW->SetTabSelected( this );
		}
	}

	return 1;
}

void UITab::OnStateChange() {
	UISelectButton::OnStateChange();

	UITabWidget * tTabW = GetTabWidget();

	if ( NULL != tTabW ) {
		Size( mSize.width(), GetSkinSize( GetSkin(), mSkinState->GetState() ).height() );

		if ( mSkinState->GetState() == UISkinState::StateSelected ) {
			mTextBox->Color( tTabW->mFontSelectedColor );
		} else if ( mSkinState->GetState() == UISkinState::StateMouseEnter ) {
			mTextBox->Color( tTabW->mFontOverColor );
		} else {
			mTextBox->Color( tTabW->mFontColor );
		}
	}
}

const String& UITab::Text() {
	return UIPushButton::Text();
}

void UITab::Text( const String &text ) {
	UITabWidget * tTabW = GetTabWidget();

	if ( NULL != tTabW ) {
		if ( text.size() > tTabW->mMaxTextLength ) {
			UIPushButton::Text( text.substr( 0, tTabW->mMaxTextLength ) );

			SetRealSize();

			return;
		}
	}

	UIPushButton::Text( text );

	SetRealSize();
}

void UITab::SetRealSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		Uint32 w = mTextBox->GetTextWidth() + GetSkinSize().width();

		UITabWidget * tTabW = GetTabWidget();

		if ( NULL != tTabW ) {
			w = eemax( w, tTabW->mMinTabWidth );
			w = eemin( w, tTabW->mMaxTabWidth );
		}

		Size( w, mSize.height() );
	}
}

UIControl * UITab::CtrlOwned() const {
	return mCtrlOwned;
}

void UITab::Update() {
	UISelectButton::Update();

	if ( mEnabled && mVisible ) {
		if ( IsMouseOver() ) {
			UITabWidget * tTabW	= GetTabWidget();

			if ( NULL != tTabW ) {
				Uint32 Flags 			= UIManager::instance()->GetInput()->ClickTrigger();

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
