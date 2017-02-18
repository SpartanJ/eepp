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
	BgParams.setParent( this );

	if ( !mVertical )
		BgParams.Size = Sizei( mSize.getWidth() - 16, 8 );
	else
		BgParams.Size = Sizei( 8, mSize.getWidth() - 16 );

	mBackSlider = eeNew( UIControlAnim, ( BgParams ) );
	mBackSlider->visible( true );
	mBackSlider->enabled( true );
	mBackSlider->center();

	UIDragable::CreateParams SlideParams;
	SlideParams.setParent( this );
	SlideParams.Size = Sizei( 16, 16 );
	SlideParams.setPos( Vector2i( 0, 0 ) );

	mSlider = eeNew( Private::UISliderButton, ( SlideParams ) );
	mSlider->enabled( true );
	mSlider->visible( true );
	mSlider->dragEnable( true );

	if ( !mVertical )
		mSlider->centerVertical();
	else
		mSlider->centerHorizontal();

	applyDefaultTheme();
}

UISlider::~UISlider() {
}

Uint32 UISlider::getType() const {
	return UI_TYPE_SLIDER;
}

bool UISlider::isType( const Uint32& type ) const {
	return UISlider::getType() == type ? true : UIComplexControl::isType( type );
}

void UISlider::setTheme( UITheme * Theme ) {
	if ( !mVertical ) {
		UIControl::setThemeControl( Theme, "hslider" );

		mBackSlider->setThemeControl( Theme, "hslider_bg" );
		mSlider->setThemeControl( Theme, "hslider_button" );
	} else {
		UIControl::setThemeControl( Theme, "vslider" );

		mBackSlider->setThemeControl( Theme, "vslider_bg" );
		mSlider->setThemeControl( Theme, "vslider_button" );
	}

	adjustChilds();

	value( mValue );
}

void UISlider::onSizeChange() {
	UIComplexControl::onSizeChange();
	adjustChilds();
}

void UISlider::adjustChilds() {
	SubTexture * tSubTexture = NULL;
	UISkin * tSkin = NULL;

	tSkin = mSlider->getSkin();

	if ( NULL != tSkin ) {
		tSubTexture = tSkin->getSubTexture( UISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			mSlider->size( tSubTexture->getRealSize() );

			if ( !mVertical )
				mSlider->centerVertical();
			else
				mSlider->centerHorizontal();
		}
	}

	tSkin = mBackSlider->getSkin();

	if ( NULL != tSkin ) {
		tSubTexture = tSkin->getSubTexture( UISkinState::StateNormal );

		if ( NULL != tSubTexture ) {
			if ( !mVertical ) {
				Int32 Height;

				if ( mExpandBackground )
					Height = mSize.getHeight();
				else
					Height = tSubTexture->getRealSize().getHeight();

				if ( mAllowHalfSliderOut )
					mBackSlider->size( Sizei( mSize.getWidth() - mSlider->size().getWidth(), Height ) );
				else
					mBackSlider->size( Sizei( mSize.getWidth(), Height ) );
			} else {
				Int32 Width;

				if ( mExpandBackground )
					Width = mSize.getWidth();
				else
					Width = tSubTexture->getRealSize().getWidth();

				if ( mAllowHalfSliderOut )
					mBackSlider->size( Sizei( Width, mSize.getHeight() - mSlider->size().getHeight() ) );
				else
					mBackSlider->size( Sizei( Width, mSize.getHeight() ) );
			}

			mBackSlider->center();
		}
	}
}

void UISlider::fixSliderPos() {
	if ( !mOnPosChange ) {
		mOnPosChange = true;

		if ( !mVertical ) {
			mSlider->position( mSlider->position().x, 0 );

			if ( mSlider->position().x < 0 )
				mSlider->position( 0, 0 );

			if ( mAllowHalfSliderOut ) {
				if ( mSlider->position().x > mBackSlider->size().getWidth() )
					mSlider->position( mBackSlider->size().getWidth(), 0 );
			} else {
				if ( mSlider->position().x > mBackSlider->size().getWidth() - mSlider->size().getWidth() )
					mSlider->position( mBackSlider->size().getWidth() - mSlider->size().getWidth(), 0 );
			}

			mSlider->centerVertical();

			if ( mAllowHalfSliderOut )
				value( mMinValue + (Float)mSlider->position().x * ( mMaxValue - mMinValue ) / (Float)mBackSlider->size().getWidth() );
			else
				value( mMinValue + (Float)mSlider->position().x * ( mMaxValue - mMinValue ) / ( (Float)mSize.getWidth() - mSlider->size().getWidth() ) );
		} else {
			mSlider->position( 0, mSlider->position().y );

			if ( mSlider->position().y < 0 )
				mSlider->position( 0, 0 );

			if ( mAllowHalfSliderOut ) {
				if ( mSlider->position().y > mBackSlider->size().getHeight() )
					mSlider->position( 0, mBackSlider->size().getHeight() );
			} else {
				if ( mSlider->position().y > mBackSlider->size().getHeight() - mSlider->size().getHeight() )
					mSlider->position( 0, mBackSlider->size().getHeight() - mSlider->size().getHeight() );
			}

			mSlider->centerHorizontal();

			if ( mAllowHalfSliderOut )
				value( mMinValue + (Float)mSlider->position().y * ( mMaxValue - mMinValue ) / (Float)mBackSlider->size().getHeight() );
			else
				value( mMinValue + (Float)mSlider->position().y * ( mMaxValue - mMinValue ) / ( (Float)mSize.getHeight() - mSlider->size().getHeight() ) );
		}

		mOnPosChange = false;
	}
}

