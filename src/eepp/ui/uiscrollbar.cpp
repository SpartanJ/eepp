#include <eepp/graphics/textureregion.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiscrollbar.hpp>

namespace EE { namespace UI {

UIScrollBar* UIScrollBar::New() {
	return eeNew( UIScrollBar, () );
}

UIScrollBar* UIScrollBar::NewHorizontal() {
	return eeNew( UIScrollBar, ( "scrollbar", UIOrientation::Horizontal ) );
}

UIScrollBar* UIScrollBar::NewVertical() {
	return eeNew( UIScrollBar, ( "scrollbar", UIOrientation::Vertical ) );
}

UIScrollBar* UIScrollBar::NewWithTag( const std::string& tag ) {
	return eeNew( UIScrollBar, ( tag, UIOrientation::Vertical ) );
}

UIScrollBar* UIScrollBar::NewHorizontalWithTag( const std::string& tag ) {
	return eeNew( UIScrollBar, ( tag, UIOrientation::Horizontal ) );
}

UIScrollBar* UIScrollBar::NewVerticalWithTag( const std::string& tag ) {
	return eeNew( UIScrollBar, ( tag, UIOrientation::Vertical ) );
}

UIScrollBar::UIScrollBar( const std::string& tag, const UIOrientation& orientation ) :
	UIWidget( tag ),
#ifdef EE_PLATFORM_TOUCH
	mScrollBarStyle( NoButtons )
#else
	mScrollBarStyle( TwoButtons )
#endif
{
	mFlags |= UI_AUTO_SIZE;

	setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	if ( orientation == UIOrientation::Vertical ) {
		mBtnDown = UIWidget::NewWithTag( mTag + "::btndown" );
		mBtnUp = UIWidget::NewWithTag( mTag + "::btnup" );
	} else {
		mBtnDown = UIWidget::NewWithTag( mTag + "::btnleft" );
		mBtnUp = UIWidget::NewWithTag( mTag + "::btnright" );
	}
	mBtnUp->setParent( this );
	mBtnUp->setSize( 8, 8 );
	mBtnDown->setParent( this );
	mBtnDown->setSize( 8, 8 );

	mSlider = UISlider::NewWithTag( orientation == UIOrientation::Vertical ? mTag + "::vslider"
																		   : mTag + "::hslider",
									orientation );
	mSlider->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	mSlider->setOrientation( orientation );
	mSlider->setParent( this );
	mSlider->setAllowHalfSliderOut( false );
	mSlider->setExpandBackground( false );
	mSlider->addEventListener( Event::OnValueChange,
							   [this] ( auto event ) { onValueChangeCb( event ); } );
	if ( orientation == UIOrientation::Vertical ) {
		mSlider->getSliderButton()->setElementTag( mTag + "::vbutton" );
		mSlider->getBackSlider()->setElementTag( mTag + "::vback" );
	} else {
		mSlider->getSliderButton()->setElementTag( mTag + "::hbutton" );
		mSlider->getBackSlider()->setElementTag( mTag + "::hback" );
	}

	applyDefaultTheme();
}

UIScrollBar::~UIScrollBar() {}

Uint32 UIScrollBar::getType() const {
	return UI_TYPE_SCROLLBAR;
}

bool UIScrollBar::isType( const Uint32& type ) const {
	return UIScrollBar::getType() == type ? true : UIWidget::isType( type );
}

void UIScrollBar::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	if ( !isVertical() ) {
		UINode::setThemeSkin( Theme, "hscrollbar" );
		mSlider->setThemeSkin( Theme, "hscrollbar_slider" );
		mSlider->getBackSlider()->setThemeSkin( Theme, "hscrollbar_bg" );
		mSlider->getSliderButton()->setThemeSkin( Theme, "hscrollbar_button" );
		mBtnUp->setThemeSkin( Theme, "hscrollbar_btnup" );
		mBtnDown->setThemeSkin( Theme, "hscrollbar_btndown" );
	} else {
		UINode::setThemeSkin( Theme, "vscrollbar" );
		mSlider->setThemeSkin( Theme, "vscrollbar_slider" );
		mSlider->getBackSlider()->setThemeSkin( Theme, "vscrollbar_bg" );
		mSlider->getSliderButton()->setThemeSkin( Theme, "vscrollbar_button" );
		mBtnUp->setThemeSkin( Theme, "vscrollbar_btnup" );
		mBtnDown->setThemeSkin( Theme, "vscrollbar_btndown" );
	}

	UISkin* tSkin = mBtnUp->getSkin();

	if ( NULL != tSkin ) {
		mBtnUp->setSize( tSkin->getSize() );
	}

	tSkin = mBtnDown->getSkin();

	if ( NULL != tSkin ) {
		mBtnDown->setSize( tSkin->getSize() );
	}

	adjustChilds();

	mSlider->adjustChilds();

