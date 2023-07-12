#include <eepp/graphics/textureregion.hpp>
#include <eepp/scene/eventdispatcher.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uislider.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace UI {

UISlider* UISlider::New() {
	return NewWithTag( "slider", UIOrientation::Vertical );
}

UISlider* UISlider::NewWithTag( const std::string& tag, const UIOrientation& orientation ) {
	return eeNew( UISlider, ( tag, orientation ) );
}

UISlider* UISlider::NewVertical() {
	return NewVerticalWithTag( "slider" );
}

UISlider* UISlider::NewHorizontal() {
	return NewHorizontalWithTag( "slider" );
}

UISlider* UISlider::NewVerticalWithTag( const std::string& tag ) {
	return eeNew( UISlider, ( tag, UIOrientation::Vertical ) );
}

UISlider* UISlider::NewHorizontalWithTag( const std::string& tag ) {
	return eeNew( UISlider, ( tag, UIOrientation::Horizontal ) );
}

UISlider::UISlider( const std::string& tag, const UIOrientation& orientation ) :
	UIWidget( tag ),
	mOrientation( orientation ),
	mAllowHalfSliderOut( false ),
	mExpandBackground( false ),
	mUpdating( false ),
	mBackSlider( NULL ),
	mSlider( NULL ),
	mMinValue( 0.f ),
	mMaxValue( 1.f ),
	mValue( 0.f ),
	mClickStep( 0.1f ),
	mPageStep( 0 ),
	mOnPosChange( false ) {
	mFlags |= UI_SCROLLABLE;

	if ( UIOrientation::Horizontal == mOrientation ) {
		mBackSlider = UIWidget::NewWithTag( mTag + "::hback" );
		mSlider = UIWidget::NewWithTag( mTag + "::hbutton" );
	} else {
		mBackSlider = UIWidget::NewWithTag( mTag + "::vback" );
		mSlider = UIWidget::NewWithTag( mTag + "::vbutton" );
	}

	auto cb = [this]( const Event* ) {
		if ( !mUpdating )
			adjustChilds();
	};

	mBackSlider->setParent( this );
	mBackSlider->setVisible( true );
	mBackSlider->setEnabled( true );
	mBackSlider->center();

	mSlider->setParent( this );
	mSlider->setEnabled( true );
	mSlider->setVisible( true );
	mSlider->setDragEnabled( true );
	mSlider->setSize( 4, 4 );
	mSlider->setPosition( 0, 0 );
	mSlider->addEventListener( Event::OnPositionChange, [this]( const Event* ) {
		if ( !mUpdating && !mOnPosChange )
			fixSliderPos();
	} );

	if ( UIOrientation::Horizontal == mOrientation )
		mSlider->centerVertical();
	else
		mSlider->centerHorizontal();

	mBackSlider->addEventListener( Event::OnSizeChange, cb );
	mSlider->addEventListener( Event::OnSizeChange, cb );
	mBackSlider->addEventListener( Event::OnPaddingChange, cb );
	mSlider->addEventListener( Event::OnPaddingChange, cb );

	applyDefaultTheme();
}

UISlider::~UISlider() {}

Uint32 UISlider::getType() const {
	return UI_TYPE_SLIDER;
}

bool UISlider::isType( const Uint32& type ) const {
	return UISlider::getType() == type ? true : UIWidget::isType( type );
}

void UISlider::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	if ( UIOrientation::Horizontal == mOrientation ) {
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

	onThemeLoaded();
}

void UISlider::onSizeChange() {
	UIWidget::onSizeChange();
	adjustChilds();
}

void UISlider::onPaddingChange() {
	adjustChilds();
	UIWidget::onPaddingChange();
}

