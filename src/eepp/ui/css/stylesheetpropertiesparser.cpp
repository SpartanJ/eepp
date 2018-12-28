#include <eepp/ui/css/stylesheetpropertiesparser.hpp>

namespace EE { namespace UI { namespace CSS {

StyleSheetPropertiesParser::StyleSheetPropertiesParser(){}

StyleSheetPropertiesParser::StyleSheetPropertiesParser( const std::string& propsstr ) {
	std::vector<std::string> props = String::split( propsstr, ';' );

	if ( !props.empty() ) {
		parse( propsstr );
	}
};

void StyleSheetPropertiesParser::parse( std::string propsstr ) {
	ReadState rs = ReadingPropertyName;
	prevRs = rs;
	std::size_t pos = 0;
	std::string buffer;

	while ( pos < propsstr.size() ) {
		switch (rs) {
			case ReadingPropertyName:
			{
				pos = readPropertyName(rs, pos, buffer, propsstr);
				break;
			}
			case ReadingPropertyValue:
			{
				pos = readPropertyValue(rs, pos, buffer, propsstr);
				break;
			}
			case ReadingComment:
			{
				pos = readComment(rs, pos, buffer, propsstr);
			}
			default:
				break;
		}
	}
}

int StyleSheetPropertiesParser::readPropertyName(StyleSheetPropertiesParser::ReadState & rs, std::size_t pos, std::string & buffer, const std::string& str) {
	prevRs = rs;
	buffer.clear();

	while ( pos < str.size() ) {
		if ( str[pos] == '/' && str.size() > pos + 1 && str[pos+1] == '*' ) {
			rs = ReadingComment;
			return pos;
		}

		if ( str[pos] == ':' ) {
			rs = ReadingPropertyValue;
			return pos + 1;
		}

		if ( str[pos] != '\n' && str[pos] != '\t' )
			buffer += str[pos];

		pos++;
	}

	return pos;
}

int StyleSheetPropertiesParser::readPropertyValue(StyleSheetPropertiesParser::ReadState & rs, std::size_t pos, std::string & buffer, const std::string& str) {
	std::string propName( buffer );

	buffer.clear();

	prevRs = rs;

	while ( pos < str.size() ) {
		if ( str[pos] == '/' && str.size() > pos + 1 && str[pos+1] == '*' ) {
			rs = ReadingComment;
			return pos;
		}

		if ( buffer.size() == 4 && buffer.substr(0,4) == "url(" ) {
			rs = ReadingValueUrl;
			pos = readValueUrl( rs, pos, buffer, str );
		}

		if ( str[pos] == ';' ) {
			rs = ReadingPropertyName;

			properties[ propName ] = StyleSheetProperty( propName, buffer );

			return pos + 1;
		}

		if ( str[pos] != '\n' && str[pos] != '\t' )
			buffer += str[pos];

		pos++;

		if ( pos == str.size() ) {
			rs = ReadingPropertyName;

			properties[ propName ] = StyleSheetProperty( propName, buffer );

			return pos + 1;
		}
	}

	return pos;
}

int StyleSheetPropertiesParser::readComment(StyleSheetPropertiesParser::ReadState & rs, std::size_t pos, std::string & buffer, const std::string & str) {
	buffer.clear();

	while ( pos < str.size() ) {
		if ( str[pos] == '*' && str.size() > pos + 1 && str[pos+1] == '/' ) {
			rs = prevRs;
			return pos + 2;
		}

		buffer += str[pos];

		pos++;
	}

	return pos;
}

int StyleSheetPropertiesParser::readValueUrl(StyleSheetPropertiesParser::ReadState & rs, std::size_t pos, std::string & buffer, const std::string& str) {
	bool quoted = false;

	while ( pos < str.size() ) {
		if ( !quoted && str[pos] == '"' ) {
			buffer += str[pos];

			quoted = true;
		} else if ( !quoted ) {
			if ( str[pos] != '\n' && str[pos] != '\t' )
				buffer += str[pos];

			if ( str[pos] == ')' ) {
				rs = ReadingPropertyValue;
				return pos + 1;
			}
		}
		else {
			buffer += str[pos];

			if ( quoted && str[pos] == '"' ) {
				quoted = false;
			}
		}

		pos++;
	}

	return pos;
}

}}}
