#include <eepp/ui/uislider.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/graphics/subtexture.hpp>

namespace EE { namespace UI {

UISlider::UISlider( const UISlider::CreateParams& Params ) :
	UIComplexControl( Params ),
	mVertical( Params.VerticalSlider ),
	mAllowHalfSliderOut( Params.AllowHalfSliderOut ),
	mExpandBackground( Params.ExpandBackground ),
	mBackSlider( NULL ),
	mSlider( NULL ),
	mMinValue( 0.f ),
	mMaxValue( 1.f ),
	mValue( 0.f ),
	mClickStep( 0.1f ),
	mOnPosChange( false )
{
	UIControl::CreateParams BgParams;
	BgParams.Parent( this );

	if ( !mVertical )
		BgParams.Size = Sizei( mSize.Width() - 16, 8 );
	else
		BgParams.Size = Sizei( 8, mSize.Width() - 16 );

	mBackSlider = eeNew( UIControlAnim, ( BgParams ) );
	mBackSlider->Visible( true );
	mBackSlider->Enabled( true );
	mBackSlider->Center();

	UIDragable::CreateParams SlideParams;
	SlideParams.Parent( this );
	SlideParams.Size = Sizei( 16, 16 );
	SlideParams.PosSet( Vector2i( 0, 0 ) );

	mSlider = eeNew( Private::UISliderButton, ( SlideParams ) );
	mSlider->Enabled( true );
	mSlider->Visible( true );
	mSlider->DragEnable( true );

	if ( !mVertical )
		mSlider->CenterVertical();
	else
		mSlider->CenterHorizontal();

	ApplyDefaultTheme();
}

UISlider::~UISlider() {
}

Uint32 UISlider::Type() const {
	return UI_TYPE_SLIDER;
}

bool UISlider::IsType( const Uint32& type ) const {
	return UISlider::Type() == type ? true : UIComplexControl::IsType( type );
}

void UISlider::SetTheme( UITheme * Theme ) {
	if ( !mVertical ) {
		UIControl::SetThemeControl( Theme, "hslider" );

		mBackSlider->SetThemeControl( Theme, "hslider_bg" );
		mSlider->SetThemeControl( Theme, "hslider_button" );
	} else {
		UIControl::SetThemeControl( Theme, "vslider" );

		mBackSlider->SetThemeControl( Theme, "vslider_bg" );
		mSlider->SetThemeControl( Theme, "vslider_button" );
	}

	AdjustChilds();

	Value( mValue );
}

void UISlider::OnSizeChange() {
	UIComplexControl::OnSizeChange();
	AdjustChilds();
}

void UISlider::AdjustChilds() {
	SubTexture * tSubTexture = NULL;
	UISkin * tSkin = NULL;

	tSkin = mSlider->GetSkin();

	if ( NULL != tSkin ) {
		tSubTexture = tSkin->GetSubTexture( UISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			mSlider->Size( tSubTexture->RealSize() );

			if ( !mVertical )
				mSlider->CenterVertical();
			else
				mSlider->CenterHorizontal();
		}
	}

	tSkin = mBackSlider->GetSkin();

	if ( NULL != tSkin ) {
		tSubTexture = tSkin->GetSubTexture( UISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			if ( !mVertical ) {
				Int32 Height;

				if ( mExpandBackground )
					Height = mSize.Height();
				else
					Height = tSubTexture->RealSize().Height();

				if ( mAllowHalfSliderOut )
					mBackSlider->Size( Sizei( mSize.Width() - mSlider->Size().Width(), Height ) );
				else
					mBackSlider->Size( Sizei( mSize.Width(), Height ) );
			} else {
				Int32 Width;

				if ( mExpandBackground )
					Width = mSize.Width();
				else
					Width = tSubTexture->RealSize().Width();

				if ( mAllowHalfSliderOut )
					mBackSlider->Size( Sizei( Width, mSize.Height() - mSlider->Size().Height() ) );
				else
					mBackSlider->Size( Sizei( Width, mSize.Height() ) );
			}

			mBackSlider->Center();
		}
	}
}

void UISlider::FixSliderPos() {
	if ( !mOnPosChange ) {
		mOnPosChange = true;

		if ( !mVertical ) {
			mSlider->Pos( mSlider->Pos().x, 0 );

			if ( mSlider->Pos().x < 0 )
				mSlider->Pos( 0, 0 );

			if ( mAllowHalfSliderOut ) {
				if ( mSlider->Pos().x > mBackSlider->Size().Width() )
					mSlider->Pos( mBackSlider->Size().Width(), 0 );
			} else {
				if ( mSlider->Pos().x > mBackSlider->Size().Width() - mSlider->Size().Width() )
					mSlider->Pos( mBackSlider->Size().Width() - mSlider->Size().Width(), 0 );
			}

			mSlider->CenterVertical();

			if ( mAllowHalfSliderOut )
				Value( mMinValue + (Float)mSlider->Pos().x * ( mMaxValue - mMinValue ) / (Float)mBackSlider->Size().Width() );
			else
				Value( mMinValue + (Float)mSlider->Pos().x * ( mMaxValue - mMinValue ) / ( (Float)mSize.Width() - mSlider->Size().Width() ) );
		} else {
			mSlider->Pos( 0, mSlider->Pos().y );

			if ( mSlider->Pos().y < 0 )
				mSlider->Pos( 0, 0 );

			if ( mAllowHalfSliderOut ) {
				if ( mSlider->Pos().y > mBackSlider->Size().Height() )
					mSlider->Pos( 0, mBackSlider->Size().Height() );
			} else {
				if ( mSlider->Pos().y > mBackSlider->Size().Height() - mSlider->Size().Height() )
					mSlider->Pos( 0, mBackSlider->Size().Height() - mSlider->Size().Height() );
			}

			mSlider->CenterHorizontal();

			if ( mAllowHalfSliderOut )
				Value( mMinValue + (Float)mSlider->Pos().y * ( mMaxValue - mMinValue ) / (Float)mBackSlider->Size().Height() );
			else
				Value( mMinValue + (Float)mSlider->Pos().y * ( mMaxValue - mMinValue ) / ( (Float)mSize.Height() - mSlider->Size().Height() ) );
		}

		mOnPosChange = false;
	}
}

void UISlider::Value( Float Val ) {
	if ( Val < mMinValue ) Val = mMinValue;
	if ( Val > mMaxValue ) Val = mMaxValue;

	if ( Val >= mMinValue && Val <= mMaxValue ) {
		mValue = Val;

		if ( !mOnPosChange ) {
			Float Percent = ( Val - mMinValue ) / ( mMaxValue - mMinValue );

			mOnPosChange = true;

			if ( !mVertical ) {
				if ( mAllowHalfSliderOut )
					mSlider->Pos( (Int32)( (Float)mBackSlider->Size().Width() * Percent ), mSlider->Pos().y );
				else
					mSlider->Pos( (Int32)( ( (Float)mSize.Width() - mSlider->Size().Width() ) * Percent ), mSlider->Pos().y );
			} else {
				if ( mAllowHalfSliderOut )
					mSlider->Pos( mSlider->Pos().x, (Int32)( (Float)mBackSlider->Size().Height() * Percent ) );
				else
					mSlider->Pos( mSlider->Pos().x, (Int32)( ( (Float)mSize.Height() - mSlider->Size().Height() ) * Percent ) );
			}

			mOnPosChange = false;
		}

		OnValueChange();
	}
}

const Float& UISlider::Value() const {
	return mValue;
}

void UISlider::MinValue( const Float& MinVal ) {
	mMinValue = MinVal;

	if ( mValue < mMinValue )
		mValue = mMinValue;

	FixSliderPos();
}

const Float& UISlider::MinValue() const {
	return mMinValue;
}

void UISlider::MaxValue( const Float& MaxVal ) {
	mMaxValue = MaxVal;

	if ( mValue > mMaxValue )
		mValue = mMaxValue;

	FixSliderPos();
}

const Float& UISlider::MaxValue() const {
	return mMaxValue;
}

void UISlider::ClickStep( const Float& step ) {
	mClickStep = step;
}

const Float& UISlider::ClickStep() const {
	return mClickStep;
}

const bool& UISlider::IsVertical() const {
	return mVertical;
}

void UISlider::Update() {
	UIControlAnim::Update();

	if ( IsMouseOver() || mBackSlider->IsMouseOver() || mSlider->IsMouseOver() ) {
		ManageClick( UIManager::instance()->GetInput()->ClickTrigger() );
	}
}

Uint32 UISlider::OnKeyDown( const UIEventKey &Event ) {
	if ( Sys::GetTicks() - mLastTickMove > 100 ) {
		if ( Event.KeyCode() == KEY_DOWN ) {
			mLastTickMove = Sys::GetTicks();

			Value( mValue + mClickStep );
		} else if ( Event.KeyCode() == KEY_UP ) {
			mLastTickMove = Sys::GetTicks();

			Value( mValue - mClickStep );
		} else if ( Event.KeyCode() == KEY_PAGEUP ) {
			mLastTickMove = Sys::GetTicks();

			Value( mMinValue );
		} else if ( Event.KeyCode() == KEY_PAGEDOWN ) {
			mLastTickMove = Sys::GetTicks();

			Value( mMaxValue );
		}
	}

	return UIComplexControl::OnKeyDown( Event );
}

void UISlider::ManageClick( const Uint32& Flags ) {
	if ( Flags ) {
		Vector2i ControlPos = UIManager::instance()->GetMousePos();
		mSlider->WorldToControl( ControlPos );

		if ( Flags & EE_BUTTON_LMASK && !mSlider->IsMouseOver()  ) {
			if ( !mVertical ) {
				if ( ControlPos.x < 0 )
					Value( mValue - mClickStep );
				else
					Value( mValue + mClickStep );
			} else {
				if ( ControlPos.y < 0 )
					Value( mValue - mClickStep );
				else
					Value( mValue + mClickStep );
			}
		} else if ( Flags & EE_BUTTONS_WUWD ) {
			if ( Flags & EE_BUTTON_WUMASK )
				Value( mValue - mClickStep );
			else
				Value( mValue + mClickStep );
		}
	}
}

UIControl * UISlider::GetBackSlider() const {
	return mBackSlider;
}

UIDragable * UISlider::GetSliderButton() const {
	return mSlider;
}

const bool& UISlider::AllowHalfSliderOut() const {
	return mAllowHalfSliderOut;
}

const bool& UISlider::ExpandBackground() const {
	return mExpandBackground;
}

void UISlider::OnAlphaChange() {
	UIControlAnim::OnAlphaChange();
	
	mBackSlider->Alpha( mAlpha );
	mSlider->Alpha( mAlpha );
}

}}