Uint32 UISlider::onMouseDown( const Vector2i& position, const Uint32& flags ) {
	if ( !getEventDispatcher()->isNodeDragging() ) {
		Vector2f mouseDownInitPos( getEventDispatcher()->getMouseDownPos().asFloat() );
		worldToNode( mouseDownInitPos );
		if ( getLocalDpBounds().contains( mouseDownInitPos ) ) {
			Vector2f localPos( position.asFloat() );
			worldToNode( localPos );
			if ( localPos.y >= mSlider->getPosition().y &&
				 localPos.y <= mSlider->getPosition().y + mSlider->getSize().getHeight() ) {
				mSlider->startDragging( position.asFloat() );
			}
		}
	}
	return UIWidget::onMouseDown( position, flags );
}

void UISlider::adjustChilds() {
	mUpdating = true;

	UISkin* tSkin = mSlider->getSkin();

	if ( NULL != tSkin )
		mSlider->setMinSize( tSkin->getSize() );

	if ( mPageStep == 0 ) {
		mSlider->setSize( mSlider->getMinSize() );
	} else {
		Float percent = ( mPageStep / ( mMaxValue - mMinValue ) );

		if ( UIOrientation::Horizontal == mOrientation ) {
			Float size = eeceil( eemax(
				( (Float)( getSize().getWidth() - mPadding.Left - mPadding.Right ) * percent ),
				mSlider->getMinSize().getWidth() ) );

			mSlider->setSize( size, mSlider->getSize().getHeight() );
		} else {
			Float size = eeceil( eemax(
				( (Float)( getSize().getHeight() - mPadding.Top - mPadding.Bottom ) * percent ),
				mSlider->getMinSize().getHeight() ) );

			mSlider->setSize( mSlider->getSize().getWidth(), size );
		}
	}

	if ( UIOrientation::Horizontal == mOrientation ) {
		if ( mSlider->getSize().getHeight() > getMinSize().getHeight() )
			setMinHeight( mSlider->getSize().getHeight() );

		mSlider->centerVertical();
	} else {
		if ( mSlider->getSize().getWidth() > getMinimumSize().getWidth() )
			setMinWidth( mSlider->getSize().getWidth() );

		mSlider->centerHorizontal();
	}

	tSkin = mBackSlider->getSkin();

	if ( NULL != tSkin )
		mBackSlider->setMinSize( tSkin->getSize() );

	if ( UIOrientation::Horizontal == mOrientation ) {
		Float Height;

		if ( mExpandBackground )
			Height = getSize().getHeight() - mPadding.Top - mPadding.Bottom;
		else
			Height = mBackSlider->getMinSize().getHeight();

		if ( mAllowHalfSliderOut ) {
			mBackSlider->setSize( Sizef( getSize().getWidth() - mSlider->getSize().getWidth() -
											 mPadding.Left - mPadding.Right,
										 Height ) );
		} else {
			mBackSlider->setSize(
				Sizef( getSize().getWidth() - mPadding.Left - mPadding.Right, Height ) );
		}

		if ( Height > getMinSize().getHeight() )
			setMinHeight( Height );
	} else {
		Float Width;

		if ( mExpandBackground ) {
			Width = getSize().getWidth() - mPadding.Left - mPadding.Right;
		} else {
			Width = mBackSlider->getMinSize().getWidth();
		}

		if ( mAllowHalfSliderOut ) {
			mBackSlider->setSize( Sizef( Width, getSize().getHeight() -
													mSlider->getSize().getHeight() - mPadding.Top -
													mPadding.Bottom ) );
		} else {
			mBackSlider->setSize(
				Sizef( Width, getSize().getHeight() - mPadding.Top - mPadding.Bottom ) );
		}

		if ( Width > getMinimumSize().getWidth() )
			setMinWidth( Width );
	}

	mBackSlider->center();

	adjustSliderPos();

	mUpdating = false;
}

