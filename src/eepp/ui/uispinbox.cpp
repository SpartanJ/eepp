#include <eepp/ui/uispinbox.hpp>
#include <eepp/graphics/textureregion.hpp>
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UISpinBox * UISpinBox::New() {
	return eeNew( UISpinBox, () );
}

UISpinBox::UISpinBox() :
	UIWidget(),
	mMinValue( 0.f ),
	mMaxValue( 1024.f ),
	mValue( 0 ),
	mClickStep( 1.f )
{
	mInput	= UITextInput::New();
	mInput->setVisible( true );
	mInput->setEnabled( true );
	mInput->setParent( this );

	mPushUp	= UINode::New();
	mPushUp->setVisible( true );
	mPushUp->setEnabled( true );
	mPushUp->setParent( this );
	mPushUp->setSize( 16, 16 );

	mPushDown = UINode::New();
	mPushDown->setVisible( true );
	mPushDown->setEnabled( true );
	mPushDown->setParent( this );
	mPushDown->setSize( 16, 16 );

	mInput->getInputTextBuffer()->setAllowOnlyNumbers( true, false );

	internalValue( mValue, true );

	adjustChilds();

	applyDefaultTheme();
}

UISpinBox::~UISpinBox() {
}

Uint32 UISpinBox::getType() const {
	return UI_TYPE_SPINBOX;
}

bool UISpinBox::isType( const Uint32& type ) const {
	return UISpinBox::getType() == type ? true : UIWidget::isType( type );
}

void UISpinBox::setTheme( UITheme * Theme ) {
	UIWidget::setTheme( Theme );

	setThemeSkin( Theme, "spinbox" );

	mInput->setThemeSkin( Theme, "spinbox_input" );
	mPushUp->setThemeSkin( Theme, "spinbox_btnup" );
	mPushDown->setThemeSkin( Theme, "spinbox_btndown" );

	UISkin * tSkin = NULL;

	tSkin = mPushUp->getSkin();

	if ( NULL != tSkin ) {
		mPushUp->setSize( tSkin->getSize() );
	}

	tSkin = mPushDown->getSkin();

	if ( NULL != tSkin ) {
		mPushDown->setSize( tSkin->getSize() );
	}

	adjustChilds();
}

void UISpinBox::adjustChilds() {
	mInput->setSize( mDpSize.getWidth() - mPushUp->getSize().getWidth(), mInput->getSize().getHeight() );

	if ( mInput->getSize().getHeight() < mInput->getSkinSize().getHeight() ) {
		mInput->setSize( mDpSize.getWidth() - mPushUp->getSize().getWidth(), mInput->getSkinSize().getHeight() );
	}

	if ( ( mFlags & UI_AUTO_SIZE ) || mDpSize.getHeight() < mInput->getSize().getHeight() ) {
		setInternalHeight( mInput->getSkinSize().getHeight() );
	}

	mInput->centerVertical();

	mPushUp->setPosition( mDpSize.getWidth() - mPushUp->getSize().getWidth(), mInput->getPosition().y );
	mPushDown->setPosition( mDpSize.getWidth() - mPushDown->getSize().getWidth(), mInput->getPosition().y + mPushUp->getSize().getHeight() );
}

void UISpinBox::setPadding( const Rectf& padding ) {
	mInput->setPadding( padding );
}

const Rectf& UISpinBox::getPadding() const {
	return mInput->getPadding();
}

void UISpinBox::setClickStep( const Float& step ) {
	mClickStep = step;
}

const Float& UISpinBox::getClickStep() const {
	return mClickStep;
}

Uint32 UISpinBox::onMessage( const NodeMessage * Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::Click:
		{
			if ( Msg->getFlags() & EE_BUTTON_LMASK ) {
				if ( Msg->getSender() == mPushUp ) {
					addValue( mClickStep );
				} else if ( Msg->getSender() == mPushDown ) {
					addValue( -mClickStep );
				}
			} else if ( Msg->getFlags() & EE_BUTTONS_WUWD ) {
				if ( Msg->getFlags() & EE_BUTTON_WUMASK )
					addValue( mClickStep );
				else
					addValue( -mClickStep );
			}

			return 1;
		}
	}

	return 0;
}

