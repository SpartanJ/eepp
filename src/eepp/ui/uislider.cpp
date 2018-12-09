#include <eepp/ui/uislider.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/graphics/textureregion.hpp>
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UISlider * UISlider::New( const UI_ORIENTATION& orientation ) {
	return eeNew( UISlider, ( orientation ) );
}

UISlider::UISlider( const UI_ORIENTATION& orientation ) :
	UIWidget(),
	mOrientation( orientation ),
	mBackSlider( NULL ),
	mSlider( NULL ),
	mMinValue( 0.f ),
	mMaxValue( 1.f ),
	mValue( 0.f ),
	mClickStep( 0.1f ),
	mPageStep( 0 ),
	mOnPosChange( false )
{
	UITheme * theme = UIThemeManager::instance()->getDefaultTheme();

	if ( NULL != theme ) {
		mStyleConfig = theme->getSliderStyleConfig();
	}

	Sizef bgSize;

	if ( UI_HORIZONTAL == mOrientation )
		bgSize = Sizef( mDpSize.getWidth() - 16, 8 );
	else
		bgSize = Sizef( 8, mDpSize.getHeight() - 16 );

	mBackSlider = UINode::New();
	mBackSlider->setParent( this );
	mBackSlider->setVisible( true );
	mBackSlider->setEnabled( true );
	mBackSlider->setSize( bgSize );
	mBackSlider->center();

	mSlider = Private::UISliderButton::New();
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
	return UISlider::getType() == type ? true : UIWidget::isType( type );
}

void UISlider::setTheme( UITheme * Theme ) {
	UIWidget::setTheme( Theme );

	if ( UI_HORIZONTAL == mOrientation ) {
		setThemeSkin( Theme, "hslider" );

		mBackSlider->setThemeSkin( Theme, "hslider_bg" );
		mSlider->setThemeSkin( Theme, "hslider_button" );
	} else {
		setThemeSkin( Theme, "vslider" );

		mBackSlider->setThemeSkin( Theme, "vslider_bg" );
		mSlider->setThemeSkin( Theme, "vslider_button" );
	}

	adjustChilds();

	setValue( mValue );
}

void UISlider::onSizeChange() {
	UIWidget::onSizeChange();
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
				Float size = eemax( ( (Float)mDpSize.getWidth() * percent ), tSkin->getSize().getWidth() );

				mSlider->setSize( size, tSkin->getSize().getHeight() );
			} else {
				Float size = eemax( ( (Float)mDpSize.getHeight() * percent ), tSkin->getSize().getHeight() );

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
			Float Height;

			if ( mStyleConfig.ExpandBackground )
				Height = mDpSize.getHeight();
			else
				Height = tSkin->getSize().getHeight();

			if ( mStyleConfig.AllowHalfSliderOut )
				mBackSlider->setSize( Sizef( mDpSize.getWidth() - mSlider->getSize().getWidth(), Height ) );
			else
				mBackSlider->setSize( Sizef( mDpSize.getWidth(), Height ) );
		} else {
			Float Width;

			if ( mStyleConfig.ExpandBackground )
				Width = mDpSize.getWidth();
			else
				Width = tSkin->getSize().getWidth();

			if ( mStyleConfig.AllowHalfSliderOut )
				mBackSlider->setSize( Sizef( Width, mDpSize.getHeight() - mSlider->getSize().getHeight() ) );
			else
				mBackSlider->setSize( Sizef( Width, mDpSize.getHeight() ) );
		}

		mBackSlider->center();
	}

	fixSliderPos();
}

