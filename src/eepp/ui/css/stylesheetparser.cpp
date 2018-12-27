#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/system/iostreamfile.hpp>

namespace EE { namespace UI { namespace CSS {

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

StyleSheetParser::StyleSheetParser() {
}

bool StyleSheetParser::loadFromFile( const std::string& file ) {
	IOStreamFile stream( file );
	return loadFromStream( stream );
}

void StyleSheetParser::print() {
	for ( auto it = mStyleSheet.nodes.begin(); it != mStyleSheet.nodes.end(); ++it ) {
		StyleSheetNode& style = *it;

		style.print();
	}

	std::cout << "Comments: " << std::endl;

	for ( std::size_t i = 0; i < mComments.size(); i++ ) {
		std::cout << mComments[i] << std::endl;
	}
}

bool StyleSheetParser::parse() {
	ReadState rs = ReadingStyle;
	std::size_t pos = 0;
	std::string buffer;

	while ( pos < mCSS.size() ) {
		switch (rs) {
			case ReadingStyle:
			{
				pos = readStyle(rs, pos, buffer);
				break;
			}
			case ReadingComment:
			{
				pos = readComment(rs, pos, buffer);
				break;
			}
			case ReadingProperty:
			{
				pos = readProperty(rs, pos, buffer);
				break;
			}
			default:
				break;
		}
	}

	return true;
}

int StyleSheetParser::readStyle(ReadState& rs, std::size_t pos, std::string& buffer) {
	buffer.clear();

	while ( pos < mCSS.size() ) {
		if ( mCSS[pos] == '/' && mCSS.size() > pos + 1 && mCSS[pos+1] == '*' ) {
			rs = ReadingComment;
			return pos;
		}

		if ( mCSS[pos] == '{' ) {
			rs = ReadingProperty;
			return pos + 1;
		}

		if ( mCSS[pos] != '\n' && mCSS[pos] != '\t' )
			buffer += mCSS[pos];

		pos++;
	}

	return pos;
}

int StyleSheetParser::readComment(ReadState& rs, std::size_t pos, std::string& buffer) {
	buffer.clear();

	while ( pos < mCSS.size() ) {
		if ( mCSS[pos] == '*' && mCSS.size() > pos + 1 && mCSS[pos+1] == '/' ) {
			rs = ReadingStyle;

			buffer += mCSS[pos];
			buffer += mCSS[pos+1];

			mComments.push_back( buffer );

			return pos + 2;
		}

		buffer += mCSS[pos];

		pos++;
	}

	return pos;
}

int StyleSheetParser::readProperty( ReadState& rs, std::size_t pos, std::string& buffer ) {
	std::string selectorName( buffer );

	buffer.clear();

	while ( pos < mCSS.size() ) {
		if ( mCSS[pos] == '{' ) {
			pos++;
			continue;
		} else if ( mCSS[pos] == '}' ) {
			selectorName = String::trim( selectorName );

			StyleSheetSelectorParser selectorParse( selectorName );
			StyleSheetPropertiesParser propertiesParse( buffer );

			if ( !selectorParse.selectors.empty() ) {
				for ( auto it = selectorParse.selectors.begin(); it != selectorParse.selectors.end(); ++it ) {
					StyleSheetNode node( selectorName, propertiesParse.properties );

					mStyleSheet.addNode( node );
				}
			}

			rs = ReadingStyle;
			return pos + 1;
		}

		if ( mCSS[pos] != '\n' && mCSS[pos] != '\t' )
			buffer += mCSS[pos];

		pos++;
	}

	return pos;
}

bool StyleSheetParser::loadFromStream( IOStream& stream ) {
	mCSS.resize( stream.getSize(), '\0' );
	stream.read( &mCSS[0], stream.getSize() );
	return parse();
}

}}}
