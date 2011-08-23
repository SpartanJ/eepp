#include "cuislider.hpp"
#include "cuimanager.hpp"

namespace EE { namespace UI {

cUISlider::cUISlider( const cUISlider::CreateParams& Params ) :
	cUIComplexControl( Params ),
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
	cUIControl::CreateParams BgParams;
	BgParams.Parent( this );

	if ( !mVertical )
		BgParams.Size = eeSize( mSize.Width() - 16, 8 );
	else
		BgParams.Size = eeSize( 8, mSize.Width() - 16 );

	mBackSlider = eeNew( cUIControlAnim, ( BgParams ) );
	mBackSlider->Visible( true );
	mBackSlider->Enabled( true );
	mBackSlider->Center();

	cUIDragable::CreateParams SlideParams;
	SlideParams.Parent( this );
	SlideParams.Size = eeSize( 16, 16 );
	SlideParams.PosSet( eeVector2i( 0, 0 ) );

	mSlider = eeNew( Private::cUISliderButton, ( SlideParams ) );
	mSlider->Enabled( true );
	mSlider->Visible( true );
	mSlider->DragEnable( true );

	if ( !mVertical )
		mSlider->CenterVertical();
	else
		mSlider->CenterHorizontal();

	ApplyDefaultTheme();
}

cUISlider::~cUISlider() {
}

Uint32 cUISlider::Type() const {
	return UI_TYPE_SLIDER;
}

bool cUISlider::IsType( const Uint32& type ) const {
	return cUISlider::Type() == type ? true : cUIComplexControl::IsType( type );
}

void cUISlider::SetTheme( cUITheme * Theme ) {
	if ( !mVertical ) {
		cUIControl::SetTheme( Theme, "hslider" );

		mBackSlider->ForceThemeSkin( Theme, "hslider_bg" );
		mSlider->ForceThemeSkin( Theme, "hslider_button" );
	} else {
		cUIControl::SetTheme( Theme, "vslider" );

		mBackSlider->ForceThemeSkin( Theme, "vslider_bg" );
		mSlider->ForceThemeSkin( Theme, "vslider_button" );
	}

	AdjustChilds();

	Value( mValue );
}

void cUISlider::OnSizeChange() {
	cUIComplexControl::OnSizeChange();
	AdjustChilds();
}

void cUISlider::AdjustChilds() {
	cShape * tShape = NULL;
	cUISkin * tSkin = NULL;

	tSkin = mSlider->GetSkin();

	if ( NULL != tSkin ) {
		tShape = tSkin->GetShape( cUISkinState::StateNormal );

		if ( NULL != tShape ) {
			mSlider->Size( tShape->RealSize() );

			if ( !mVertical )
				mSlider->CenterVertical();
			else
				mSlider->CenterHorizontal();
		}
	}

	tSkin = mBackSlider->GetSkin();

	if ( NULL != tSkin ) {
		tShape = tSkin->GetShape( cUISkinState::StateNormal );

		if ( NULL != tShape ) {
			if ( !mVertical ) {
				Int32 Height;

				if ( mExpandBackground )
					Height = mSize.Height();
				else
					Height = tShape->RealSize().Height();

				if ( mAllowHalfSliderOut )
					mBackSlider->Size( eeSize( mSize.Width() - mSlider->Size().Width(), Height ) );
				else
					mBackSlider->Size( eeSize( mSize.Width(), Height ) );
			} else {
				Int32 Width;

				if ( mExpandBackground )
					Width = mSize.Width();
				else
					Width = tShape->RealSize().Width();

				if ( mAllowHalfSliderOut )
					mBackSlider->Size( eeSize( Width, mSize.Height() - mSlider->Size().Height() ) );
				else
					mBackSlider->Size( eeSize( Width, mSize.Height() ) );
			}

			mBackSlider->Center();
		}
	}
}

void cUISlider::FixSliderPos() {
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
				Value( mMinValue + (eeFloat)mSlider->Pos().x * ( mMaxValue - mMinValue ) / (eeFloat)mBackSlider->Size().Width() );
			else
				Value( mMinValue + (eeFloat)mSlider->Pos().x * ( mMaxValue - mMinValue ) / ( (eeFloat)mSize.Width() - mSlider->Size().Width() ) );
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
				Value( mMinValue + (eeFloat)mSlider->Pos().y * ( mMaxValue - mMinValue ) / (eeFloat)mBackSlider->Size().Height() );
			else
				Value( mMinValue + (eeFloat)mSlider->Pos().y * ( mMaxValue - mMinValue ) / ( (eeFloat)mSize.Height() - mSlider->Size().Height() ) );
		}

