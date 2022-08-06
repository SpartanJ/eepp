#include <eepp/math/rect.hpp>
#include <eepp/ui/css/stylesheetpropertiesparser.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>

using namespace EE::UI;

namespace EE { namespace UI { namespace CSS {

StyleSheetPropertiesParser::StyleSheetPropertiesParser() : mPrevRs( ReadingPropertyName ) {}

StyleSheetPropertiesParser::StyleSheetPropertiesParser( const std::string& propsstr ) :
	mPrevRs( ReadingPropertyName ) {
	std::vector<std::string> props = String::split( propsstr, ';' );

	if ( !props.empty() ) {
		parse( propsstr );
	}
}

const StyleSheetProperties& StyleSheetPropertiesParser::getProperties() const {
	return mProperties;
}

const StyleSheetVariables& StyleSheetPropertiesParser::getVariables() const {
	return mVariables;
}

void StyleSheetPropertiesParser::parse( const std::string& propsstr ) {
	mProperties.clear();
	mVariables.clear();
	ReadState rs = ReadingPropertyName;
	mPrevRs = rs;
	std::size_t pos = 0;
	std::string buffer;

	while ( pos < propsstr.size() ) {
		switch ( rs ) {
			case ReadingPropertyName: {
				pos = readPropertyName( rs, pos, buffer, propsstr );
				break;
			}
			case ReadingPropertyValue: {
				pos = readPropertyValue( rs, pos, buffer, propsstr );
				break;
			}
			case ReadingComment: {
				pos = readComment( rs, pos, buffer, propsstr );
			}
			default:
				break;
		}
	}
}

int StyleSheetPropertiesParser::readPropertyName( StyleSheetPropertiesParser::ReadState& rs,
												  std::size_t pos, std::string& buffer,
												  const std::string& str ) {
	mPrevRs = rs;
	buffer.clear();

	while ( pos < str.size() ) {
		if ( str[pos] == '/' && str.size() > pos + 1 && str[pos + 1] == '*' ) {
			rs = ReadingComment;
			return pos;
		}

		if ( str[pos] == ':' ) {
			rs = ReadingPropertyValue;
			return pos + 1;
		}

		if ( str[pos] != '\n' && str[pos] != '\r' && str[pos] != '\t' )
			buffer += str[pos];

		pos++;
	}

	return pos;
}

int StyleSheetPropertiesParser::readPropertyValue( StyleSheetPropertiesParser::ReadState& rs,
												   std::size_t pos, std::string& buffer,
												   const std::string& str ) {
	std::string propName( buffer );

	buffer.clear();

	mPrevRs = rs;

	bool inString = false;
	int prevChar = -1;

	while ( pos < str.size() ) {
		if ( str[pos] == '/' && str.size() > pos + 1 && str[pos + 1] == '*' ) {
			rs = ReadingComment;
			return pos;
		}

		if ( str[pos] == ';' && !inString ) {
			rs = ReadingPropertyName;

			addProperty( propName, buffer );

			return pos + 1;
		}

		if ( str[pos] == '"' && prevChar != '\\' )
			inString = !inString;

		if ( str[pos] != '\n' && str[pos] != '\r' && str[pos] != '\t' )
			buffer += str[pos];

		pos++;

		if ( pos == str.size() ) {
			rs = ReadingPropertyName;

			addProperty( propName, buffer );

			return pos + 1;
		}

		prevChar = str[pos - 1];
	}

	return pos;
}

int StyleSheetPropertiesParser::readComment( StyleSheetPropertiesParser::ReadState& rs,
											 std::size_t pos, std::string& buffer,
											 const std::string& str ) {
	buffer.clear();

	while ( pos < str.size() ) {
		if ( str[pos] == '*' && str.size() > pos + 1 && str[pos + 1] == '/' ) {
			rs = mPrevRs;
			return pos + 2;
		}

		buffer += str[pos];

		pos++;
	}

	return pos;
}

void StyleSheetPropertiesParser::addProperty( std::string name, std::string value ) {
	String::toLowerInPlace( name );
	String::trimInPlace( name );

	if ( StyleSheetSpecification::instance()->isShorthand( name ) ) {
		std::vector<StyleSheetProperty> properties =
			StyleSheetSpecification::instance()->getShorthand( name )->parse( value );

		for ( auto& property : properties )
			mProperties.emplace( std::make_pair( property.getId(), std::move( property ) ) );
	} else {
		if ( String::startsWith( name, "--" ) ) {
			mVariables[String::hash( name )] = StyleSheetVariable( name, value );
		} else {
			StyleSheetProperty property( name, value );
			mProperties.emplace( std::make_pair( property.getId(), std::move( property ) ) );
		}
	}
}

}}} // namespace EE::UI::CSS
