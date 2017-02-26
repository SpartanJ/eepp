#include <eepp/ui/uislider.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/graphics/subtexture.hpp>

namespace EE { namespace UI {

UISlider::UISlider( const UISlider::CreateParams& Params ) :
	UIComplexControl( Params ),
	mOrientation( Params.VerticalSlider ? UI_VERTICAL : UI_HORIZONTAL ),
	mAllowHalfSliderOut( Params.AllowHalfSliderOut ),
	mExpandBackground( Params.ExpandBackground ),
	mBackSlider( NULL ),
	mSlider( NULL ),
	mMinValue( 0.f ),
	mMaxValue( 1.f ),
	mValue( 0.f ),
	mClickStep( 0.1f ),
	mPageStep(0),
	mOnPosChange( false )
{
	UIControl::CreateParams BgParams;
	BgParams.setParent( this );

	if ( UI_HORIZONTAL == mOrientation )
		BgParams.Size = Sizei( mSize.getWidth() - 16, 8 );
	else
		BgParams.Size = Sizei( 8, mSize.getHeight() - 16 );

	mBackSlider = eeNew( UIControlAnim, ( BgParams ) );
	mBackSlider->setVisible( true );
	mBackSlider->setEnabled( true );
	mBackSlider->center();

	UIDragable::CreateParams SlideParams;
	SlideParams.setParent( this );
	SlideParams.Size = Sizei( 16, 16 );
	SlideParams.setPosition( Vector2i( 0, 0 ) );

	mSlider = eeNew( Private::UISliderButton, ( SlideParams ) );
	mSlider->setEnabled( true );
	mSlider->setVisible( true );
	mSlider->setDragEnabled( true );

	if ( UI_HORIZONTAL == mOrientation )
		mSlider->centerVertical();
	else
		mSlider->centerHorizontal();

	applyDefaultTheme();
}

UISlider::UISlider() :
	UIComplexControl(),
	mOrientation( UI_VERTICAL ),
	mAllowHalfSliderOut( false ),
	mExpandBackground( false ),
	mBackSlider( NULL ),
	mSlider( NULL ),
	mMinValue( 0.f ),
	mMaxValue( 1.f ),
	mValue( 0.f ),
	mClickStep( 0.1f ),
	mPageStep(0),
	mOnPosChange( false )
{
	Sizei bgSize;

	if ( UI_HORIZONTAL == mOrientation )
		bgSize = Sizei( mSize.getWidth() - 16, 8 );
	else
		bgSize = Sizei( 8, mSize.getHeight() - 16 );

	mBackSlider = eeNew( UIControlAnim, () );
	mBackSlider->setParent( this );
	mBackSlider->setVisible( true );
	mBackSlider->setEnabled( true );
	mBackSlider->setSize( bgSize );
	mBackSlider->center();

	mSlider = eeNew( Private::UISliderButton, () );
	mSlider->setParent( this );
	mSlider->setEnabled( true );
	mSlider->setVisible( true );
	mSlider->setDragEnabled( true );
	mSlider->setSize( 16, 16 );
	mSlider->setPosition( 0, 0 );

	if ( UI_HORIZONTAL == mOrientation )
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
	if ( UI_HORIZONTAL == mOrientation ) {
		UIControl::setThemeControl( Theme, "hslider" );

		mBackSlider->setThemeControl( Theme, "hslider_bg" );
		mSlider->setThemeControl( Theme, "hslider_button" );
	} else {
		UIControl::setThemeControl( Theme, "vslider" );

		mBackSlider->setThemeControl( Theme, "vslider_bg" );
		mSlider->setThemeControl( Theme, "vslider_button" );
	}

	adjustChilds();

	setValue( mValue );
}

void UISlider::onSizeChange() {
	UIComplexControl::onSizeChange();
	adjustChilds();
}

void UISlider::adjustChilds() {
	UISkin * tSkin = NULL;

	tSkin = mSlider->getSkin();

	if ( NULL != tSkin ) {
		if ( mPageStep == 0 ) {
			mSlider->setSize( tSkin->getSize() );
		} else {
			Float percent = ( mPageStep / ( mMaxValue - mMinValue ) );

			if ( UI_HORIZONTAL == mOrientation ) {
				Int32 size = eemax( (Int32)( (Float)mSize.getWidth() * percent ), tSkin->getSize().getWidth() );

				mSlider->setSize( size, tSkin->getSize().getHeight() );
			} else {
				Int32 size = eemax( (Int32)( (Float)mSize.getHeight() * percent ), tSkin->getSize().getHeight() );

				mSlider->setSize( tSkin->getSize().getWidth(), size );
			}
		}

		if ( UI_HORIZONTAL == mOrientation ) {
			mSlider->centerVertical();
		} else {
			mSlider->centerHorizontal();
		}
	}

	tSkin = mBackSlider->getSkin();

	if ( NULL != tSkin ) {
		if ( UI_HORIZONTAL == mOrientation ) {
			Int32 Height;

			if ( mExpandBackground )
				Height = mSize.getHeight();
			else
				Height = tSkin->getSize().getHeight();

			if ( mAllowHalfSliderOut )
				mBackSlider->setSize( Sizei( mSize.getWidth() - mSlider->getSize().getWidth(), Height ) );
			else
				mBackSlider->setSize( Sizei( mSize.getWidth(), Height ) );
		} else {
			Int32 Width;

			if ( mExpandBackground )
				Width = mSize.getWidth();
			else
				Width = tSkin->getSize().getWidth();

			if ( mAllowHalfSliderOut )
				mBackSlider->setSize( Sizei( Width, mSize.getHeight() - mSlider->getSize().getHeight() ) );
			else
				mBackSlider->setSize( Sizei( Width, mSize.getHeight() ) );
		}

		mBackSlider->center();

		fixSliderPos();
	}
}

void UISlider::fixSliderPos() {
	if ( !mOnPosChange ) {
		mOnPosChange = true;

		if ( UI_HORIZONTAL == mOrientation ) {
			mSlider->setPosition( mSlider->getPosition().x, 0 );

			if ( mSlider->getPosition().x < 0 )
				mSlider->setPosition( 0, 0 );

			if ( mAllowHalfSliderOut ) {
				if ( mSlider->getPosition().x > mBackSlider->getSize().getWidth() )
					mSlider->setPosition( mBackSlider->getSize().getWidth(), 0 );
			} else {
				if ( mSlider->getPosition().x > mBackSlider->getSize().getWidth() - mSlider->getSize().getWidth() )
					mSlider->setPosition( mBackSlider->getSize().getWidth() - mSlider->getSize().getWidth(), 0 );
			}

			mSlider->centerVertical();

			if ( mAllowHalfSliderOut )
				setValue( mMinValue + (Float)mSlider->getPosition().x * ( mMaxValue - mMinValue ) / (Float)mBackSlider->getSize().getWidth() );
			else
				setValue( mMinValue + (Float)mSlider->getPosition().x * ( mMaxValue - mMinValue ) / ( (Float)mSize.getWidth() - mSlider->getSize().getWidth() ) );
		} else {
			mSlider->setPosition( 0, mSlider->getPosition().y );

			if ( mSlider->getPosition().y < 0 )
				mSlider->setPosition( 0, 0 );

			if ( mAllowHalfSliderOut ) {
				if ( mSlider->getPosition().y > mBackSlider->getSize().getHeight() )
					mSlider->setPosition( 0, mBackSlider->getSize().getHeight() );
			} else {
				if ( mSlider->getPosition().y > mBackSlider->getSize().getHeight() - mSlider->getSize().getHeight() ) {
					mSlider->setPosition( 0, mBackSlider->getSize().getHeight() - mSlider->getSize().getHeight() );
				}
			}

			mSlider->centerHorizontal();

			if ( mAllowHalfSliderOut )
				setValue( mMinValue + (Float)mSlider->getPosition().y * ( mMaxValue - mMinValue ) / (Float)mBackSlider->getSize().getHeight() );
			else
				setValue( mMinValue + (Float)mSlider->getPosition().y * ( mMaxValue - mMinValue ) / ( (Float)mSize.getHeight() - mSlider->getSize().getHeight() ) );
		}

		mOnPosChange = false;
	}
}

void UISlider::setValue( Float Val ) {
	if ( Val < mMinValue ) Val = mMinValue;
	if ( Val > mMaxValue ) Val = mMaxValue;

	if ( Val >= mMinValue && Val <= mMaxValue ) {
		mValue = Val;

		if ( !mOnPosChange ) {
			Float Percent = ( Val - mMinValue ) / ( mMaxValue - mMinValue );

			mOnPosChange = true;

			if ( UI_HORIZONTAL == mOrientation ) {
				if ( mAllowHalfSliderOut )
					mSlider->setPosition( (Int32)( (Float)mBackSlider->getSize().getWidth() * Percent ), mSlider->getPosition().y );
				else
					mSlider->setPosition( (Int32)( ( (Float)mSize.getWidth() - mSlider->getSize().getWidth() ) * Percent ), mSlider->getPosition().y );
			} else {
				if ( mAllowHalfSliderOut )
					mSlider->setPosition( mSlider->getPosition().x, (Int32)( (Float)mBackSlider->getSize().getHeight() * Percent ) );
				else
					mSlider->setPosition( mSlider->getPosition().x, (Int32)( ( (Float)mSize.getHeight() - mSlider->getSize().getHeight() ) * Percent ) );
			}

			mOnPosChange = false;
		}

		onValueChange();
	}
}

const Float& UISlider::getValue() const {
	return mValue;
}

void UISlider::setMinValue( const Float& MinVal ) {
	mMinValue = MinVal;

	if ( mValue < mMinValue )
		mValue = mMinValue;

	fixSliderPos();
}

const Float& UISlider::getMinValue() const {
	return mMinValue;
}

void UISlider::setMaxValue( const Float& MaxVal ) {
	mMaxValue = MaxVal;

	if ( mValue > mMaxValue )
		mValue = mMaxValue;

	fixSliderPos();
}

const Float& UISlider::getMaxValue() const {
	return mMaxValue;
}

void UISlider::setClickStep( const Float& step ) {
	mClickStep = step;
}

const Float& UISlider::getClickStep() const {
	return mClickStep;
}

bool UISlider::isVertical() const {
	return mOrientation == UI_VERTICAL;
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

			setValue( mValue + mClickStep );
		} else if ( Event.getKeyCode() == KEY_UP ) {
			mLastTickMove = Sys::getTicks();

			setValue( mValue - mClickStep );
		} else if ( Event.getKeyCode() == KEY_PAGEUP ) {
			mLastTickMove = Sys::getTicks();

			setValue( mMinValue );
		} else if ( Event.getKeyCode() == KEY_PAGEDOWN ) {
			mLastTickMove = Sys::getTicks();

			setValue( mMaxValue );
		}
	}

