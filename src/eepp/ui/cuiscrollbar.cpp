#include <eepp/ui/cuiscrollbar.hpp>
#include <eepp/ui/cuimanager.hpp>
#include <eepp/graphics/subtexture.hpp>

namespace EE { namespace UI {

cUIScrollBar::cUIScrollBar( const cUIScrollBar::CreateParams& Params ) :
	cUIComplexControl( Params )
{
	cUIControlAnim::CreateParams CParams = Params;
	CParams.Size = Sizei( 16, 16 );
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

Uint32 cUIScrollBar::Type() const {
	return UI_TYPE_SCROLLBAR;
}

bool cUIScrollBar::IsType( const Uint32& type ) const {
	return cUIScrollBar::Type() == type ? true : cUIComplexControl::IsType( type );
}

void cUIScrollBar::SetTheme( cUITheme * Theme ) {
	if ( !IsVertical() ) {
		cUIControl::SetThemeControl( Theme, "hscrollbar" );
		mSlider->SetThemeControl( Theme, "hscrollbar_slider" );
		mSlider->GetBackSlider()->SetThemeControl( Theme, "hscrollbar_bg" );
		mSlider->GetSliderButton()->SetThemeControl( Theme, "hscrollbar_button" );
		mBtnUp->SetThemeControl( Theme, "hscrollbar_btnup" );
		mBtnDown->SetThemeControl( Theme, "hscrollbar_btndown" );
	} else {
		cUIControl::SetThemeControl( Theme, "vscrollbar" );
		mSlider->SetThemeControl( Theme, "vscrollbar_slider" );
		mSlider->GetBackSlider()->SetThemeControl( Theme, "vscrollbar_bg" );
		mSlider->GetSliderButton()->SetThemeControl( Theme, "vscrollbar_button" );
		mBtnUp->SetThemeControl( Theme, "vscrollbar_btnup" );
		mBtnDown->SetThemeControl( Theme, "vscrollbar_btndown" );
	}

	SubTexture * tSubTexture = NULL;
	cUISkin * tSkin = NULL;

	tSkin = mBtnUp->GetSkin();

	if ( NULL != tSkin ) {
		tSubTexture = tSkin->GetSubTexture( cUISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			mBtnUp->Size( tSubTexture->RealSize() );
		}
	}

	tSkin = mBtnDown->GetSkin();

	if ( NULL != tSkin ) {
		tSubTexture = tSkin->GetSubTexture( cUISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			mBtnDown->Size( tSubTexture->RealSize() );
		}
	}

	if ( mFlags & UI_AUTO_SIZE ) {
		tSkin = mSlider->GetBackSlider()->GetSkin();

		if ( NULL != tSkin ) {
			tSubTexture = tSkin->GetSubTexture( cUISkinState::StateNormal );

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

void cUIScrollBar::OnSizeChange() {
	AdjustChilds();
	mSlider->AdjustChilds();
	cUIComplexControl::OnSizeChange();
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

void cUIScrollBar::Value( Float Val ) {
	mSlider->Value( Val );
}

const Float& cUIScrollBar::Value() const {
	return mSlider->Value();
}

void cUIScrollBar::MinValue( const Float& MinVal ) {
	mSlider->MinValue( MinVal );
}

const Float& cUIScrollBar::MinValue() const {
	return mSlider->MinValue();
}

void cUIScrollBar::MaxValue( const Float& MaxVal ) {
	mSlider->MaxValue( MaxVal );
}

const Float& cUIScrollBar::MaxValue() const {
	return mSlider->MaxValue();
}

void cUIScrollBar::ClickStep( const Float& step ) {
	mSlider->ClickStep( step );
}

const Float& cUIScrollBar::ClickStep() const {
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