void UISlider::fixSliderPos() {
	if ( !mOnPosChange ) {
		mOnPosChange = true;

		if ( UIOrientation::Horizontal == mOrientation ) {
			mSlider->setPosition( mSlider->getPosition().x, 0 );

			if ( mSlider->getPosition().x < mPadding.Left )
				mSlider->setPosition( mPadding.Left, 0 );

			if ( mAllowHalfSliderOut ) {
				if ( mSlider->getPosition().x > mBackSlider->getSize().getWidth() + mPadding.Left )
					mSlider->setPosition( mBackSlider->getSize().getWidth() + mPadding.Left, 0 );
			} else {
				if ( mSlider->getPosition().x > mBackSlider->getSize().getWidth() -
													mSlider->getSize().getWidth() + mPadding.Left )
					mSlider->setPosition( eemax( 0.f, mBackSlider->getSize().getWidth() -
														  mSlider->getSize().getWidth() ) +
											  mPadding.Left,
										  0 );
			}

			mSlider->centerVertical();

			if ( mAllowHalfSliderOut )
				setValue( mMinValue + ( mSlider->getPosition().x - mPadding.Left ) *
										  ( mMaxValue - mMinValue ) /
										  (Float)mBackSlider->getSize().getWidth() );
			else
				setValue( mMinValue + ( mSlider->getPosition().x - mPadding.Left ) *
										  ( mMaxValue - mMinValue ) /
										  ( (Float)mBackSlider->getSize().getWidth() -
											mSlider->getSize().getWidth() ) );
		} else {
			mSlider->setPosition( 0, mSlider->getPosition().y );

			if ( mSlider->getPosition().y < mPadding.Top )
				mSlider->setPosition( 0, mPadding.Top );

			if ( mAllowHalfSliderOut ) {
				if ( mSlider->getPosition().y > mBackSlider->getSize().getHeight() + mPadding.Top )
					mSlider->setPosition( 0, mBackSlider->getSize().getHeight() + mPadding.Top );
			} else {
				if ( mSlider->getPosition().y > mBackSlider->getSize().getHeight() -
													mSlider->getSize().getHeight() +
													mPadding.Top ) {
					mSlider->setPosition( 0, eemax( 0.f, mBackSlider->getSize().getHeight() -
															 mSlider->getSize().getHeight() ) +
												 mPadding.Top );
				}
			}

			mSlider->centerHorizontal();

			if ( mAllowHalfSliderOut )
				setValue( mMinValue + ( mSlider->getPosition().y - mPadding.Top ) *
										  ( mMaxValue - mMinValue ) /
										  (Float)mBackSlider->getSize().getHeight() );
			else
				setValue( mMinValue + ( mSlider->getPosition().y - mPadding.Top ) *
										  ( mMaxValue - mMinValue ) /
										  ( (Float)mBackSlider->getSize().getHeight() -
											mSlider->getSize().getHeight() ) );
		}

		mOnPosChange = false;
	}
}

void UISlider::adjustSliderPos() {
	Float Percent = ( mValue - mMinValue ) / ( mMaxValue - mMinValue );
	mOnPosChange = true;

	if ( UIOrientation::Horizontal == mOrientation ) {
		if ( mAllowHalfSliderOut )
			mSlider->setPosition( mPadding.Left +
									  (Int32)( (Float)mBackSlider->getSize().getWidth() * Percent ),
								  mSlider->getPosition().y );
		else
			mSlider->setPosition( mPadding.Left +
									  (Int32)( ( (Float)getSize().getWidth() - mPadding.Left -
												 mPadding.Right - mSlider->getSize().getWidth() ) *
											   Percent ),
								  mSlider->getPosition().y );
	} else {
		if ( mAllowHalfSliderOut )
			mSlider->setPosition(
				mSlider->getPosition().x,
				mPadding.Top + (Int32)( (Float)mBackSlider->getSize().getHeight() * Percent ) );
		else
			mSlider->setPosition(
				mSlider->getPosition().x,
				mPadding.Top + (Int32)( ( (Float)getSize().getHeight() - mPadding.Top -
										  mPadding.Bottom - mSlider->getSize().getHeight() ) *
										Percent ) );
	}

	mOnPosChange = false;
}

