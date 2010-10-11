#include "cuislider.hpp"
#include "cuimanager.hpp"

namespace EE { namespace UI {

cUISlider::cUISlider( const cUISlider::CreateParams& Params ) :
	cUIControlAnim( Params ),
	mVertical( Params.VerticalSlider ),
	mBackSlider( NULL ),
	mSlider( NULL ),
	mMinValue( 0.f ),
	mMaxValue( 1.f ),
	mValue( 0.f ),
	mClickStep( 0.1f ),
	mOnPosChange( false )
{
	mType |= UI_TYPE_GET(UI_TYPE_SLIDER);

	cUIControl::CreateParams BgParams;
	BgParams.Parent( this );

	if ( !mVertical )
		BgParams.Size = eeSize( mSize.Width() - 16, 8 );
	else
		BgParams.Size = eeSize( 8, mSize.Width() - 16 );

	mBackSlider = eeNew( cUIControl, ( BgParams ) );
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
}

cUISlider::~cUISlider() {
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

	cShape * tShape = NULL;
	cUISkin * tSkin = NULL;

	tSkin = mSlider->GetSkin();

	if ( NULL != tSkin ) {
		tShape = tSkin->GetShape( cUISkin::StateNormal );

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
		tShape = tSkin->GetShape( cUISkin::StateNormal );

		if ( NULL != tShape ) {
			if ( !mVertical )
				mBackSlider->Size( eeSize( mSize.Width() - mSlider->Size().Width(), tShape->RealSize().Height() ) );
			else
				mBackSlider->Size( eeSize( tShape->RealSize().Width(), mSize.Height() - mSlider->Size().Height() ) );

			mBackSlider->Center();
		}
	}

	Value( mValue );
}

void cUISlider::FixSliderPos() {
	if ( !mOnPosChange ) {
		mOnPosChange = true;

		if ( !mVertical ) {
			mSlider->Pos( mSlider->Pos().x, 0 );

			if ( mSlider->Pos().x < 0 )
				mSlider->Pos( 0, 0 );

			if ( mSlider->Pos().x > mBackSlider->Size().Width() )
				mSlider->Pos( mBackSlider->Size().Width(), 0 );

			mSlider->CenterVertical();

			Value( mMinValue + (eeFloat)mSlider->Pos().x * ( mMaxValue - mMinValue ) / (eeFloat)mBackSlider->Size().Width() );
		} else {
			mSlider->Pos( 0, mSlider->Pos().y );

			if ( mSlider->Pos().y < 0 )
				mSlider->Pos( 0, 0 );

			if ( mSlider->Pos().y > mBackSlider->Size().Height() )
				mSlider->Pos( 0, mBackSlider->Size().Height() );

			mSlider->CenterHorizontal();

			Value( mMinValue + (eeFloat)mSlider->Pos().y * ( mMaxValue - mMinValue ) / (eeFloat)mBackSlider->Size().Height() );
		}

		mOnPosChange = false;
	}
}

void cUISlider::Value( const eeFloat& Val ) {
	if ( Val >= mMinValue && Val <= mMaxValue ) {
		mValue = Val;

		if ( !mOnPosChange ) {
			if ( !mVertical )
				mSlider->Pos( mBackSlider->Size().Width() * ( Val - mMinValue ), mSlider->Pos().y );
			else
				mSlider->Pos( mSlider->Pos().x, mBackSlider->Size().Height() * ( Val - mMinValue ) );
		}
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

void cUISlider::ManageClick( const Uint32& Flags ) {
	if ( Flags ) {
		eeVector2i ControlPos = cUIManager::instance()->GetMousePos();
		mSlider->ScreenToControl( ControlPos );
	
		if ( Flags & EE_BUTTON_LMASK && !mSlider->IsMouseOver()  ) {
			if ( !mVertical ) {
				if ( ControlPos.x < 0 )
					Value( Value() - ClickStep() );
				else
					Value( Value() + ClickStep() );
			} else {
				if ( ControlPos.y < 0 )
					Value( Value() - ClickStep() );
				else
					Value( Value() + ClickStep() );
			}
		} else if ( Flags & EE_BUTTONS_WUWD ) {
			if ( Flags & EE_BUTTON( EE_BUTTON_WHEELUP ) )
				Value( Value() - ClickStep() );
			else
				Value( Value() + ClickStep() );
		}
	}
}

}}