void UISlider::value( Float Val ) {
	if ( Val < mMinValue ) Val = mMinValue;
	if ( Val > mMaxValue ) Val = mMaxValue;

	if ( Val >= mMinValue && Val <= mMaxValue ) {
		mValue = Val;

		if ( !mOnPosChange ) {
			Float Percent = ( Val - mMinValue ) / ( mMaxValue - mMinValue );

			mOnPosChange = true;

			if ( !mVertical ) {
				if ( mAllowHalfSliderOut )
					mSlider->position( (Int32)( (Float)mBackSlider->size().getWidth() * Percent ), mSlider->position().y );
				else
					mSlider->position( (Int32)( ( (Float)mSize.getWidth() - mSlider->size().getWidth() ) * Percent ), mSlider->position().y );
			} else {
				if ( mAllowHalfSliderOut )
					mSlider->position( mSlider->position().x, (Int32)( (Float)mBackSlider->size().getHeight() * Percent ) );
				else
					mSlider->position( mSlider->position().x, (Int32)( ( (Float)mSize.getHeight() - mSlider->size().getHeight() ) * Percent ) );
			}

			mOnPosChange = false;
		}

		onValueChange();
	}
}

const Float& UISlider::value() const {
	return mValue;
}

void UISlider::minValue( const Float& MinVal ) {
	mMinValue = MinVal;

	if ( mValue < mMinValue )
		mValue = mMinValue;

	fixSliderPos();
}

const Float& UISlider::minValue() const {
	return mMinValue;
}

void UISlider::maxValue( const Float& MaxVal ) {
	mMaxValue = MaxVal;

	if ( mValue > mMaxValue )
		mValue = mMaxValue;

	fixSliderPos();
}

const Float& UISlider::maxValue() const {
	return mMaxValue;
}

void UISlider::clickStep( const Float& step ) {
	mClickStep = step;
}

const Float& UISlider::clickStep() const {
	return mClickStep;
}

const bool& UISlider::isVertical() const {
	return mVertical;
}

void UISlider::update() {
	UIControlAnim::update();

	if ( isMouseOver() || mBackSlider->isMouseOver() || mSlider->isMouseOver() ) {
		manageClick( UIManager::instance()->getInput()->getClickTrigger() );
	}
}

Uint32 UISlider::onKeyDown( const UIEventKey &Event ) {
	if ( Sys::getTicks() - mLastTickMove > 100 ) {
		if ( Event.getKeyCode() == KEY_DOWN ) {
			mLastTickMove = Sys::getTicks();

			value( mValue + mClickStep );
		} else if ( Event.getKeyCode() == KEY_UP ) {
			mLastTickMove = Sys::getTicks();

			value( mValue - mClickStep );
		} else if ( Event.getKeyCode() == KEY_PAGEUP ) {
			mLastTickMove = Sys::getTicks();

			value( mMinValue );
		} else if ( Event.getKeyCode() == KEY_PAGEDOWN ) {
			mLastTickMove = Sys::getTicks();

			value( mMaxValue );
		}
	}

	return UIComplexControl::onKeyDown( Event );
}

void UISlider::manageClick( const Uint32& Flags ) {
	if ( Flags ) {
		Vector2i ControlPos = UIManager::instance()->getMousePos();
		mSlider->worldToControl( ControlPos );

		if ( Flags & EE_BUTTON_LMASK && !mSlider->isMouseOver()  ) {
			if ( !mVertical ) {
				if ( ControlPos.x < 0 )
					value( mValue - mClickStep );
				else
					value( mValue + mClickStep );
			} else {
				if ( ControlPos.y < 0 )
					value( mValue - mClickStep );
				else
					value( mValue + mClickStep );
			}
		} else if ( Flags & EE_BUTTONS_WUWD ) {
			if ( Flags & EE_BUTTON_WUMASK )
				value( mValue - mClickStep );
			else
				value( mValue + mClickStep );
		}
	}
}

UIControl * UISlider::getBackSlider() const {
	return mBackSlider;
}

UIDragable * UISlider::getSliderButton() const {
	return mSlider;
}

const bool& UISlider::allowHalfSliderOut() const {
	return mAllowHalfSliderOut;
}

const bool& UISlider::expandBackground() const {
	return mExpandBackground;
}

void UISlider::onAlphaChange() {
	UIControlAnim::onAlphaChange();
	
	mBackSlider->alpha( mAlpha );
	mSlider->alpha( mAlpha );
}

}}