	onThemeLoaded();
}

void UIScrollBar::onAutoSize() {
	Sizef size;
	UISkin* tSkin = mSlider->getBackSlider()->getSkin();

	if ( NULL != tSkin ) {
		size = tSkin->getSize();

		setMinSize( PixelDensity::pxToDp( size ) );

		if ( mFlags & UI_AUTO_SIZE ) {
			if ( mSlider->isVertical() ) {
				mSlider->setSize( size.getWidth(), getSize().getHeight() );
				setSize( size.getWidth(), getSize().getHeight() );
			} else {
				mSlider->setSize( getSize().getWidth(), size.getHeight() );
				setSize( getSize().getWidth(), size.getHeight() );
			}
		}
	} else if ( NULL != mSlider->getSliderButton() ) {
		tSkin = mSlider->getSliderButton()->getSkin();

		if ( NULL != tSkin ) {
			size = tSkin->getSize();

			setMinSize( PixelDensity::pxToDp( size ) );

			if ( mFlags & UI_AUTO_SIZE ) {
				if ( mSlider->isVertical() ) {
					setSize( size.getWidth(), getSize().getHeight() );
				} else {
					setSize( getSize().getWidth(), size.getHeight() );
				}
			}
		}
	}

	if ( mWidthPolicy == SizePolicy::WrapContent || mHeightPolicy == SizePolicy::WrapContent ) {
		size = PixelDensity::dpToPx( mSlider->getSize() ) + mPaddingPx;

		if ( mScrollBarStyle == TwoButtons ) {
			if ( mSlider->isVertical() ) {
				size.y +=
					mBtnDown->getPixelsSize().getHeight() + mBtnUp->getPixelsSize().getHeight();
			} else {
				size.x += mBtnDown->getPixelsSize().getWidth() + mBtnUp->getPixelsSize().getWidth();
			}
		}

		if ( mWidthPolicy == SizePolicy::WrapContent ) {
			setInternalPixelsWidth( size.getWidth() );
		}

		if ( mHeightPolicy == SizePolicy::WrapContent ) {
			setInternalPixelsHeight( size.getHeight() );
		}

		adjustChilds();
	}
}

void UIScrollBar::onSizeChange() {
	onAutoSize();

	adjustChilds();

	UIWidget::onSizeChange();
}

void UIScrollBar::adjustChilds() {
	if ( mNodeFlags & NODE_FLAG_FREE_USE )
		return;

	mNodeFlags |= NODE_FLAG_FREE_USE;

	mBtnUp->setPosition( 0, 0 );

	switch ( mScrollBarStyle ) {
		case NoButtons: {
			mBtnDown->setVisible( false )->setEnabled( false );
			mBtnUp->setVisible( false )->setEnabled( false );

			if ( !isVertical() ) {
				mSlider->setSize( getSize() - mPadding )
					->setPosition( mPadding.Left, mPadding.Top )
					->centerVertical();
			} else {
				mSlider->setSize( getSize() - mPadding )
					->setPosition( mPadding.Left, mPadding.Top )
					->centerHorizontal();
			}

			break;
		}
		case TwoButtons:
		default: {
			mBtnDown->setVisible( true )->setEnabled( true );
			mBtnUp->setVisible( true )->setEnabled( true );

			if ( !isVertical() ) {
				mBtnUp->setPosition( mPadding.Left, mPadding.Top );
				mBtnDown->setPosition( getSize().getWidth() - mBtnDown->getSize().getWidth() -
										   mPadding.Right,
									   mPadding.Top );
				mSlider->setSize( getSize().getWidth() - mBtnDown->getSize().getWidth() -
									  mBtnUp->getSize().getWidth() - mPadding.Left - mPadding.Right,
								  getSize().getHeight() - mPadding.Top - mPadding.Bottom );
				mSlider->setPosition( mPadding.Left + mBtnUp->getSize().getWidth(), mPadding.Top );

				mBtnDown->centerVertical();
				mBtnUp->centerVertical();
				mSlider->centerVertical();
			} else {
				mBtnUp->setPosition( mPadding.Left, mPadding.Top );
				mBtnDown->setPosition( mPadding.Left, getSize().getHeight() -
														  mBtnDown->getSize().getHeight() -
														  mPadding.Bottom );
				mSlider->setSize( getSize().getWidth() - mPadding.Left - mPadding.Right,
								  getSize().getHeight() - mBtnDown->getSize().getHeight() -
									  mBtnUp->getSize().getHeight() - mPadding.Top -
									  mPadding.Bottom );
				mSlider->setPosition( mPadding.Left, mBtnUp->getSize().getHeight() + mPadding.Top );

				mBtnDown->centerHorizontal();
				mBtnUp->centerHorizontal();
				mSlider->centerHorizontal();
			}

			break;
		}
	}

	mSlider->adjustChilds();

	mNodeFlags &= ~NODE_FLAG_FREE_USE;
}

Uint32 UIScrollBar::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::DragStart: {
			addClass( "dragging" );
			return 1;
		}
		case NodeMessage::DragStop: {
			removeClass( "dragging" );
			return 1;
		}
		case NodeMessage::MouseClick: {
			if ( Msg->getFlags() & EE_BUTTON_LMASK ) {
				if ( Msg->getSender() == mBtnUp ) {
					mSlider->setValue( getValue() - getClickStep() );
				} else if ( Msg->getSender() == mBtnDown ) {
					mSlider->setValue( getValue() + getClickStep() );
				}
			}

			return 1;
		}
	}

	return 0;
}

