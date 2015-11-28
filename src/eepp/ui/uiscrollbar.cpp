#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/graphics/subtexture.hpp>

namespace EE { namespace UI {

UIScrollBar::UIScrollBar( const UIScrollBar::CreateParams& Params ) :
	UIComplexControl( Params )
{
	UIControlAnim::CreateParams CParams = Params;
	CParams.Size = Sizei( 16, 16 );
	CParams.Parent( this );

	mBtnDown	= eeNew( UIControlAnim, ( CParams ) );
	mBtnUp		= eeNew( UIControlAnim, ( CParams ) );

	mBtnDown->Visible( true );
	mBtnDown->Enabled( true );
	mBtnUp->Visible( true );
	mBtnUp->Enabled( true );

	UISlider::CreateParams SParams;
	SParams.Background = Params.Background;
	SParams.Blend = Params.Blend;
	SParams.Border = Params.Border;
	SParams.Flags = Params.Flags;
	SParams.Parent( this );
	SParams.Pos = Params.Pos;
	SParams.Size = Params.Size;
	SParams.VerticalSlider = Params.VerticalScrollBar;
	SParams.AllowHalfSliderOut = false;
	SParams.ExpandBackground = true;

	mSlider		= eeNew( UISlider, ( SParams ) );
	mSlider->Visible( true );
	mSlider->Enabled( true );

	mSlider->AddEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &UIScrollBar::OnValueChangeCb ) );

	AdjustChilds();

	ApplyDefaultTheme();
}

UIScrollBar::~UIScrollBar() {
}

Uint32 UIScrollBar::Type() const {
	return UI_TYPE_SCROLLBAR;
}

bool UIScrollBar::IsType( const Uint32& type ) const {
	return UIScrollBar::Type() == type ? true : UIComplexControl::IsType( type );
}

void UIScrollBar::SetTheme( UITheme * Theme ) {
	if ( !IsVertical() ) {
		UIControl::SetThemeControl( Theme, "hscrollbar" );
		mSlider->SetThemeControl( Theme, "hscrollbar_slider" );
		mSlider->GetBackSlider()->SetThemeControl( Theme, "hscrollbar_bg" );
		mSlider->GetSliderButton()->SetThemeControl( Theme, "hscrollbar_button" );
		mBtnUp->SetThemeControl( Theme, "hscrollbar_btnup" );
		mBtnDown->SetThemeControl( Theme, "hscrollbar_btndown" );
	} else {
		UIControl::SetThemeControl( Theme, "vscrollbar" );
		mSlider->SetThemeControl( Theme, "vscrollbar_slider" );
		mSlider->GetBackSlider()->SetThemeControl( Theme, "vscrollbar_bg" );
		mSlider->GetSliderButton()->SetThemeControl( Theme, "vscrollbar_button" );
		mBtnUp->SetThemeControl( Theme, "vscrollbar_btnup" );
		mBtnDown->SetThemeControl( Theme, "vscrollbar_btndown" );
	}

	SubTexture * tSubTexture = NULL;
	UISkin * tSkin = NULL;

	tSkin = mBtnUp->GetSkin();

	if ( NULL != tSkin ) {
		tSubTexture = tSkin->GetSubTexture( UISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			mBtnUp->Size( tSubTexture->RealSize() );
		}
	}

	tSkin = mBtnDown->GetSkin();

	if ( NULL != tSkin ) {
		tSubTexture = tSkin->GetSubTexture( UISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			mBtnDown->Size( tSubTexture->RealSize() );
		}
	}

	if ( mFlags & UI_AUTO_SIZE ) {
		tSkin = mSlider->GetBackSlider()->GetSkin();

		if ( NULL != tSkin ) {
			tSubTexture = tSkin->GetSubTexture( UISkinState::StateNormal );

			if ( NULL != tSubTexture ) {
				if ( mSlider->IsVertical() ) {
					mSlider->Size( tSubTexture->RealSize().Width() , mSize.Height() );
					Size( tSubTexture->RealSize().Width() , mSize.Height() );
					mMinControlSize.x = mSize.Width();
				} else {
					mSlider->Size( mSize.Width(), tSubTexture->RealSize().Height() );
					Size( mSize.Width(), tSubTexture->RealSize().Height() );
					mMinControlSize.y = mSize.Height();
				}
			}
		}
	}

	AdjustChilds();

	mSlider->AdjustChilds();
}

