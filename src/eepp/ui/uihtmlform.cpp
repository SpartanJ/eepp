#include <eepp/network/http.hpp>
#include <eepp/network/uri.hpp>
#include <eepp/scene/nodemessage.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/uihtmlform.hpp>
#include <eepp/ui/uihtmlinput.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitextedit.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

UIHTMLForm* UIHTMLForm::New() {
	return eeNew( UIHTMLForm, () );
}

UIHTMLForm::UIHTMLForm( const std::string& tag ) : UIRichText( tag ) {}

Uint32 UIHTMLForm::getType() const {
	return static_cast<Uint32>( UI_TYPE_HTML_FORM );
}

bool UIHTMLForm::isType( const Uint32& type ) const {
	return UI_TYPE_HTML_FORM == type ? true : UIRichText::isType( type );
}

bool UIHTMLForm::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !attribute.getPropertyDefinition() )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Action:
			mAction = attribute.value();
			return true;
		case PropertyId::Method:
			mMethod = attribute.value();
			return true;
		case PropertyId::Enctype:
			mEnctype = attribute.value();
			return true;
		default:
			break;
	}

	return UIRichText::applyProperty( attribute );
}

std::string UIHTMLForm::getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex ) const {
	if ( !propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Action:
			return mAction;
		case PropertyId::Method:
			return mMethod;
		case PropertyId::Enctype:
			return mEnctype;
		default:
			break;
	}

	return UIRichText::getPropertyString( propertyDef, propertyIndex );
}

std::vector<PropertyId> UIHTMLForm::getPropertiesImplemented() const {
	auto props = UIRichText::getPropertiesImplemented();
	props.push_back( PropertyId::Action );
	props.push_back( PropertyId::Method );
	props.push_back( PropertyId::Enctype );
	return props;
}

void UIHTMLForm::submit() {
	std::vector<std::pair<std::string, std::string>> fields;
	collectFormData( this, fields );

	UISceneNode* sceneNode = getUISceneNode();

	NavigationRequest request;
	request.uri = URI( mAction );

	if ( mEnctype == "multipart/form-data" ) {
		request.method = "POST";
		Http::MultipartEntitiesBuilder builder;
		for ( auto& field : fields )
			builder.addParameter( field.first, field.second );
		request.body = builder.build();
		request.extraHeaders["Content-Type"] = builder.getContentType();
	} else if ( mEnctype == "text/plain" ) {
		request.method = "POST";
		for ( size_t i = 0; i < fields.size(); i++ ) {
			if ( i > 0 )
				request.body += "\r\n";
			request.body += fields[i].first + "=" + fields[i].second;
		}
		request.extraHeaders["Content-Type"] = mEnctype;
	} else {
		std::string queryString;
		for ( size_t i = 0; i < fields.size(); i++ ) {
			if ( i > 0 )
				queryString += "&";
			queryString += URI::encode( fields[i].first ) + "=" + URI::encode( fields[i].second );
		}

		if ( mMethod == "GET" ) {
			if ( !queryString.empty() ) {
				std::string existingQuery = request.uri.getQuery();
				if ( existingQuery.empty() )
					request.uri.setQuery( queryString );
				else
					request.uri.setQuery( existingQuery + "&" + queryString );
				request.uri.setRawQuery( queryString );
			}
		} else {
			request.method = "POST";
			request.body = queryString;
			request.extraHeaders["Content-Type"] = mEnctype;
		}
	}

	sceneNode->navigate( request );
}

static String getWidgetFormValue( UIWidget* widget ) {
	if ( widget->isType( UI_TYPE_HTML_WIDGET ) )
		return static_cast<UIHTMLWidget*>( widget )->getFormValue();
	if ( widget->isType( UI_TYPE_HTML_INPUT ) )
		return static_cast<UIHTMLInput*>( widget )->getFormValue();
	if ( widget->isType( UI_TYPE_TEXTINPUT ) )
		return static_cast<UITextInput*>( widget )->getText();
	if ( widget->isType( UI_TYPE_TEXTEDIT ) )
		return static_cast<UITextEdit*>( widget )->getText();
	return String();
}

void UIHTMLForm::collectFormData( Node* node,
								  std::vector<std::pair<std::string, std::string>>& fields ) {
	if ( !node )
		return;

	UIWidget* widget = node->asType<UIWidget>();
	if ( widget ) {
		std::string name;
		UIStyle* style = widget->getUIStyle();
		if ( style ) {
			const CSS::StyleSheetProperty* prop = style->getProperty( PropertyId::Name );
			if ( prop )
				name = prop->value();
		}
		if ( name.empty() )
			name = widget->getPropertyString( "name" );
		if ( !name.empty() ) {
			String value = getWidgetFormValue( widget );
			if ( !value.empty() )
				fields.emplace_back( name, value.toUtf8() );
		}
	}

	Node* child = node->getFirstChild();
	while ( child ) {
		collectFormData( child, fields );
		child = child->getNextNode();
	}
}

Uint32 UIHTMLForm::onMessage( const NodeMessage* msg ) {
	if ( msg->getMsg() == NodeMessage::MouseClick && isSubmitTrigger( msg->getSender() ) ) {
		submit();
		return 1;
	}
	return UIRichText::onMessage( msg );
}

bool UIHTMLForm::isSubmitTrigger( Node* sender ) const {
	while ( sender ) {
		if ( sender->isWidget() ) {
			auto* widget = static_cast<UIWidget*>( sender );
			if ( widget->isType( UI_TYPE_HTML_INPUT ) &&
				 static_cast<UIHTMLInput*>( widget )->getInputType() == "submit" )
				return true;
			if ( widget->isType( UI_TYPE_PUSHBUTTON ) &&
				 widget->getPropertyString( "type" ) == "submit" )
				return true;
		}
		sender = sender->getParent();
	}
	return false;
}

}} // namespace EE::UI