void UIScrollBar::setValue( Float val, const bool& emmitEvent ) {
	mSlider->setValue( val, emmitEvent );
}

const Float& UIScrollBar::getValue() const {
	return mSlider->getValue();
}

void UIScrollBar::setMinValue( const Float& MinVal ) {
	mSlider->setMinValue( MinVal );
}

const Float& UIScrollBar::getMinValue() const {
	return mSlider->getMinValue();
}

void UIScrollBar::setMaxValue( const Float& MaxVal ) {
	mSlider->setMaxValue( MaxVal );
}

const Float& UIScrollBar::getMaxValue() const {
	return mSlider->getMaxValue();
}

void UIScrollBar::setClickStep( const Float& step ) {
	mSlider->setClickStep( step );
}

const Float& UIScrollBar::getClickStep() const {
	return mSlider->getClickStep();
}

Float UIScrollBar::getPageStep() const {
	return mSlider->getPageStep();
}

void UIScrollBar::setPageStep( const Float& pageStep ) {
	mSlider->setPageStep( pageStep );
}

bool UIScrollBar::isVertical() const {
	return mSlider->isVertical();
}

void UIScrollBar::onValueChangeCb( const Event* ) {
	onValueChange();
}

UISlider* UIScrollBar::getSlider() const {
	return mSlider;
}

UINode* UIScrollBar::getButtonUp() const {
	return mBtnUp;
}

UINode* UIScrollBar::getButtonDown() const {
	return mBtnDown;
}

bool UIScrollBar::getExpandBackground() const {
	return mSlider->getExpandBackground();
}

void UIScrollBar::setExpandBackground( bool expandBackground ) {
	if ( mSlider->getExpandBackground() != expandBackground ) {
		mSlider->setExpandBackground( expandBackground );

		onAutoSize();

		adjustChilds();
	}
}

std::string UIScrollBar::getPropertyString( const PropertyDefinition* propertyDef,
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
		case PropertyId::ScrollBarStyle:
			return getScrollBarType() == NoButtons ? "no-buttons" : "two-buttons";
		case PropertyId::BackgroundExpand:
			return getExpandBackground() ? "true" : "false";
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIScrollBar::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	auto local = { PropertyId::Orientation,		PropertyId::MinValue,  PropertyId::MaxValue,
				   PropertyId::Value,			PropertyId::ClickStep, PropertyId::PageStep,
				   PropertyId::BackgroundExpand };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

bool UIScrollBar::isDragging() const {
	return mSlider->isDragging();
}

bool UIScrollBar::applyProperty( const StyleSheetProperty& attribute ) {
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
		case PropertyId::ScrollBarStyle: {
			std::string val = attribute.asString();
			String::toLowerInPlace( val );

			if ( "no-buttons" == val || "nobuttons" == val ) {
				setScrollBarStyle( NoButtons );
			} else if ( "two-buttons" == val || "twobuttons" == val ) {
				setScrollBarStyle( TwoButtons );
			}
			break;
		}
		case PropertyId::BackgroundExpand:
			setExpandBackground( attribute.asBool() );
			break;
		default:
			return UIWidget::applyProperty( attribute );
	}

	return true;
}

UIScrollBar::ScrollBarType UIScrollBar::getScrollBarType() const {
	return mScrollBarStyle;
}

void UIScrollBar::setScrollBarStyle( const ScrollBarType& scrollBarType ) {
	if ( mScrollBarStyle != scrollBarType ) {
		mScrollBarStyle = scrollBarType;

		onAutoSize();

		adjustChilds();
	}
}

UIOrientation UIScrollBar::getOrientation() const {
	return mSlider->getOrientation();
}

UINode* UIScrollBar::setOrientation( const UIOrientation& orientation ) {
	if ( mSlider->getOrientation() != orientation ) {
		if ( orientation == UIOrientation::Vertical ) {
			mSlider->setElementTag( mTag + "::vslider" );
			mBtnDown->setElementTag( mTag + "::btndown" );
			mBtnUp->setElementTag( mTag + "::btnup" );
		} else {
			mSlider->setElementTag( mTag + "::hslider" );
			mBtnDown->setElementTag( mTag + "::btnleft" );
			mBtnUp->setElementTag( mTag + "::btnright" );
		}

		mSlider->setOrientation( orientation, mTag );

		applyDefaultTheme();

		adjustChilds();
	}

	return this;
}

void UIScrollBar::onAlphaChange() {
	UINode::onAlphaChange();

	mSlider->setAlpha( mAlpha );
	mBtnUp->setAlpha( mAlpha );
	mBtnDown->setAlpha( mAlpha );
}

void UIScrollBar::onPaddingChange() {
	onAutoSize();

	adjustChilds();

	UIWidget::onPaddingChange();
}

}} // namespace EE::UI