		mOnPosChange = false;
	}
}

void cUISlider::Value( eeFloat Val ) {
	if ( Val < mMinValue ) Val = mMinValue;
	if ( Val > mMaxValue ) Val = mMaxValue;

	if ( Val >= mMinValue && Val <= mMaxValue ) {
		mValue = Val;

		if ( !mOnPosChange ) {
			eeFloat Percent = ( Val - mMinValue ) / ( mMaxValue - mMinValue );

			mOnPosChange = true;

			if ( !mVertical ) {
				if ( mAllowHalfSliderOut )
					mSlider->Pos( (Int32)( (eeFloat)mBackSlider->Size().Width() * Percent ), mSlider->Pos().y );
				else
					mSlider->Pos( (Int32)( ( (eeFloat)mSize.Width() - mSlider->Size().Width() ) * Percent ), mSlider->Pos().y );
			} else {
				if ( mAllowHalfSliderOut )
					mSlider->Pos( mSlider->Pos().x, (Int32)( (eeFloat)mBackSlider->Size().Height() * Percent ) );
				else
					mSlider->Pos( mSlider->Pos().x, (Int32)( ( (eeFloat)mSize.Height() - mSlider->Size().Height() ) * Percent ) );
			}

			mOnPosChange = false;
		}

		OnValueChange();
	}
}

const eeFloat& cUISlider::Value() const {
	return mValue;
}

void cUISlider::MinValue( const eeFloat& MinVal ) {
	mMinValue = MinVal;

	if ( mValue < mMinValue )
		mValue = mMinValue;

	FixSliderPos();
}

const eeFloat& cUISlider::MinValue() const {
	return mMinValue;
}

void cUISlider::MaxValue( const eeFloat& MaxVal ) {
	mMaxValue = MaxVal;

	if ( mValue > mMaxValue )
		mValue = mMaxValue;

	FixSliderPos();
}

const eeFloat& cUISlider::MaxValue() const {
	return mMaxValue;
}

void cUISlider::ClickStep( const eeFloat& step ) {
	mClickStep = step;
}

const eeFloat& cUISlider::ClickStep() const {
	return mClickStep;
}

const bool& cUISlider::IsVertical() const {
	return mVertical;
}

void cUISlider::Update() {
	cUIControlAnim::Update();

	if ( IsMouseOver() || mBackSlider->IsMouseOver() || mSlider->IsMouseOver() ) {
		ManageClick( cUIManager::instance()->GetInput()->ClickTrigger() );
	}
}

Uint32 cUISlider::OnKeyDown( const cUIEventKey &Event ) {
	if ( eeGetTicks() - mLastTickMove > 100 ) {
		if ( Event.KeyCode() == KEY_DOWN ) {
			mLastTickMove = eeGetTicks();

			Value( mValue + mClickStep );
		} else if ( Event.KeyCode() == KEY_UP ) {
			mLastTickMove = eeGetTicks();

			Value( mValue - mClickStep );
		} else if ( Event.KeyCode() == KEY_PAGEUP ) {
			mLastTickMove = eeGetTicks();

			Value( mMinValue );
		} else if ( Event.KeyCode() == KEY_PAGEDOWN ) {
			mLastTickMove = eeGetTicks();

			Value( mMaxValue );
		}
	}

	return cUIComplexControl::OnKeyDown( Event );
}

void cUISlider::ManageClick( const Uint32& Flags ) {
	if ( Flags ) {
		eeVector2i ControlPos = cUIManager::instance()->GetMousePos();
		mSlider->ScreenToControl( ControlPos );

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

cUIControl * cUISlider::GetBackSlider() const {
	return mBackSlider;
}

cUIDragable * cUISlider::GetSliderButton() const {
	return mSlider;
}

const bool& cUISlider::AllowHalfSliderOut() const {
	return mAllowHalfSliderOut;
}

const bool& cUISlider::ExpandBackground() const {
	return mExpandBackground;
}

void cUISlider::OnAlphaChange() {
	cUIControlAnim::OnAlphaChange();
	
	mBackSlider->Alpha( mAlpha );
	mSlider->Alpha( mAlpha );
}

}}