void UISpinBox::addValue( const Float& value ) {
	if ( !mInput->getText().size() )
		mInput->setText( String::toStr( static_cast<Int32>( mMinValue ) ) );

	this->setValue( mValue + value );
}

void UISpinBox::internalValue( const Float& Val, const bool& Force ) {
	if ( Force || Val != mValue ) {
		if ( Val >= mMinValue && Val <= mMaxValue ) {
			Float iValN	= (Float)(Int32) Val;
			Float fValN 	= (Float)iValN;

			if ( fValN == Val ) {
				mInput->setText( String::toStr( iValN ) );
			} else {
				mInput->setText( String::toStr( Val ) );
			}

			mValue = Val;

			onValueChange();
		}
	}
}

void UISpinBox::onSizeChange() {
	UIWidget::onSizeChange();

	adjustChilds();
}

void UISpinBox::onPositionChange() {
	UIWidget::onPositionChange();

	adjustChilds();
}

UISpinBox * UISpinBox::setValue( const Float& Val ) {
	internalValue( Val, false );
	return this;
}

const Float& UISpinBox::getValue() const {
	return mValue;
}

UISpinBox * UISpinBox::setMinValue( const Float& MinVal ) {
	mMinValue = MinVal;

	if ( mValue < mMinValue )
		mValue = mMinValue;

	return this;
}

const Float& UISpinBox::getMinValue() const {
	return mMinValue;
}

UISpinBox * UISpinBox::setMaxValue( const Float& MaxVal ) {
	mMaxValue = MaxVal;

	if ( mValue > mMaxValue )
		mValue = mMaxValue;

	return this;
}

const Float& UISpinBox::getMaxValue() const {
	return mMaxValue;
}

void UISpinBox::update( const Time& time ) {
	bool Changed = mInput->getInputTextBuffer()->changedSinceLastUpdate();

	UINode::update( time );

	if ( Changed ) {
		if ( !mInput->getText().size() ) {
			setValue( 0 );
		} else {
			Float Val = mValue;

			if ( '.' == mInput->getText()[ mInput->getText().size() - 1 ] ) {
				Uint32 pos = (Uint32)mInput->getText().find_first_of( "." );

				if ( pos != mInput->getText().size() - 1 )
					mInput->setText( mInput->getText().substr( 0, mInput->getText().size() - 1 ) );
			} else {
				bool Res 	= String::fromString<Float>( Val, mInput->getText() );

				if ( Res )
					setValue( Val );
			}
		}
	}
}

UINode * UISpinBox::getButtonPushUp() const {
	return mPushUp;
}

UINode * UISpinBox::getButtonPushDown() const {
	return mPushDown;
}

UITextInput * UISpinBox::getTextInput() const {
	return mInput;
}

UISpinBox * UISpinBox::setAllowOnlyNumbers(bool allow) {
	mInput->getInputTextBuffer()->setAllowOnlyNumbers( true, allow );

	return this;
}

bool UISpinBox::dotsInNumbersAllowed() {
	return mInput->getInputTextBuffer()->dotsInNumbersAllowed();
}

void UISpinBox::onAlphaChange() {
	UINode::onAlphaChange();
	
	mInput->setAlpha( mAlpha );
	mPushUp->setAlpha( mAlpha );
	mPushDown->setAlpha( mAlpha );
}

void UISpinBox::loadFromXmlNode(const pugi::xml_node & node) {
	beginPropertiesTransaction();

	UIWidget::loadFromXmlNode( node );

	mInput->loadFromXmlNode( node );

	for (pugi::xml_attribute_iterator ait = node.attributes_begin(); ait != node.attributes_end(); ++ait) {
		std::string name = ait->name();
		String::toLowerInPlace( name );

		if ( "minvalue" == name ) {
			setMinValue( ait->as_float() );
		} else if ( "maxvalue" == name ) {
			setMaxValue( ait->as_float() );
		} else if ( "value" == name ) {
			setValue( ait->as_float() );
		} else if ( "clickstep" == name ) {
			setClickStep( ait->as_float() );
		}
	}

	endPropertiesTransaction();
}

}}
