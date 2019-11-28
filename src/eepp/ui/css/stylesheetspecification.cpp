#include <eepp/ui/css/stylesheetspecification.hpp>

namespace EE { namespace UI { namespace CSS {

SINGLETON_DECLARE_IMPLEMENTATION( StyleSheetSpecification )

StyleSheetSpecification::StyleSheetSpecification() {
	registerDefaultProperties();
}

StyleSheetSpecification::~StyleSheetSpecification() {}

PropertyDefinition& StyleSheetSpecification::registerProperty( const std::string& propertyVame,
															   const std::string& defaultValue,
															   bool inherited ) {
	return mPropertySpecification.registerProperty( propertyVame, defaultValue, inherited );
}

const PropertyDefinition* StyleSheetSpecification::getProperty( const Uint32& id ) const {
	return mPropertySpecification.getProperty( id );
}

const PropertyDefinition* StyleSheetSpecification::getProperty( const std::string& name ) const {
	return mPropertySpecification.getProperty( name );
}

void StyleSheetSpecification::registerDefaultProperties() {
	registerProperty( "id", "", false );
	registerProperty( "class", "", false );
	registerProperty( "x", "", false );
	registerProperty( "y", "", false );
	registerProperty( "margin-top", "0px", false );
	registerProperty( "margin-left", "0px", false );
	registerProperty( "margin-right", "0px", false );
	registerProperty( "margin-bottom", "0px", false );
}

}}} // namespace EE::UI::CSS