void UISlider::fixSliderPos() {
	if ( !mOnPosChange ) {
		mOnPosChange = true;

		if ( UI_HORIZONTAL == mOrientation ) {
			mSlider->setPosition( mSlider->getPosition().x, 0 );

			if ( mSlider->getPosition().x < 0 )
				mSlider->setPosition( 0, 0 );

			if ( mStyleConfig.AllowHalfSliderOut ) {
				if ( mSlider->getPosition().x > mBackSlider->getSize().getWidth() )
					mSlider->setPosition( mBackSlider->getSize().getWidth(), 0 );
			} else {
				if ( mSlider->getPosition().x > mBackSlider->getSize().getWidth() - mSlider->getSize().getWidth() )
					mSlider->setPosition( mBackSlider->getSize().getWidth() - mSlider->getSize().getWidth(), 0 );
			}

			mSlider->centerVertical();

			if ( mStyleConfig.AllowHalfSliderOut )
				setValue( mMinValue + (Float)mSlider->getPosition().x * ( mMaxValue - mMinValue ) / (Float)mBackSlider->getSize().getWidth() );
			else
				setValue( mMinValue + (Float)mSlider->getPosition().x * ( mMaxValue - mMinValue ) / ( (Float)mDpSize.getWidth() - mSlider->getSize().getWidth() ) );
		} else {
			mSlider->setPosition( 0, mSlider->getPosition().y );

			if ( mSlider->getPosition().y < 0 )
				mSlider->setPosition( 0, 0 );

			if ( mStyleConfig.AllowHalfSliderOut ) {
				if ( mSlider->getPosition().y > mBackSlider->getSize().getHeight() )
					mSlider->setPosition( 0, mBackSlider->getSize().getHeight() );
			} else {
				if ( mSlider->getPosition().y > mBackSlider->getSize().getHeight() - mSlider->getSize().getHeight() ) {
					mSlider->setPosition( 0, mBackSlider->getSize().getHeight() - mSlider->getSize().getHeight() );
				}
			}

			mSlider->centerHorizontal();

			if ( mStyleConfig.AllowHalfSliderOut )
				setValue( mMinValue + (Float)mSlider->getPosition().y * ( mMaxValue - mMinValue ) / (Float)mBackSlider->getSize().getHeight() );
			else
				setValue( mMinValue + (Float)mSlider->getPosition().y * ( mMaxValue - mMinValue ) / ( (Float)mDpSize.getHeight() - mSlider->getSize().getHeight() ) );
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
				if ( mStyleConfig.AllowHalfSliderOut )
					mSlider->setPosition( (Int32)( (Float)mBackSlider->getSize().getWidth() * Percent ), mSlider->getPosition().y );
				else
					mSlider->setPosition( (Int32)( ( (Float)mDpSize.getWidth() - mSlider->getSize().getWidth() ) * Percent ), mSlider->getPosition().y );
			} else {
				if ( mStyleConfig.AllowHalfSliderOut )
					mSlider->setPosition( mSlider->getPosition().x, (Int32)( (Float)mBackSlider->getSize().getHeight() * Percent ) );
				else
					mSlider->setPosition( mSlider->getPosition().x, (Int32)( ( (Float)mDpSize.getHeight() - mSlider->getSize().getHeight() ) * Percent ) );
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

void UISlider::update( const Time& time ) {
	UINode::update( time );

	if ( NULL != getEventDispatcher() && ( isMouseOver() || mBackSlider->isMouseOver() || mSlider->isMouseOver() ) ) {
		manageClick( getEventDispatcher()->getClickTrigger() );
	}
}

Uint32 UISlider::onKeyDown( const KeyEvent &Event ) {
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

	return UIWidget::onKeyDown( Event );
}

void UISlider::manageClick( const Uint32& Flags ) {
	if ( Flags && NULL != getEventDispatcher() ) {
		Vector2f ControlPos = getEventDispatcher()->getMousePosf();
		mSlider->worldToNode( ControlPos );

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

UISlider * UISlider::setOrientation( const UI_ORIENTATION & orientation ) {
	mOrientation = orientation;

	applyDefaultTheme();

	return this;
}

bool UISlider::getAllowHalfSliderOut() const {
	return mStyleConfig.AllowHalfSliderOut;
}

void UISlider::setAllowHalfSliderOut( bool allowHalfSliderOut ) {
	mStyleConfig.AllowHalfSliderOut = allowHalfSliderOut;

	adjustChilds();

	setValue( mValue );
}

bool UISlider::getExpandBackground() const {
	return mStyleConfig.ExpandBackground;
}

void UISlider::setExpandBackground( bool expandBackground ) {
	mStyleConfig.ExpandBackground = expandBackground;

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

UINode * UISlider::getBackSlider() const {
	return mBackSlider;
}

UINode * UISlider::getSliderButton() const {
	return mSlider;
}

void UISlider::onAlphaChange() {
	UINode::onAlphaChange();
	
	mBackSlider->setAlpha( mAlpha );
	mSlider->setAlpha( mAlpha );
}

bool UISlider::setAttribute( const NodeAttribute& attribute ) {
	const std::string& name = attribute.getName();

	if ( "orientation" == name ) {
		std::string val = attribute.asString();
		String::toLowerInPlace( val );

		if ( "horizontal" == val )
			setOrientation( UI_HORIZONTAL );
		else if ( "vertical" == val )
			setOrientation( UI_VERTICAL );
	} else if ( "minvalue" == name ) {
		setMinValue( attribute.asFloat() );
	} else if ( "maxvalue" == name ) {
		setMaxValue( attribute.asFloat() );
	} else if ( "value" == name ) {
		setValue( attribute.asFloat() );
	} else if ( "clickstep" == name ) {
		setClickStep( attribute.asFloat() );
	} else if ( "pagestep" == name ) {
		setPageStep( attribute.asFloat() );
	} else {
		return UIWidget::setAttribute( attribute );
	}

	return true;
}

}}
