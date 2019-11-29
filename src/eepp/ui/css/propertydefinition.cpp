#include <algorithm>
#include <eepp/core.hpp>
#include <eepp/ui/css/propertydefinition.hpp>

namespace EE { namespace UI { namespace CSS {

PropertyDefinition* PropertyDefinition::New( const std::string& name,
											 const std::string& defaultValue,
											 const bool& inherited ) {
	return eeNew( PropertyDefinition, ( name, defaultValue, inherited ) );
}

PropertyDefinition::PropertyDefinition( const std::string& name, const std::string& defaultValue,
										const bool& inherited ) :
	mName( name ),
	mId( String::hash( name ) ),
	mDefaultValue( defaultValue ),
	mInherited( inherited ),
	mPropertyType( PropertyType::Undefined ) {

	for ( auto& sep : {"-", "_"} ) {
		if ( mName.find( sep ) != std::string::npos ) {
			std::string alias( name );
			String::replaceAll( alias, sep, "" );
			addAlias( alias );
		}
	}
}

const std::string& PropertyDefinition::getName() const {
	return mName;
}

const Uint32& PropertyDefinition::getId() const {
	return mId;
}

PropertyId PropertyDefinition::getPropertyId() const {
	return static_cast<PropertyId>( mId );
}

const std::string& PropertyDefinition::getDefaultValue() const {
	return mDefaultValue;
}

bool PropertyDefinition::getInherited() const {
	return mInherited;
}

const PropertyRelativeTarget& PropertyDefinition::getRelativeTarget() const {
	return mRelativeTarget;
}

PropertyDefinition& PropertyDefinition::setRelativeTarget( const PropertyRelativeTarget& relativeTarget ) {
	mRelativeTarget = relativeTarget;
	return *this;
}

PropertyDefinition& PropertyDefinition::setType( const PropertyType& propertyType ) {
	mPropertyType = propertyType;
	return *this;
}

const PropertyType& PropertyDefinition::getType() const {
	return mPropertyType;
}

PropertyDefinition& PropertyDefinition::addAlias( const std::string& alias ) {
	mAliases.push_back( alias );
	mAliasesHash.push_back( String::hash( alias ) );
	return *this;
}

bool PropertyDefinition::isAlias( const std::string& alias ) const {
	return isAlias( String::hash( alias ) );
}

bool PropertyDefinition::isAlias( const Uint32& id ) const {
	return std::find( mAliasesHash.begin(), mAliasesHash.end(), id ) != mAliasesHash.end();
}

bool PropertyDefinition::isDefinition( const std::string& name ) const {
	return isDefinition( String::hash( name ) );
}

bool PropertyDefinition::isDefinition( const Uint32& id ) const {
	return mId == id || isAlias( id );
}

}}} // namespace EE::UI::CSS
