#include <eepp/graphics/textureregion.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <limits>

namespace EE { namespace UI {

UISpinBox* UISpinBox::New() {
	return eeNew( UISpinBox, () );
}

UISpinBox::UISpinBox() :
	UIWidget( "spinbox" ),
	mMinValue( 0.f ),
	mMaxValue( std::numeric_limits<double>::max() ),
	mValue( 0 ),
	mClickStep( 1.f ),
	mModifyingVal( false ) {
	mFlags |= UI_SCROLLABLE;
	mInput = UITextInput::NewWithTag( "spinbox::input" );
	mInput->setVisible( true );
	mInput->setEnabled( true );
	mInput->setParent( this );

	auto cb = [this]( const Event* ) { adjustChilds(); };

	mPushUp = UIWidget::NewWithTag( "spinbox::btnup" );
	mPushUp->setVisible( true );
	mPushUp->setEnabled( true );
	mPushUp->setParent( this );
	mPushUp->setSize( 8, 8 );
	mPushUp->addEventListener( Event::OnSizeChange, cb );

	mPushDown = UIWidget::NewWithTag( "spinbox::btndown" );
	mPushDown->setVisible( true );
	mPushDown->setEnabled( true );
	mPushDown->setParent( this );
	mPushDown->setSize( 8, 8 );
	mPushDown->addEventListener( Event::OnSizeChange, cb );

	mInput->setAllowOnlyNumbers( true, false );
	mInput->addEventListener( Event::OnBufferChange,
							  [this]( auto event ) { onBufferChange( event ); } );
	double val = mValue;
	mValue += 1;
	setValue( val );

	applyDefaultTheme();
}

UISpinBox::~UISpinBox() {}

Uint32 UISpinBox::getType() const {
	return UI_TYPE_SPINBOX;
}

bool UISpinBox::isType( const Uint32& type ) const {
	return UISpinBox::getType() == type ? true : UIWidget::isType( type );
}

void UISpinBox::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	setThemeSkin( Theme, "spinbox" );

	mInput->setThemeSkin( Theme, "spinbox_input" );
	mPushUp->setThemeSkin( Theme, "spinbox_btnup" );
	mPushDown->setThemeSkin( Theme, "spinbox_btndown" );

	UISkin* tSkin = NULL;

	tSkin = mPushUp->getSkin();

	if ( NULL != tSkin ) {
		mPushUp->setSize( tSkin->getSize() );
	}

	tSkin = mPushDown->getSkin();

	if ( NULL != tSkin ) {
		mPushDown->setSize( tSkin->getSize() );
	}