	return UIComplexControl::onKeyDown( Event );
}

void UISlider::manageClick( const Uint32& Flags ) {
	if ( Flags ) {
		Vector2i ControlPos = UIManager::instance()->getMousePos();
		mSlider->worldToControl( ControlPos );

		if ( Flags & EE_BUTTON_LMASK && !mSlider->isMouseOver()  ) {
			if ( UI_HORIZONTAL == mOrientation ) {
				if ( ControlPos.x < 0 )
					setValue( mValue - mClickStep );
				else
					setValue( mValue + mClickStep );
			} else {
				if ( ControlPos.y < 0 )
					setValue( mValue - mClickStep );
				else
					setValue( mValue + mClickStep );
			}
		} else if ( Flags & EE_BUTTONS_WUWD ) {
			if ( Flags & EE_BUTTON_WUMASK )
				setValue( mValue - mClickStep );
			else
				setValue( mValue + mClickStep );
		}
	}
}

UI_ORIENTATION UISlider::getOrientation() const {
	return mOrientation;
}

void UISlider::setOrientation( const UI_ORIENTATION & orientation ) {
	mOrientation = orientation;

	applyDefaultTheme();
}

bool UISlider::getAllowHalfSliderOut() const {
	return mAllowHalfSliderOut;
}

void UISlider::setAllowHalfSliderOut( bool allowHalfSliderOut ) {
	mAllowHalfSliderOut = allowHalfSliderOut;

	adjustChilds();

	setValue( mValue );
}

bool UISlider::getExpandBackground() const {
	return mExpandBackground;
}

void UISlider::setExpandBackground( bool expandBackground ) {
	mExpandBackground = expandBackground;

	adjustChilds();

	setValue( mValue );
}

Float UISlider::getPageStep() const {
	return mPageStep;
}

void UISlider::setPageStep(const Float & pageStep) {
	mPageStep = eemin( eemax( pageStep, mMinValue ), mMaxValue );

	adjustChilds();

	setValue( mValue );
}

UIControl * UISlider::getBackSlider() const {
	return mBackSlider;
}

UIDragable * UISlider::getSliderButton() const {
	return mSlider;
}

void UISlider::onAlphaChange() {
	UIControlAnim::onAlphaChange();
	
	mBackSlider->setAlpha( mAlpha );
	mSlider->setAlpha( mAlpha );
}

}}
