#include <eepp/core/debug.hpp>
#include <eepp/ui/css/propertyspecification.hpp>

namespace EE { namespace UI { namespace CSS {

PropertyDefinition& PropertySpecification::registerProperty( const std::string& propertyVame,
															 const std::string& defaultValue,
															 bool inherited ) {
	PropertyDefinition* property = const_cast<PropertyDefinition*>( getProperty( propertyVame ) );

	if ( NULL != property ) {
		eePRINTL( "Property %s already registered.", propertyVame );
		return *property;
	}

	mProperties.emplace_back( std::unique_ptr<PropertyDefinition>(
		PropertyDefinition::New( propertyVame, defaultValue, inherited ) ) );

	return *mProperties.back().get();
}

const PropertyDefinition* PropertySpecification::getProperty( const Uint32& id ) const {
	for ( auto& property : mProperties ) {
		if ( property->isDefinition( id ) ) {
			return property.get();
		}
	}

	return NULL;
}

const PropertyDefinition* PropertySpecification::getProperty( const std::string& name ) const {
	return getProperty( String::hash( name ) );
}

}}} // namespace EE::UI::CSS
