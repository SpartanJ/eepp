#include <algorithm>
#include <eepp/core.hpp>
#include <eepp/system/color.hpp>
#include <eepp/ui/css/shorthanddefinition.hpp>

using namespace EE::System;

namespace EE { namespace UI { namespace CSS {

ShorthandDefinition* ShorthandDefinition::New( const std::string& name,
											   const std::vector<std::string>& properties,
											   const ShorthandType& shorthandType ) {
	return eeNew( ShorthandDefinition, ( name, properties, shorthandType ) );
}

ShorthandDefinition::ShorthandDefinition( const std::string& name,
										  const std::vector<std::string>& properties,
										  const ShorthandType& shorthandType ) :
	mName( name ), mId( String::hash( name ) ), mProperties( properties ), mType( shorthandType ) {
	for ( auto& sep : {"-", "_"} ) {
		if ( mName.find( sep ) != std::string::npos ) {
			std::string alias( name );
			String::replaceAll( alias, sep, "" );
			addAlias( alias );
		}
	}
}

const std::string& ShorthandDefinition::getName() const {
	return mName;
}

const Uint32& ShorthandDefinition::getId() const {
	return mId;
}

ShorthandId ShorthandDefinition::getShorthandId() const {
	return static_cast<ShorthandId>( mId );
}

const std::vector<std::string>& ShorthandDefinition::getProperties() const {
	return mProperties;
}

const ShorthandType& ShorthandDefinition::getType() const {
	return mType;
}

static int getIndexEndingWith( const std::vector<std::string>& vec, const std::string& endWidth ) {
	for ( size_t i = 0; i < vec.size(); i++ ) {
		if ( String::endsWith( vec[i], endWidth ) ) {
			return i;
		}
	}

	return -1;
}

ShorthandDefinition& ShorthandDefinition::addAlias( const std::string& alias ) {
	mAliases.push_back( alias );
	mAliasesHash.push_back( String::hash( alias ) );
	return *this;
}

bool ShorthandDefinition::isAlias( const std::string& alias ) const {
	return isAlias( String::hash( alias ) );
}

bool ShorthandDefinition::isAlias( const Uint32& id ) const {
	return std::find( mAliasesHash.begin(), mAliasesHash.end(), id ) != mAliasesHash.end();
}

bool ShorthandDefinition::isDefinition( const std::string& name ) const {
	return isDefinition( String::hash( name ) );
}

bool ShorthandDefinition::isDefinition( const Uint32& id ) const {
	return mId == id || isAlias( id );
}

std::vector<StyleSheetProperty>
ShorthandDefinition::parseShorthand( const ShorthandDefinition* shorthand, std::string value ) {
	value = String::trim( value );
	std::vector<StyleSheetProperty> properties;
	const std::vector<std::string> propNames( shorthand->getProperties() );

	switch ( shorthand->getType() ) {
		case ShorthandType::Box: {
			if ( propNames.size() != 4 ) {
				eePRINTL( "ShorthandType::Box properties must be 4 for %s",
						  shorthand->getName().c_str() );
				return properties;
			}

			auto ltrbSplit = String::split( value, ' ', true );

			if ( ltrbSplit.size() >= 2 ) {
				for ( size_t i = 0; i < ltrbSplit.size(); i++ )
					properties.emplace_back( StyleSheetProperty( propNames[i], ltrbSplit[i] ) );
			} else if ( ltrbSplit.size() == 1 ) {
				for ( size_t i = 0; i < propNames.size(); i++ )
					properties.emplace_back( StyleSheetProperty( propNames[i], ltrbSplit[0] ) );
			}

			break;
		}
		case ShorthandType::Background: {
			if ( Color::isColorString( value ) ) {
				int pos = getIndexEndingWith( propNames, "-color" );
				if ( pos != -1 )
					properties.emplace_back( StyleSheetProperty( propNames[pos], value ) );
			} else {
				int pos = getIndexEndingWith( propNames, "-image" );
				if ( pos != -1 )
					properties.emplace_back( StyleSheetProperty( propNames[pos], value ) );
			}
			break;
		}
		case ShorthandType::SingleValueVector: {
			for ( auto& prop : propNames ) {
				properties.emplace_back( StyleSheetProperty( prop, value ) );
			}
			break;
		}
		case ShorthandType::Vector2: {
			if ( propNames.size() != 2 ) {
				eePRINTL( "ShorthandType::Vector2 properties must be 2 for %s",
						  shorthand->getName().c_str() );
				return properties;
			}

			auto values = String::split( value, ' ' );

			if ( !values.empty() ) {
				for ( size_t i = 0; i < propNames.size(); i++ ) {
					properties.emplace_back(
						StyleSheetProperty( propNames[0], values[i % values.size()] ) );
				}
			}
			break;
		}
		case ShorthandType::BackgroundPosition: {
			std::vector<std::string> values = String::split( value, ',' );
			std::map<std::string, std::vector<std::string>> tmpProperties;

			for ( auto& val : values ) {
				std::vector<std::string> pos = String::split( val, ' ' );

				if ( pos.size() == 1 )
					pos.push_back( "center" );

				if ( pos.size() == 2 ) {
					int xFloatIndex = 0;
					int yFloatIndex = 1;

					if ( "bottom" == pos[0] || "top" == pos[0] ) {
						xFloatIndex = 1;
						yFloatIndex = 0;
					}

					tmpProperties[propNames[0]].emplace_back( pos[xFloatIndex] );
					tmpProperties[propNames[1]].emplace_back( pos[yFloatIndex] );
				} else if ( pos.size() > 2 ) {
					if ( pos.size() == 3 ) {
						pos.push_back( "0dp" );
					}

					int xFloatIndex = 0;
					int yFloatIndex = 2;

					if ( "bottom" == pos[0] || "top" == pos[0] ) {
						xFloatIndex = 2;
						yFloatIndex = 0;
					}

					tmpProperties[propNames[0]].emplace_back( pos[xFloatIndex] + " " +
															  pos[xFloatIndex + 1] );
					tmpProperties[propNames[1]].emplace_back( pos[yFloatIndex] + " " +
															  pos[yFloatIndex + 1] );
				}
			}

			for ( auto& props : tmpProperties ) {
				properties.push_back(
					StyleSheetProperty( props.first, String::join( props.second, ',' ) ) );
			}

			break;
		}
		case ShorthandType::BorderBox: {
			auto ltrbSplit = String::split( value, " ", "", "(\"" );
			if ( !ltrbSplit.empty() ) {
				for ( size_t i = 0; i < propNames.size(); i++ ) {
					properties.emplace_back(
						StyleSheetProperty( propNames[i], ltrbSplit[i % ltrbSplit.size()] ) );
				}
			}
			break;
		}
		case ShorthandType::Radius: {
			String::trim( value );
			auto splits = String::split( value, '/' );
			auto widths = String::split( splits[0], ' ' );
			std::vector<std::string> heights;
			if ( splits.size() >= 2 ) {
				heights = String::split( splits[1], ' ' );
			}
			if ( !widths.empty() ) {
				for ( size_t i = 0; i < propNames.size(); i++ ) {
					std::string val = widths[i % widths.size()];
					if ( !heights.empty() ) {
						val += " " + heights[i % heights.size()];
					}
					properties.emplace_back( StyleSheetProperty( propNames[i], val ) );
				}
			}
			break;
		}
		default:
			break;
	}

	return properties;
}

}}} // namespace EE::UI::CSS