void UISlider::setValue( Float val, bool emmitEvent ) {
	if ( val < mMinValue )
		val = mMinValue;
	if ( val > mMaxValue )
		val = mMaxValue;

	if ( mValue == val )
		return;

	if ( val >= mMinValue && val <= mMaxValue ) {
		mValue = val;

		if ( !mOnPosChange ) {
			mOnPosChange = true;

			adjustSliderPos();

			mOnPosChange = false;
		}

		if ( emmitEvent )
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
	return mOrientation == UIOrientation::Vertical;
}

Uint32 UISlider::onKeyDown( const KeyEvent& Event ) {
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
		Vector2f nodePos = getEventDispatcher()->getMousePosf();
		mSlider->worldToNode( nodePos );

		if ( Flags & EE_BUTTON_LMASK && !mSlider->isMouseOver() && !mSlider->isDragging() &&
			 getUISceneNode()->getEventDispatcher()->getNodeWasDragging() != mSlider ) {
			if ( UIOrientation::Horizontal == mOrientation ) {
				if ( nodePos.x < 0 )
					setValue( mValue - mClickStep );
				else
					setValue( mValue + mClickStep );
			} else {
				if ( nodePos.y < 0 )
					setValue( mValue - mClickStep );
				else
					setValue( mValue + mClickStep );
			}
		} else if ( Flags & ( EE_BUTTONS_WUWD | EE_BUTTONS_WLWR ) ) {
			if ( Flags & ( EE_BUTTON_WUMASK | EE_BUTTON_WLMASK ) )
				setValue( mValue - mClickStep );
			else
				setValue( mValue + mClickStep );
		}
	}
}

UIOrientation UISlider::getOrientation() const {
	return mOrientation;
}

UISlider* UISlider::setOrientation( const UIOrientation& orientation, std::string childsBaseTag ) {
	if ( orientation != mOrientation ) {
		if ( childsBaseTag.empty() )
			childsBaseTag = mTag;

		mOrientation = orientation;

		mBackSlider->setMinSize( Sizef::Zero );
		mBackSlider->setMinSizeEq( "", "" );
		mSlider->setMinSize( Sizef::Zero );
		mSlider->setMinSizeEq( "", "" );
		mBackSlider->setSize( Sizef::Zero );
		mSlider->setSize( Sizef::Zero );

		applyDefaultTheme();

		if ( UIOrientation::Horizontal == mOrientation ) {
			mBackSlider->setElementTag( childsBaseTag + "::hback" );
			mSlider->setElementTag( childsBaseTag + "::hbutton" );
		} else {
			mBackSlider->setElementTag( childsBaseTag + "::vback" );
			mSlider->setElementTag( childsBaseTag + "::hbutton" );
		}

		adjustChilds();
	}

	return this;
}

bool UISlider::getAllowHalfSliderOut() const {
	return mAllowHalfSliderOut;
}

void UISlider::setAllowHalfSliderOut( bool allowHalfSliderOut ) {
	if ( mAllowHalfSliderOut != allowHalfSliderOut ) {
		mAllowHalfSliderOut = allowHalfSliderOut;

		adjustChilds();

		setValue( mValue );
	}
}

bool UISlider::getExpandBackground() const {
	return mExpandBackground;
}

void UISlider::setExpandBackground( bool expandBackground ) {
	if ( mExpandBackground != expandBackground ) {
		mExpandBackground = expandBackground;

		adjustChilds();

		setValue( mValue );
	}
}

Float UISlider::getPageStep() const {
	return mPageStep;
}

void UISlider::setPageStep( const Float& pageStep ) {
	if ( pageStep != mPageStep ) {
		mPageStep = eemin( eemax( pageStep, mMinValue ), mMaxValue );

		adjustChilds();

		setValue( mValue );
	}
}

