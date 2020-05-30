#include <algorithm>
#include <eepp/core.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/css/propertyspecification.hpp>

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
	mIndexed( false ),
	mRelativeTarget( PropertyRelativeTarget::None ),
	mPropertyType( PropertyType::Undefined ) {}

const std::string& PropertyDefinition::getName() const {
	return mName;
}

const String::HashType& PropertyDefinition::getId() const {
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

PropertyDefinition&
PropertyDefinition::setRelativeTarget( const PropertyRelativeTarget& relativeTarget ) {
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
	String::HashType aliasId = String::hash( alias );
	mAliases.push_back( alias );
	mAliasesHash.push_back( aliasId );
	PropertySpecification::instance()->addPropertyAlias( aliasId, this );
	return *this;
}

bool PropertyDefinition::isAlias( const std::string& alias ) const {
	return isAlias( String::hash( alias ) );
}

bool PropertyDefinition::isAlias( const Uint32& id ) const {
	size_t size = mAliasesHash.size();
	for ( size_t i = 0; i < size; i++ ) {
		if ( mAliasesHash[i] == id )
			return true;
	}
	return false;
}

bool PropertyDefinition::isDefinition( const std::string& name ) const {
	return isDefinition( String::hash( name ) );
}

bool PropertyDefinition::isDefinition( const Uint32& id ) const {
	return mId == id || isAlias( id );
}

PropertyDefinition& PropertyDefinition::setIndexed() {
	mIndexed = true;
	return *this;
}

const bool& PropertyDefinition::isIndexed() const {
	return mIndexed;
}

}}} // namespace EE::UI::CSS
