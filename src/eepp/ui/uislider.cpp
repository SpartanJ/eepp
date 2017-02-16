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
		BgParams.Size = Sizei( mSize.width() - 16, 8 );
	else
		BgParams.Size = Sizei( 8, mSize.width() - 16 );

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
			mSlider->Size( tSubTexture->realSize() );

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
					Height = mSize.height();
				else
					Height = tSubTexture->realSize().height();

				if ( mAllowHalfSliderOut )
					mBackSlider->Size( Sizei( mSize.width() - mSlider->Size().width(), Height ) );
				else
					mBackSlider->Size( Sizei( mSize.width(), Height ) );
			} else {
				Int32 Width;

				if ( mExpandBackground )
					Width = mSize.width();
				else
					Width = tSubTexture->realSize().width();

				if ( mAllowHalfSliderOut )
					mBackSlider->Size( Sizei( Width, mSize.height() - mSlider->Size().height() ) );
				else
					mBackSlider->Size( Sizei( Width, mSize.height() ) );
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
				if ( mSlider->Pos().x > mBackSlider->Size().width() )
					mSlider->Pos( mBackSlider->Size().width(), 0 );
			} else {
				if ( mSlider->Pos().x > mBackSlider->Size().width() - mSlider->Size().width() )
					mSlider->Pos( mBackSlider->Size().width() - mSlider->Size().width(), 0 );
			}

			mSlider->CenterVertical();

			if ( mAllowHalfSliderOut )
				Value( mMinValue + (Float)mSlider->Pos().x * ( mMaxValue - mMinValue ) / (Float)mBackSlider->Size().width() );
			else
				Value( mMinValue + (Float)mSlider->Pos().x * ( mMaxValue - mMinValue ) / ( (Float)mSize.width() - mSlider->Size().width() ) );
		} else {
			mSlider->Pos( 0, mSlider->Pos().y );

			if ( mSlider->Pos().y < 0 )
				mSlider->Pos( 0, 0 );

			if ( mAllowHalfSliderOut ) {
				if ( mSlider->Pos().y > mBackSlider->Size().height() )
					mSlider->Pos( 0, mBackSlider->Size().height() );
			} else {
				if ( mSlider->Pos().y > mBackSlider->Size().height() - mSlider->Size().height() )
					mSlider->Pos( 0, mBackSlider->Size().height() - mSlider->Size().height() );
			}

			mSlider->CenterHorizontal();

			if ( mAllowHalfSliderOut )
				Value( mMinValue + (Float)mSlider->Pos().y * ( mMaxValue - mMinValue ) / (Float)mBackSlider->Size().height() );
			else
				Value( mMinValue + (Float)mSlider->Pos().y * ( mMaxValue - mMinValue ) / ( (Float)mSize.height() - mSlider->Size().height() ) );
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
					mSlider->Pos( (Int32)( (Float)mBackSlider->Size().width() * Percent ), mSlider->Pos().y );
				else
					mSlider->Pos( (Int32)( ( (Float)mSize.width() - mSlider->Size().width() ) * Percent ), mSlider->Pos().y );
			} else {
				if ( mAllowHalfSliderOut )
					mSlider->Pos( mSlider->Pos().x, (Int32)( (Float)mBackSlider->Size().height() * Percent ) );
				else
					mSlider->Pos( mSlider->Pos().x, (Int32)( ( (Float)mSize.height() - mSlider->Size().height() ) * Percent ) );
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
		ManageClick( UIManager::instance()->GetInput()->clickTrigger() );
	}
}

Uint32 UISlider::OnKeyDown( const UIEventKey &Event ) {
	if ( Sys::getTicks() - mLastTickMove > 100 ) {
		if ( Event.KeyCode() == KEY_DOWN ) {
			mLastTickMove = Sys::getTicks();

			Value( mValue + mClickStep );
		} else if ( Event.KeyCode() == KEY_UP ) {
			mLastTickMove = Sys::getTicks();

			Value( mValue - mClickStep );
		} else if ( Event.KeyCode() == KEY_PAGEUP ) {
			mLastTickMove = Sys::getTicks();

			Value( mMinValue );
		} else if ( Event.KeyCode() == KEY_PAGEDOWN ) {
			mLastTickMove = Sys::getTicks();

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
