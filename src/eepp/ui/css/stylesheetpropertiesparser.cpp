#include <eepp/math/rect.hpp>
#include <eepp/ui/css/stylesheetpropertiesparser.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>

using namespace EE::UI;

namespace EE { namespace UI { namespace CSS {

static std::size_t skipComment( std::string_view str, std::size_t pos ) {
	pos += 2;
	while ( pos + 1 < str.size() ) {
		if ( str[pos] == '*' && str[pos + 1] == '/' )
			return pos + 2;
		pos++;
	}
	return str.size();
}

static void appendCommentWhitespace( std::string& buffer ) {
	if ( !buffer.empty() && buffer.back() != ' ' )
		buffer += ' ';
}

StyleSheetPropertiesParser::StyleSheetPropertiesParser( std::string_view propsstr ) {
	parse( propsstr );
}

const StyleSheetProperties& StyleSheetPropertiesParser::getProperties() const {
	return mProperties;
}

const StyleSheetVariables& StyleSheetPropertiesParser::getVariables() const {
	return mVariables;
}

void StyleSheetPropertiesParser::parse( std::string_view propsstr ) {
	mProperties.clear();
	mVariables.clear();
	ReadState rs = ReadingPropertyName;
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
			default:
				break;
		}
	}
}

int StyleSheetPropertiesParser::readPropertyName( StyleSheetPropertiesParser::ReadState& rs,
												  std::size_t pos, std::string& buffer,
												  std::string_view str ) {
	buffer.clear();

	while ( pos < str.size() ) {
		if ( str[pos] == '/' && str.size() > pos + 1 && str[pos + 1] == '*' ) {
			appendCommentWhitespace( buffer );
			pos = skipComment( str, pos );
			continue;
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
												   std::string_view str ) {
	std::string propName( buffer );

	buffer.clear();

	bool inDoubleQuote = false;
	bool inSingleQuote = false;
	int nestedParenthesis = 0;
	int prevChar = -1;

	while ( pos < str.size() ) {
		// Ensure we aren't parsing comments inside strings
		if ( str[pos] == '/' && str.size() > pos + 1 && str[pos + 1] == '*' && !inDoubleQuote &&
			 !inSingleQuote ) {
			appendCommentWhitespace( buffer );
			pos = skipComment( str, pos );
			continue;
		}

		// Only terminate property parsing on ';' if we are outside of quotes and parentheses
		if ( str[pos] == ';' && !inDoubleQuote && !inSingleQuote && nestedParenthesis == 0 ) {
			rs = ReadingPropertyName;

			addProperty( propName, buffer );

			return pos + 1;
		}

		// Keep track of quotes and nested parentheses
		if ( str[pos] == '"' && prevChar != '\\' && !inSingleQuote ) {
			inDoubleQuote = !inDoubleQuote;
		} else if ( str[pos] == '\'' && prevChar != '\\' && !inDoubleQuote ) {
			inSingleQuote = !inSingleQuote;
		} else if ( str[pos] == '(' && !inDoubleQuote && !inSingleQuote ) {
			nestedParenthesis++;
		} else if ( str[pos] == ')' && !inDoubleQuote && !inSingleQuote && nestedParenthesis > 0 ) {
			nestedParenthesis--;
		}

		if ( str[pos] != '\n' && str[pos] != '\r' && str[pos] != '\t' )
			buffer += str[pos];

		pos++;

		prevChar = str[pos - 1];
	}

	rs = ReadingPropertyName;
	addProperty( propName, buffer );
	return pos;
}

void StyleSheetPropertiesParser::addProperty( std::string name, std::string value ) {
	String::toLowerInPlace( name );
	String::trimInPlace( name );

	if ( StyleSheetSpecification::instance()->isShorthand( name ) ) {
		std::vector<StyleSheetProperty> properties =
			StyleSheetSpecification::instance()->getShorthand( name )->parse( value );

		for ( auto& property : properties )
			mProperties[property.getId()] = std::move( property );
	} else {
		if ( String::startsWith( name, "--" ) ) {
			mVariables[String::hash( name )] = StyleSheetVariable( name, value );
		} else {
			StyleSheetProperty property( name, value );
			mProperties[property.getId()] = std::move( property );
		}
	}
}

}}} // namespace EE::UI::CSS
