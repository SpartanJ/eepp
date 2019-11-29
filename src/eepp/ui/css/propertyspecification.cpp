#include <eepp/core/core.hpp>
#include <eepp/ui/css/propertyspecification.hpp>

namespace EE { namespace UI { namespace CSS {

PropertySpecification::~PropertySpecification() {
	for ( std::size_t i = 0; i < mProperties.size(); i++ ) {
		eeSAFE_DELETE( mProperties[i] );
	}

	for ( std::size_t i = 0; i < mShorthands.size(); i++ ) {
		eeSAFE_DELETE( mShorthands[i] );
	}
}

PropertyDefinition& PropertySpecification::registerProperty( const std::string& propertyVame,
															 const std::string& defaultValue,
															 bool inherited ) {
	PropertyDefinition* property = const_cast<PropertyDefinition*>( getProperty( propertyVame ) );

	if ( NULL != property ) {
		eePRINTL( "Property %s already registered.", propertyVame.c_str() );
		return *property;
	}

	mProperties.emplace_back( PropertyDefinition::New( propertyVame, defaultValue, inherited ) );

	return *mProperties.back();
}

const PropertyDefinition* PropertySpecification::getProperty( const Uint32& id ) const {
	for ( auto& property : mProperties ) {
		if ( property->isDefinition( id ) ) {
			return property;
		}
	}

	return NULL;
}

const PropertyDefinition* PropertySpecification::getProperty( const std::string& name ) const {
	return getProperty( String::hash( name ) );
}

ShorthandDefinition& PropertySpecification::registerShorthand(
	const std::string& name, const std::vector<std::string>& properties,
	const ShorthandDefinition::ShorthandType& shorthandType ) {
	ShorthandDefinition* shorthand = const_cast<ShorthandDefinition*>( getShorthand( name ) );

	if ( NULL != shorthand ) {
		eePRINTL( "Shorthand %s already registered.", name.c_str() );
		return *shorthand;
	}

	mShorthands.emplace_back( ShorthandDefinition::New( name, properties, shorthandType ) );

	return *mShorthands.back();
}

const ShorthandDefinition* PropertySpecification::getShorthand( const Uint32& id ) const {
	for ( auto& shorthand : mShorthands ) {
		if ( shorthand->getId() == id ) {
			return shorthand;
		}
	}

	return NULL;
}

const ShorthandDefinition* PropertySpecification::getShorthand( const std::string& name ) const {
	return getShorthand( String::hash( name ) );
}

bool PropertySpecification::isShorthand( const std::string& name ) const {
	return getShorthand( name ) != NULL;
}

bool PropertySpecification::isShorthand( const Uint32& id ) const {
	return getShorthand( id ) != NULL;
}

}}} // namespace EE::UI::CSS
