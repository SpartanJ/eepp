#include "cuiscrollbar.hpp"
#include "cuimanager.hpp"

namespace EE { namespace UI {

cUIScrollBar::cUIScrollBar( const cUIScrollBar::CreateParams& Params ) :
	cUIComplexControl( Params )
{
	mType |= UI_TYPE_GET( UI_TYPE_SCROLLBAR );

	cUIControlAnim::CreateParams CParams = Params;
	CParams.Size = eeSize( 16, 16 );
	CParams.Parent( this );

	mBtnDown	= eeNew( cUIControlAnim, ( CParams ) );
	mBtnUp		= eeNew( cUIControlAnim, ( CParams ) );

	mBtnDown->Visible( true );
	mBtnDown->Enabled( true );
	mBtnUp->Visible( true );
	mBtnUp->Enabled( true );

	cUISlider::CreateParams SParams;
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

	mSlider		= eeNew( cUISlider, ( SParams ) );
	mSlider->Visible( true );
	mSlider->Enabled( true );

	mSlider->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cUIScrollBar::OnValueChangeCb ) );

	AdjustChilds();

	ApplyDefaultTheme();
}

cUIScrollBar::~cUIScrollBar() {
}

void cUIScrollBar::SetTheme( cUITheme * Theme ) {
	if ( !IsVertical() ) {
		cUIControl::SetTheme( Theme, "hscrollbar" );
		mSlider->ForceThemeSkin( Theme, "hscrollbar_slider" );
		mSlider->GetBackSlider()->ForceThemeSkin( Theme, "hscrollbar_bg" );
		mSlider->GetSliderButton()->ForceThemeSkin( Theme, "hscrollbar_button" );
		mBtnUp->ForceThemeSkin( Theme, "hscrollbar_btnup" );
		mBtnDown->ForceThemeSkin( Theme, "hscrollbar_btndown" );
	} else {
		cUIControl::SetTheme( Theme, "vscrollbar" );
		mSlider->ForceThemeSkin( Theme, "vscrollbar_slider" );
		mSlider->GetBackSlider()->ForceThemeSkin( Theme, "vscrollbar_bg" );
		mSlider->GetSliderButton()->ForceThemeSkin( Theme, "vscrollbar_button" );
		mBtnUp->ForceThemeSkin( Theme, "vscrollbar_btnup" );
		mBtnDown->ForceThemeSkin( Theme, "vscrollbar_btndown" );
	}

	cShape * tShape = NULL;
	cUISkin * tSkin = NULL;

	tSkin = mBtnUp->GetSkin();

	if ( NULL != tSkin ) {
		tShape = tSkin->GetShape( cUISkinState::StateNormal );

		if ( NULL != tShape ) {
			mBtnUp->Size( tShape->RealSize() );
		}
	}

	tSkin = mBtnDown->GetSkin();

	if ( NULL != tSkin ) {
		tShape = tSkin->GetShape( cUISkinState::StateNormal );

		if ( NULL != tShape ) {
			mBtnDown->Size( tShape->RealSize() );
		}
	}

	AdjustChilds();

	mSlider->AdjustChilds();
}

void cUIScrollBar::OnSizeChange() {
	AdjustChilds();
}

void cUIScrollBar::AdjustChilds() {
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

void cUIScrollBar::Update() {
	cUIControlAnim::Update();

	if ( mBtnUp->IsMouseOver() || mBtnDown->IsMouseOver() ) {
		ManageClick( cUIManager::instance()->GetInput()->ClickTrigger() );
	}
}

void cUIScrollBar::ManageClick( const Uint32& Flags ) {
	if ( Flags & EE_BUTTONS_WUWD ) {
		if ( Flags & EE_BUTTON_WUMASK )
			mSlider->Value( Value() + ClickStep() );
		else
			mSlider->Value( Value() - ClickStep() );
	}
}

Uint32 cUIScrollBar::OnMessage( const cUIMessage * Msg ) {
	switch ( Msg->Msg() ) {
		case cUIMessage::MsgClick:
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

void cUIScrollBar::Value( eeFloat Val ) {
	mSlider->Value( Val );
}

const eeFloat& cUIScrollBar::Value() const {
	return mSlider->Value();
}

void cUIScrollBar::MinValue( const eeFloat& MinVal ) {
	mSlider->MinValue( MinVal );
}

const eeFloat& cUIScrollBar::MinValue() const {
	return mSlider->MinValue();
}

void cUIScrollBar::MaxValue( const eeFloat& MaxVal ) {
	mSlider->MaxValue( MaxVal );
}

const eeFloat& cUIScrollBar::MaxValue() const {
	return mSlider->MaxValue();
}

void cUIScrollBar::ClickStep( const eeFloat& step ) {
	mSlider->ClickStep( step );
}

const eeFloat& cUIScrollBar::ClickStep() const {
	return mSlider->ClickStep();
}

const bool& cUIScrollBar::IsVertical() const {
	return mSlider->IsVertical();
}

void cUIScrollBar::OnValueChangeCb( const cUIEvent * Event ) {
	OnValueChange();
}

cUISlider * cUIScrollBar::Slider() const {
	return mSlider;
}

cUIControlAnim * cUIScrollBar::ButtonUp() const {
	return mBtnUp;
}

cUIControlAnim * cUIScrollBar::ButtonDown() const {
	return mBtnDown;
}

void cUIScrollBar::OnAlphaChange() {
	cUIControlAnim::OnAlphaChange();
	
	mSlider->Alpha( mAlpha );
	mBtnUp->Alpha( mAlpha );
	mBtnDown->Alpha( mAlpha );
}

}}
