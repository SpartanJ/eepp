#include <algorithm>
#include <eepp/core.hpp>
#include <eepp/system/color.hpp>
#include <eepp/ui/css/shorthanddefinition.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>

using namespace EE::System;

namespace EE { namespace UI { namespace CSS {

ShorthandDefinition* ShorthandDefinition::New( const std::string& name,
											   const std::vector<std::string>& properties,
											   const std::string& shorthandParserName ) {
	return eeNew( ShorthandDefinition, ( name, properties, shorthandParserName ) );
}

ShorthandDefinition::ShorthandDefinition( const std::string& name,
										  const std::vector<std::string>& properties,
										  const std::string& shorthandParserName ) :
	mName( name ),
	mFuncName( shorthandParserName ),
	mId( String::hash( name ) ),
	mProperties( properties ) {
	for ( auto& sep : {"-", "_"} ) {
		if ( mName.find( sep ) != std::string::npos ) {
			std::string alias( name );
			String::replaceAll( alias, sep, "" );
			addAlias( alias );
		}
	}
}

std::vector<StyleSheetProperty> ShorthandDefinition::parse( std::string value ) const {
	return StyleSheetSpecification::instance()->getShorthandParser( mFuncName )( this, value );
}

const std::string& ShorthandDefinition::getName() const {
	return mName;
}

const String::HashType& ShorthandDefinition::getId() const {
	return mId;
}

ShorthandId ShorthandDefinition::getShorthandId() const {
	return static_cast<ShorthandId>( mId );
}

const std::vector<std::string>& ShorthandDefinition::getProperties() const {
	return mProperties;
}

ShorthandDefinition& ShorthandDefinition::addAlias( const std::string& alias ) {
	mAliases.push_back( alias );
	mAliasesHash.push_back( String::hash( alias ) );
	return *this;
}

bool ShorthandDefinition::isAlias( const std::string& alias ) const {
	return isAlias( String::hash( alias ) );
}

bool ShorthandDefinition::isAlias( const String::HashType& id ) const {
	return std::find( mAliasesHash.begin(), mAliasesHash.end(), id ) != mAliasesHash.end();
}

bool ShorthandDefinition::isDefinition( const std::string& name ) const {
	return isDefinition( String::hash( name ) );
}

bool ShorthandDefinition::isDefinition( const String::HashType& id ) const {
	return mId == id || isAlias( id );
}

}}} // namespace EE::UI::CSS
