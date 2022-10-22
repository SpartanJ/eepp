#include <eepp/ui/uiroot.hpp>

namespace EE { namespace UI {

UIRoot* UIRoot::New() {
	return eeNew( UIRoot, () );
}

UIRoot::UIRoot() : UIWidget( ":root" ) {}

std::string UIRoot::getPropertyString( const PropertyDefinition* propertyDef,
									   const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::DroppableHoveringColor:
			return mDroppableHoveringColor.toHexString();
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

bool UIRoot::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::DroppableHoveringColor:
			mDroppableHoveringColor = attribute.asColor();
			break;
		default:
			return UIWidget::applyProperty( attribute );
	}

	return true;
}

std::vector<PropertyId> UIRoot::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	props.push_back( PropertyId::DroppableHoveringColor );
	return props;
}

}} // namespace EE::UI