UIWidget* UISlider::getBackSlider() const {
	return mBackSlider;
}

UIWidget* UISlider::getSliderButton() const {
	return mSlider;
}

void UISlider::onAlphaChange() {
	UINode::onAlphaChange();

	mBackSlider->setAlpha( mAlpha );
	mSlider->setAlpha( mAlpha );
}

Uint32 UISlider::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::MouseUp: {
			manageClick( Msg->getFlags() );
			return 1;
		}
	}

	return 0;
}

std::string UISlider::getPropertyString( const PropertyDefinition* propertyDef,
										 const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Orientation:
			return getOrientation() == UIOrientation::Horizontal ? "horizontal" : "vertical";
		case PropertyId::MinValue:
			return String::fromFloat( getMinValue() );
		case PropertyId::MaxValue:
			return String::fromFloat( getMaxValue() );
		case PropertyId::Value:
			return String::fromFloat( getValue() );
		case PropertyId::ClickStep:
			return String::fromFloat( getClickStep() );
		case PropertyId::PageStep:
			return String::fromFloat( getPageStep() );
		case PropertyId::HalfSlider:
			return getAllowHalfSliderOut() ? "true" : "false";
		case PropertyId::BackgroundExpand:
			return getExpandBackground() ? "true" : "false";
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UISlider::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	auto local = { PropertyId::Orientation, PropertyId::MinValue,		 PropertyId::MaxValue,
				   PropertyId::Value,		PropertyId::ClickStep,		 PropertyId::PageStep,
				   PropertyId::HalfSlider,	PropertyId::BackgroundExpand };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

bool UISlider::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Orientation: {
			std::string val = attribute.asString();
			String::toLowerInPlace( val );

			if ( "horizontal" == val )
				setOrientation( UIOrientation::Horizontal );
			else if ( "vertical" == val )
				setOrientation( UIOrientation::Vertical );
			break;
		}
		case PropertyId::MinValue:
			setMinValue( attribute.asFloat() );
			break;
		case PropertyId::MaxValue:
			setMaxValue( attribute.asFloat() );
			break;
		case PropertyId::Value:
			setValue( attribute.asFloat() );
			break;
		case PropertyId::ClickStep:
			setClickStep( attribute.asFloat() );
			break;
		case PropertyId::PageStep:
			setPageStep( attribute.asFloat() );
			break;
		case PropertyId::BackgroundExpand:
			setExpandBackground( attribute.asBool() );
			break;
		case PropertyId::HalfSlider:
			setAllowHalfSliderOut( attribute.asBool() );
			break;
		default:
			return UIWidget::applyProperty( attribute );
	}

	return true;
}

Sizef UISlider::getMinimumSize() {
	Float w = eemax( mBackSlider->getSkinSize().getWidth(), mSlider->getSkinSize().getWidth() );
	Float h = eemax( mBackSlider->getSkinSize().getHeight(), mSlider->getSkinSize().getHeight() );
	return Sizef( w + ( mAllowHalfSliderOut ? w : 0 ) + mPadding.Left + mPadding.Right,
				  h + mPadding.Top + mPadding.Bottom );
}

bool UISlider::isDragging() const {
	return mSlider && mSlider->isDragging();
}

void UISlider::onAutoSize() {
	if ( mWidthPolicy == SizePolicy::WrapContent || mHeightPolicy == SizePolicy::WrapContent ) {
		bool modified = false;
		Sizef total( getMinimumSize() );

		total = PixelDensity::dpToPx( total );

		if ( mWidthPolicy == SizePolicy::WrapContent ) {
			setInternalPixelsWidth( total.getWidth() );
			modified = true;
		}

		if ( mHeightPolicy == SizePolicy::WrapContent ) {
			setInternalPixelsHeight( total.getHeight() );
			modified = true;
		}

		if ( modified )
			adjustChilds();
	}
}

}} // namespace EE::UI
