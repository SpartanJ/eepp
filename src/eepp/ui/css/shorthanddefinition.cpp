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
	mName( name ), mId( String::hash( name ) ), mProperties( properties ), mType( shorthandType ) {}

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

			if ( values.size() >= 2 ) {
				properties.emplace_back( StyleSheetProperty( propNames[0], values[0] ) );
				properties.emplace_back( StyleSheetProperty( propNames[1], values[1] ) );
			} else if ( values.size() == 1 ) {
				properties.emplace_back( StyleSheetProperty( propNames[0], values[0] ) );
				properties.emplace_back( StyleSheetProperty( propNames[1], values[0] ) );
			}
			break;
		}
		case ShorthandType::BackgroundPosition: {
			std::vector<std::string> pos = String::split( value, ' ' );

			if ( pos.size() == 1 )
				pos.push_back( "center" );

			if ( pos.size() == 2 ) {
				int xFloatIndex = 0;
				int yFloatIndex = 1;

				if ( "bottom" == pos[0] || "top" == pos[0] ) {
					xFloatIndex = 1;
					yFloatIndex = 0;
				}

				properties.push_back( StyleSheetProperty( propNames[0], pos[xFloatIndex] ) );
				properties.push_back( StyleSheetProperty( propNames[1], pos[yFloatIndex] ) );
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

				properties.push_back( StyleSheetProperty(
					propNames[0], pos[xFloatIndex] + " " + pos[xFloatIndex + 1] ) );
				properties.push_back( StyleSheetProperty(
					propNames[1], pos[yFloatIndex] + " " + pos[yFloatIndex + 1] ) );
			}

			break;
		}
		default:
			break;
	}

	return properties;
}

}}} // namespace EE::UI::CSS
