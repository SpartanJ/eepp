#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitextnode.hpp>

namespace EE { namespace UI {

UITextNode* UITextNode::New() {
	return eeNew( UITextNode, () );
}

UITextNode::UITextNode() : UIWidget( "textnode" ) {
	mNodeFlags |= NODE_FLAG_TEXTNODE;
	mFlags |= UI_HTML_ELEMENT;
}

UITextNode::~UITextNode() {}

Uint32 UITextNode::getType() const {
	return UI_TYPE_TEXTNODE;
}

bool UITextNode::isType( const Uint32& type ) const {
	return UITextNode::getType() == type ? true : UIWidget::isType( type );
}

void UITextNode::draw() {
	// Text nodes do not draw themselves; their parent handles rendering
}

std::string UITextNode::getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	const StyleSheetProperty* prop = getUIStyle()->getProperty( propertyDef->getPropertyId() );
	if ( prop )
		return prop->value();

	if ( propertyDef->isInherited() && getParent() && getParent()->isWidget() )
		return static_cast<UIWidget*>( getParent() )
			->getPropertyString( propertyDef, propertyIndex );

	return UIWidget::getPropertyString( propertyDef, propertyIndex );
}

const String& UITextNode::getText() const {
	return mText;
}

void UITextNode::setText( const String& text ) {
	if ( mText != text ) {
		mText = text;
		notifyLayoutAttrChange();
	}
}

}} // namespace EE::UI