void UIScrollBar::OnSizeChange() {
	AdjustChilds();
	mSlider->AdjustChilds();
	UIComplexControl::OnSizeChange();
}

void UIScrollBar::AdjustChilds() {
	mBtnUp->Pos( 0, 0 );

	if ( !IsVertical() ) {
		mBtnDown->Pos( mSize.Width() - mBtnDown->Size().Width(), 0 );
		mSlider->Size( mSize.Width() - mBtnDown->Size().Width() - mBtnUp->Size().Width(), mSlider->Size().Height() );
		mSlider->Pos( mBtnUp->Size().Width(), 0 );

		mBtnDown->CenterVertical();
		mBtnUp->CenterVertical();
		mSlider->CenterVertical();
	} else {
		mBtnDown->Pos( 0, mSize.Height() - mBtnDown->Size().Height() );
		mSlider->Size( mSlider->Size().Width(), mSize.Height() - mBtnDown->Size().Height() - mBtnUp->Size().Height() );
		mSlider->Pos( 0, mBtnUp->Size().Height() );

		mBtnDown->CenterHorizontal();
		mBtnUp->CenterHorizontal();
		mSlider->CenterHorizontal();
	}
}

void UIScrollBar::Update() {
	UIControlAnim::Update();

	if ( mBtnUp->IsMouseOver() || mBtnDown->IsMouseOver() ) {
		ManageClick( UIManager::instance()->GetInput()->ClickTrigger() );
	}
}

void UIScrollBar::ManageClick( const Uint32& Flags ) {
	if ( Flags & EE_BUTTONS_WUWD ) {
		if ( Flags & EE_BUTTON_WUMASK )
			mSlider->Value( Value() + ClickStep() );
		else
			mSlider->Value( Value() - ClickStep() );
	}
}

Uint32 UIScrollBar::OnMessage( const UIMessage * Msg ) {
	switch ( Msg->Msg() ) {
		case UIMessage::MsgClick:
		{
			if ( Msg->Flags() & EE_BUTTON_LMASK ) {
				if ( Msg->Sender() == mBtnUp ) {
					mSlider->Value( Value() - ClickStep() );
				} else if ( Msg->Sender() == mBtnDown ) {
					mSlider->Value( Value() + ClickStep() );
				}
			}

			return 1;
		}
	}

	return 0;
}

void UIScrollBar::Value( Float Val ) {
	mSlider->Value( Val );
}

const Float& UIScrollBar::Value() const {
	return mSlider->Value();
}

void UIScrollBar::MinValue( const Float& MinVal ) {
	mSlider->MinValue( MinVal );
}

const Float& UIScrollBar::MinValue() const {
	return mSlider->MinValue();
}

void UIScrollBar::MaxValue( const Float& MaxVal ) {
	mSlider->MaxValue( MaxVal );
}

const Float& UIScrollBar::MaxValue() const {
	return mSlider->MaxValue();
}

void UIScrollBar::ClickStep( const Float& step ) {
	mSlider->ClickStep( step );
}

const Float& UIScrollBar::ClickStep() const {
	return mSlider->ClickStep();
}

const bool& UIScrollBar::IsVertical() const {
	return mSlider->IsVertical();
}

void UIScrollBar::OnValueChangeCb( const UIEvent * Event ) {
	OnValueChange();
}

UISlider * UIScrollBar::Slider() const {
	return mSlider;
}

UIControlAnim * UIScrollBar::ButtonUp() const {
	return mBtnUp;
}

UIControlAnim * UIScrollBar::ButtonDown() const {
	return mBtnDown;
}

void UIScrollBar::OnAlphaChange() {
	UIControlAnim::OnAlphaChange();
	
	mSlider->Alpha( mAlpha );
	mBtnUp->Alpha( mAlpha );
	mBtnDown->Alpha( mAlpha );
}

}}