	adjustChilds();
	onThemeLoaded();
}

void UISpinBox::adjustChilds() {
	mInput->setSize( getSize().getWidth() - mPushUp->getSize().getWidth(),
					 mInput->getSize().getHeight() );

	if ( mInput->getSize().getHeight() < mInput->getSkinSize().getHeight() ) {
		mInput->setSize( getSize().getWidth() - mPushUp->getSize().getWidth(),
						 mInput->getSkinSize().getHeight() );
	}

	if ( ( mFlags & UI_AUTO_SIZE ) || getSize().getHeight() < mInput->getSize().getHeight() ) {
		setInternalHeight( mInput->getSize().getHeight() );
	}

	mInput->centerVertical();

	int posY = ( getSize().getHeight() - mPushUp->getSize().getHeight() -
				 mPushDown->getSize().getHeight() ) /
			   2;

	mPushUp->setPosition( getSize().getWidth() - mPushUp->getSize().getWidth(), posY );
	mPushDown->setPosition( getSize().getWidth() - mPushDown->getSize().getWidth(),
							posY + mPushUp->getSize().getHeight() );
}

void UISpinBox::setPadding( const Rectf& padding ) {
	mInput->setPadding( padding );

	UIWidget::setPadding( padding );
}

const Rectf& UISpinBox::getPadding() const {
	return mInput->getPadding();
}

void UISpinBox::setClickStep( const double& step ) {
	mClickStep = step;
}

const double& UISpinBox::getClickStep() const {
	return mClickStep;
}

Uint32 UISpinBox::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::MouseClick: {
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

void UISpinBox::addValue( const double& value ) {
	if ( mInput->getText().empty() )
		mInput->setText( String::toString( static_cast<Int32>( mMinValue ) ) );

	setValue( mValue + value );
}

UISpinBox* UISpinBox::setValue( const double& val ) {
	double newVal = eeclamp( val, mMinValue, mMaxValue );
	double iValN = (double)(Int64)newVal;
	double fValN = (double)iValN;
	bool valueChanged = mValue != newVal;

	mValue = newVal;

	mModifyingVal = true;
	if ( fValN == newVal ) {
		mInput->setText( String::toString( (Int64)iValN ) );
	} else {
		mInput->setText( String::toString( newVal ) );
	}
	mModifyingVal = false;

	if ( valueChanged )
		onValueChange();
	return this;
}

void UISpinBox::onBufferChange( const Event* ) {
	if ( mModifyingVal )
		return;

	if ( mInput->getText().empty() ) {
		setValue( 0 );
	} else {
		double val = mValue;

		if ( '.' == mInput->getText()[mInput->getText().size() - 1] ) {
			Uint32 pos = (Uint32)mInput->getText().find_first_of( "." );

			if ( pos != mInput->getText().size() - 1 )
				mInput->setText( mInput->getText().substr( 0, mInput->getText().size() - 1 ) );
		} else {
			bool res = String::fromString<double>( val, mInput->getText() );
			if ( res && val != mValue )
				setValue( val );
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

const double& UISpinBox::getValue() const {
	return mValue;
}

UISpinBox* UISpinBox::setMinValue( const double& minVal ) {
	mMinValue = minVal;

	if ( mValue < mMinValue )
		setValue( mMinValue );

	return this;
}

const double& UISpinBox::getMinValue() const {
	return mMinValue;
}

UISpinBox* UISpinBox::setMaxValue( const double& maxVal ) {
	mMaxValue = maxVal;

	if ( mValue > mMaxValue )
		setValue( mMaxValue );

	return this;
}

const double& UISpinBox::getMaxValue() const {
	return mMaxValue;
}

UINode* UISpinBox::getButtonPushUp() const {
	return mPushUp;
}

UINode* UISpinBox::getButtonPushDown() const {
	return mPushDown;
}

UITextInput* UISpinBox::getTextInput() const {
	return mInput;
}

UISpinBox* UISpinBox::allowFloatingPoint( bool allow ) {
	mInput->setAllowOnlyNumbers( true, allow );

	return this;
}

bool UISpinBox::dotsInNumbersAllowed() {
	return mInput->floatingPointAllowed();
}

void UISpinBox::onAlphaChange() {
	UINode::onAlphaChange();

	mInput->setAlpha( mAlpha );
	mPushUp->setAlpha( mAlpha );
	mPushDown->setAlpha( mAlpha );
}

void UISpinBox::onPaddingChange() {
	adjustChilds();
	UIWidget::onPaddingChange();
}

std::string UISpinBox::getPropertyString( const PropertyDefinition* propertyDef,
										  const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::MinValue:
			return String::fromDouble( getMinValue() );
		case PropertyId::MaxValue:
			return String::fromDouble( getMaxValue() );
		case PropertyId::Value:
			return String::fromDouble( getValue() );
		case PropertyId::ClickStep:
			return String::fromDouble( getClickStep() );
		case PropertyId::Text:
		case PropertyId::AllowEditing:
		case PropertyId::MaxLength:
		case PropertyId::Numeric:
		case PropertyId::AllowFloat:
		case PropertyId::Hint:
		case PropertyId::HintColor:
		case PropertyId::HintShadowColor:
		case PropertyId::HintShadowOffset:
		case PropertyId::HintFontSize:
		case PropertyId::HintFontFamily:
		case PropertyId::HintFontStyle:
		case PropertyId::HintStrokeWidth:
		case PropertyId::HintStrokeColor:
		case PropertyId::HintDisplay:
		case PropertyId::Color:
		case PropertyId::TextShadowColor:
		case PropertyId::TextShadowOffset:
		case PropertyId::SelectionColor:
		case PropertyId::SelectionBackColor:
		case PropertyId::FontFamily:
		case PropertyId::FontSize:
		case PropertyId::FontStyle:
		case PropertyId::Wordwrap:
		case PropertyId::TextStrokeWidth:
		case PropertyId::TextStrokeColor:
		case PropertyId::TextSelection:
		case PropertyId::TextAlign:
			return mInput->getPropertyString( propertyDef, propertyIndex );
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UISpinBox::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	auto local = { PropertyId::MinValue, PropertyId::MaxValue, PropertyId::Value,
				   PropertyId::ClickStep };
	auto input = { PropertyId::Text,
				   PropertyId::AllowEditing,
				   PropertyId::MaxLength,
				   PropertyId::Numeric,
				   PropertyId::AllowFloat,
				   PropertyId::Hint,
				   PropertyId::HintColor,
				   PropertyId::HintShadowColor,
				   PropertyId::HintShadowOffset,
				   PropertyId::HintFontSize,
				   PropertyId::HintFontFamily,
				   PropertyId::HintFontStyle,
				   PropertyId::HintStrokeWidth,
				   PropertyId::HintStrokeColor,
				   PropertyId::HintDisplay,
				   PropertyId::Color,
				   PropertyId::TextShadowColor,
				   PropertyId::TextShadowOffset,
				   PropertyId::SelectionColor,
				   PropertyId::SelectionBackColor,
				   PropertyId::FontFamily,
				   PropertyId::FontSize,
				   PropertyId::FontStyle,
				   PropertyId::Wordwrap,
				   PropertyId::TextStrokeWidth,
				   PropertyId::TextStrokeColor,
				   PropertyId::TextSelection,
				   PropertyId::TextAlign };
	props.insert( props.end(), local.begin(), local.end() );
	props.insert( props.end(), input.begin(), input.end() );
	return props;
}

bool UISpinBox::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	bool attributeSet = true;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
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
		case PropertyId::Text:
		case PropertyId::AllowEditing:
		case PropertyId::MaxLength:
		case PropertyId::Numeric:
		case PropertyId::AllowFloat:
		case PropertyId::Hint:
		case PropertyId::HintColor:
		case PropertyId::HintShadowColor:
		case PropertyId::HintShadowOffset:
		case PropertyId::HintFontSize:
		case PropertyId::HintFontFamily:
		case PropertyId::HintFontStyle:
		case PropertyId::HintStrokeWidth:
		case PropertyId::HintStrokeColor:
		case PropertyId::HintDisplay:
		case PropertyId::Color:
		case PropertyId::TextShadowColor:
		case PropertyId::TextShadowOffset:
		case PropertyId::SelectionColor:
		case PropertyId::SelectionBackColor:
		case PropertyId::FontFamily:
		case PropertyId::FontSize:
		case PropertyId::FontStyle:
		case PropertyId::Wordwrap:
		case PropertyId::TextStrokeWidth:
		case PropertyId::TextStrokeColor:
		case PropertyId::TextSelection:
		case PropertyId::TextAlign:
			return mInput->applyProperty( attribute );
		default:
			attributeSet = UIWidget::applyProperty( attribute );
			break;
	}

	return attributeSet;
}

}} // namespace EE::UI
