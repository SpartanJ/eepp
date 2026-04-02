#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/htmlinput.hpp>
#include <eepp/ui/htmltextinput.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiradiobutton.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitextinputpassword.hpp>

namespace EE { namespace UI {

HTMLInput* HTMLInput::New() {
	return eeNew( HTMLInput, () );
}

HTMLInput::HTMLInput() : UIWidget( "input" ) {
	mWidthPolicy = SizePolicy::WrapContent;
	mHeightPolicy = SizePolicy::WrapContent;
	createChildWidget();
}

Uint32 HTMLInput::getType() const {
	return UI_TYPE_HTML_INPUT;
}

bool HTMLInput::isType( const Uint32& type ) const {
	return HTMLInput::getType() == type || UIWidget::isType( type );
}

bool HTMLInput::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !attribute.getPropertyDefinition() )
		return false;

	PropertyId id = attribute.getPropertyDefinition()->getPropertyId();

	switch ( id ) {
		case PropertyId::Type:
			setInputType( attribute.value() );
			return true;
		default:
			break;
	}

	if ( id != PropertyId::Id && id != PropertyId::Class && id != PropertyId::Type ) {
		mProperties[id] = attribute;
		if ( mChildWidget ) {
			mChildWidget->applyProperty( attribute );
		}
	}

	return UIWidget::applyProperty( attribute );
}

std::string HTMLInput::getPropertyString( const PropertyDefinition* propertyDef,
										  const Uint32& propertyIndex ) const {
	if ( !propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Type:
			return mInputType;
		default:
			break;
	}

	if ( mChildWidget ) {
		std::string val = mChildWidget->getPropertyString( propertyDef, propertyIndex );
		if ( !val.empty() )
			return val;
	}

	return UIWidget::getPropertyString( propertyDef, propertyIndex );
}

std::vector<PropertyId> HTMLInput::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	props.push_back( PropertyId::Type );
	return props;
}

Float HTMLInput::getMinIntrinsicWidth() const {
	return mChildWidget ? mChildWidget->getMinIntrinsicWidth() : 0;
}

Float HTMLInput::getMaxIntrinsicWidth() const {
	return mChildWidget ? mChildWidget->getMaxIntrinsicWidth() : 0;
}

const std::string& HTMLInput::getInputType() const {
	return mInputType;
}

void HTMLInput::setInputType( const std::string& type ) {
	if ( mInputType != type ) {
		mInputType = type;
		createChildWidget();
	}
}

UIWidget* HTMLInput::getChildWidget() const {
	return mChildWidget;
}

void HTMLInput::createChildWidget() {
	if ( mChildWidget ) {
		mChildWidget->close();
		mChildWidget = nullptr;
	}

	if ( mInputType == "button" || mInputType == "submit" ) {
		mChildWidget = UIPushButton::New();
	} else if ( mInputType == "checkbox" ) {
		mChildWidget = UICheckBox::New();
	} else if ( mInputType == "hidden" ) {
		mChildWidget = UIWidget::New();
		mChildWidget->setVisible( false );
	} else if ( mInputType == "number" ) {
		mChildWidget = UISpinBox::New();
	} else if ( mInputType == "password" ) {
		mChildWidget = UITextInputPassword::New();
	} else if ( mInputType == "radio" ) {
		mChildWidget = UIRadioButton::New();
	} else {
		mChildWidget = HTMLTextInput::New();
	}

	if ( mChildWidget ) {
		mChildWidget->setParent( this );
		mChildWidget->setLayoutWidthPolicy( SizePolicy::WrapContent );
		mChildWidget->setLayoutHeightPolicy( SizePolicy::WrapContent );
		mChildWidget->on( Event::OnSizeChange,
						  [this]( auto ) { setPixelsSize( mChildWidget->getPixelsSize() ); } );
		for ( const auto& propIt : mProperties ) {
			mChildWidget->applyProperty( propIt.second );
		}
	}
}

void HTMLInput::onSizeChange() {
	UIWidget::onSizeChange();
}

}} // namespace EE::UI
