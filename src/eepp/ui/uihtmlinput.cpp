#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uihelper.hpp>
#include <eepp/ui/uihtmlinput.hpp>
#include <eepp/ui/uihtmltextinput.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiradiobutton.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitextinput.hpp>

namespace EE { namespace UI {

UIHTMLInput* UIHTMLInput::New() {
	return eeNew( UIHTMLInput, () );
}

UIHTMLInput::UIHTMLInput() : UIWidget( "input" ) {
	mFlags |= UI_HTML_ELEMENT;
	mWidthPolicy = SizePolicy::WrapContent;
	mHeightPolicy = SizePolicy::WrapContent;
	createChildWidget();
}

Uint32 UIHTMLInput::getType() const {
	return UI_TYPE_HTML_INPUT;
}

bool UIHTMLInput::isType( const Uint32& type ) const {
	return UIHTMLInput::getType() == type || UIWidget::isType( type );
}

bool UIHTMLInput::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !attribute.getPropertyDefinition() )
		return false;

	PropertyId id = attribute.getPropertyDefinition()->getPropertyId();

	switch ( id ) {
		case PropertyId::Value:
		case PropertyId::Text:
			mValue = attribute.value();
			break;
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

std::string UIHTMLInput::getPropertyString( const PropertyDefinition* propertyDef,
											const Uint32& propertyIndex ) const {
	if ( !propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Value:
			return mValue;
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

std::vector<PropertyId> UIHTMLInput::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	props.push_back( PropertyId::Type );
	props.push_back( PropertyId::Value );
	return props;
}

Float UIHTMLInput::getMinIntrinsicWidth() const {
	return mChildWidget ? mChildWidget->getMinIntrinsicWidth() : 0;
}

Float UIHTMLInput::getMaxIntrinsicWidth() const {
	return mChildWidget ? mChildWidget->getMaxIntrinsicWidth() : 0;
}

const std::string& UIHTMLInput::getInputType() const {
	return mInputType;
}

void UIHTMLInput::setInputType( const std::string& type ) {
	if ( mInputType != type ) {
		mInputType = type;
		createChildWidget();
	}
}

UIWidget* UIHTMLInput::getChildWidget() const {
	return mChildWidget;
}

void UIHTMLInput::createChildWidget() {
	if ( mChildWidget ) {
		mChildWidget->close();
		mChildWidget = nullptr;
	}

	if ( mInputType == "button" || mInputType == "submit" ) {
		mChildWidget = UIPushButton::New();
	} else if ( mInputType == "checkbox" ) {
		mChildWidget = UICheckBox::New();
	} else if ( mInputType == "hidden" ) {
		// We don't need it
	} else if ( mInputType == "number" ) {
		mChildWidget = UISpinBox::New();
	} else if ( mInputType == "password" ) {
		mChildWidget = UIHTMLTextInput::New()->setMode( UITextInput::TextInputMode::Password );
	} else if ( mInputType == "radio" ) {
		mChildWidget = UIRadioButton::New();
	} else {
		mChildWidget = UIHTMLTextInput::New();
	}

	if ( mChildWidget == nullptr )
		return;

	mChildWidget->setFlags( UI_HTML_ELEMENT );

	if ( mChildWidget ) {
		mChildWidget->setParent( this );
		mChildWidget->setLayoutWidthPolicy( SizePolicy::WrapContent );
		mChildWidget->setLayoutHeightPolicy( SizePolicy::WrapContent );
		mChildWidget->on( Event::OnSizeChange, [this]( auto ) {
			if ( mChildWidget )
				setPixelsSize( mChildWidget->getPixelsSize() );
		} );
		for ( const auto& propIt : mProperties ) {
			mChildWidget->applyProperty( propIt.second );
		}
	}
}

String UIHTMLInput::getFormValue() const {
	if ( !mChildWidget )
		return String();

	if ( mInputType == "checkbox" )
		return static_cast<UICheckBox*>( mChildWidget )->isChecked() ? "on" : "";
	if ( mInputType == "radio" )
		return static_cast<UIRadioButton*>( mChildWidget )->isActive() ? "on" : "";
	if ( mInputType == "number" )
		return static_cast<UISpinBox*>( mChildWidget )->getTextInput()->getText();
	if ( mInputType == "button" || mInputType == "submit" )
		return static_cast<UIPushButton*>( mChildWidget )->getText();

	if ( mChildWidget->isType( UI_TYPE_TEXTINPUT ) )
		return static_cast<UITextInput*>( mChildWidget )->getText();

	return mValue;
}

void UIHTMLInput::onSizeChange() {
	UIWidget::onSizeChange();
}

}} // namespace EE::UI
