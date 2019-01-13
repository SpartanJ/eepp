#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/graphics/textureregion.hpp>

namespace EE { namespace UI {

UIScrollBar * UIScrollBar::New() {
	return eeNew( UIScrollBar, ( UI_VERTICAL ) );
}

UIScrollBar * UIScrollBar::NewHorizontal() {
	return eeNew( UIScrollBar, ( UI_HORIZONTAL ) );
}

UIScrollBar * UIScrollBar::NewVertical() {
	return eeNew( UIScrollBar, ( UI_VERTICAL ) );
}

UIScrollBar::UIScrollBar( const UI_ORIENTATION& orientation ) :
	UIWidget( "scrollbar" ),
#ifdef EE_PLATFORM_TOUCH
	mScrollBarType( NoButtons )
#else
	mScrollBarType( TwoButtons )
#endif
{
	mFlags |= UI_AUTO_SIZE;

	setLayoutSizeRules( FIXED, FIXED );

	mBtnDown	= UINode::New();
	mBtnUp		= UINode::New();
	mBtnUp->setParent( this );
	mBtnUp->setSize( 16, 16 );
	mBtnDown->setParent( this );
	mBtnDown->setSize( 16, 16 );

	mSlider		= UISlider::New();
	mSlider->setElementTag( "scrollbarslider" );
	mSlider->setOrientation( orientation );
	mSlider->setParent( this );
	mSlider->setAllowHalfSliderOut( false );
	mSlider->setExpandBackground( false );

	mSlider->addEventListener( Event::OnValueChange, cb::Make1( this, &UIScrollBar::onValueChangeCb ) );

	adjustChilds();

	applyDefaultTheme();
}

UIScrollBar::~UIScrollBar() {
}

Uint32 UIScrollBar::getType() const {
	return UI_TYPE_SCROLLBAR;
}

bool UIScrollBar::isType( const Uint32& type ) const {
	return UIScrollBar::getType() == type ? true : UIWidget::isType( type );
}

void UIScrollBar::setTheme( UITheme * Theme ) {
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

	UISkin * tSkin = mBtnUp->getSkin();

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
	UISkin * tSkin = mSlider->getBackSlider()->getSkin();

	if ( NULL != tSkin ) {
		size = tSkin->getSize();

		mMinControlSize = PixelDensity::pxToDp( size );

		if ( mFlags & UI_AUTO_SIZE ) {
			if ( mSlider->isVertical() ) {
				mSlider->setSize( size.getWidth() , mDpSize.getHeight() );
				setSize( size.getWidth(), mDpSize.getHeight() );
			} else {
				mSlider->setSize( mDpSize.getWidth(), size.getHeight() );
				setSize( mDpSize.getWidth(), size.getHeight() );
			}
		}
	} else if ( NULL != mSlider->getSliderButton() ) {
		tSkin = mSlider->getSliderButton()->getSkin();

		if ( NULL != tSkin ) {
			size = tSkin->getSize();

			if ( mFlags & UI_AUTO_SIZE ) {
				if ( mSlider->isVertical() ) {
					setSize( size.getWidth(), mDpSize.getHeight() );
				} else {
					setSize( mDpSize.getWidth(), size.getHeight() );
				}
			}
		}
	}

	if ( mLayoutWidthRules == WRAP_CONTENT || mLayoutHeightRules == WRAP_CONTENT ) {
		size =  PixelDensity::dpToPx( mSlider->getMinimumSize() ) + mRealPadding;

		if (  mScrollBarType == TwoButtons ) {
			if ( mSlider->isVertical() ) {
				size.y += mBtnDown->getPixelsSize().getHeight() + mBtnUp->getPixelsSize().getHeight();
			} else {
				size.x += mBtnDown->getPixelsSize().getWidth() + mBtnUp->getPixelsSize().getWidth();
			}
		}

		if ( mLayoutWidthRules == WRAP_CONTENT ) {
			setInternalPixelsWidth( size.getWidth() );
		}

		if ( mLayoutHeightRules == WRAP_CONTENT ) {
			setInternalPixelsHeight( size.getHeight() );
		}
	}
}

void UIScrollBar::onSizeChange() {
	adjustChilds();

	mSlider->adjustChilds();

	UIWidget::onSizeChange();
}

void UIScrollBar::adjustChilds() {
	onAutoSize();

	mBtnUp->setPosition( 0, 0 );

	switch ( mScrollBarType ) {
		case NoButtons:
		{
			mBtnDown->setVisible( false )->setEnabled( false );
			mBtnUp->setVisible( false )->setEnabled( false );

			if ( !isVertical() ) {
				mSlider->setSize( mDpSize - mPadding )->setPosition( mPadding.Left, mPadding.Top )->centerVertical();
			} else {
				mSlider->setSize( mDpSize - mPadding )->setPosition( mPadding.Left, mPadding.Top )->centerHorizontal();
			}

			break;
		}
		case TwoButtons:
		default:
		{
			mBtnDown->setVisible( true )->setEnabled( true );
			mBtnUp->setVisible( true )->setEnabled( true );

			if ( !isVertical() ) {
				mBtnUp->setPosition( mPadding.Left, mPadding.Top );
				mBtnDown->setPosition( mDpSize.getWidth() - mBtnDown->getSize().getWidth() - mPadding.Right, mPadding.Top );
				mSlider->setSize( mDpSize.getWidth() - mBtnDown->getSize().getWidth() - mBtnUp->getSize().getWidth() - mPadding.Left - mPadding.Right, mDpSize.getHeight() - mPadding.Top - mPadding.Bottom );
				mSlider->setPosition( mPadding.Left + mBtnUp->getSize().getWidth(), mPadding.Top );

				mBtnDown->centerVertical();
				mBtnUp->centerVertical();
				mSlider->centerVertical();
			} else {
				mBtnUp->setPosition( mPadding.Left, mPadding.Top );
				mBtnDown->setPosition( mPadding.Left, mDpSize.getHeight() - mBtnDown->getSize().getHeight() - mPadding.Bottom );
				mSlider->setSize( mDpSize.getWidth() - mPadding.Left - mPadding.Right, mDpSize.getHeight() - mBtnDown->getSize().getHeight() - mBtnUp->getSize().getHeight() - mPadding.Top - mPadding.Bottom );
				mSlider->setPosition( mPadding.Left, mBtnUp->getSize().getHeight() + mPadding.Top );

				mBtnDown->centerHorizontal();
				mBtnUp->centerHorizontal();
				mSlider->centerHorizontal();
			}

			break;
		}
	}
}

Uint32 UIScrollBar::onMessage( const NodeMessage * Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::Click:
		{
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

void UIScrollBar::setValue( Float Val ) {
	mSlider->setValue( Val );
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

void UIScrollBar::setPageStep(const Float& pageStep) {
	mSlider->setPageStep( pageStep );
}

bool UIScrollBar::isVertical() const {
	return mSlider->isVertical();
}

void UIScrollBar::onValueChangeCb( const Event * ) {
	onValueChange();
}

UISlider * UIScrollBar::getSlider() const {
	return mSlider;
}

UINode * UIScrollBar::getButtonUp() const {
	return mBtnUp;
}

UINode * UIScrollBar::getButtonDown() const {
	return mBtnDown;
}

bool UIScrollBar::getExpandBackground() const {
	return mSlider->getExpandBackground();
}

void UIScrollBar::setExpandBackground( bool expandBackground ) {
	mSlider->setExpandBackground( expandBackground );

	adjustChilds();
}

bool UIScrollBar::setAttribute( const NodeAttribute& attribute, const Uint32& state ) {
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
	} else if ( "scrollbartype" == name ) {
		std::string val = attribute.asString();
		String::toLowerInPlace( val );

		if ( "nobuttons" == val ) {
			setScrollBarType( NoButtons );
		} else if ( "twobuttons" == val ) {
			setScrollBarType( TwoButtons );
		}
	} else if ( "expandbackground" == name ) {
		setExpandBackground( attribute.asBool() );
	} else {
		return UIWidget::setAttribute( attribute, state );
	}

	return true;
}

UIScrollBar::ScrollBarType UIScrollBar::getScrollBarType() const {
	return mScrollBarType;
}

void UIScrollBar::setScrollBarType( const ScrollBarType & scrollBarType ) {
	if ( mScrollBarType != scrollBarType ) {
		mScrollBarType = scrollBarType;

		adjustChilds();
	}
}

UI_ORIENTATION UIScrollBar::getOrientation() const {
	return mSlider->getOrientation();
}

UINode * UIScrollBar::setOrientation( const UI_ORIENTATION & orientation ) {
	mSlider->setOrientation( orientation );

	applyDefaultTheme();

	return this;
}

void UIScrollBar::onAlphaChange() {
	UINode::onAlphaChange();
	
	mSlider->setAlpha( mAlpha );
	mBtnUp->setAlpha( mAlpha );
	mBtnDown->setAlpha( mAlpha );
}

void UIScrollBar::onPaddingChange() {
	adjustChilds();
	UIWidget::onPaddingChange();
}

}}
