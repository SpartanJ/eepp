#include <eepp/ui/css/stylesheetpropertiesparser.hpp>
#include <eepp/math/rect.hpp>
#include <eepp/scene/nodeattribute.hpp>

using namespace EE::UI;
using namespace EE::Scene;

namespace EE { namespace UI { namespace CSS {

StyleSheetPropertiesParser::StyleSheetPropertiesParser(){}

StyleSheetPropertiesParser::StyleSheetPropertiesParser( const std::string& propsstr ) {
	std::vector<std::string> props = String::split( propsstr, ';' );

	if ( !props.empty() ) {
		parse( propsstr );
	}
}

const StyleSheetProperties & StyleSheetPropertiesParser::getProperties() const {
	return mProperties;
};

void StyleSheetPropertiesParser::parse( std::string propsstr ) {
	ReadState rs = ReadingPropertyName;
	mPrevRs = rs;
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
	mPrevRs = rs;
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

	mPrevRs = rs;

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

			addProperty( propName, buffer );

			return pos + 1;
		}

		if ( str[pos] != '\n' && str[pos] != '\t' )
			buffer += str[pos];

		pos++;

		if ( pos == str.size() ) {
			rs = ReadingPropertyName;

			addProperty( propName, buffer );

			return pos + 1;
		}
	}

	return pos;
}

int StyleSheetPropertiesParser::readComment(StyleSheetPropertiesParser::ReadState & rs, std::size_t pos, std::string & buffer, const std::string & str) {
	buffer.clear();

	while ( pos < str.size() ) {
		if ( str[pos] == '*' && str.size() > pos + 1 && str[pos+1] == '/' ) {
			rs = mPrevRs;
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

void StyleSheetPropertiesParser::addProperty( const std::string& name, std::string value ) {
	if ( name == "padding" ) {
		value = String::toLower( String::trim( value ) );
		Rectf rect( NodeAttribute( name, value ).asRectf() );
		mProperties[ "paddingleft" ] = StyleSheetProperty( "paddingleft", String::toStr( rect.Left ) );
		mProperties[ "paddingright" ] = StyleSheetProperty( "paddingright", String::toStr( rect.Right ) );
		mProperties[ "paddingtop" ] = StyleSheetProperty( "paddingtop", String::toStr( rect.Top ) );
		mProperties[ "paddingbottom" ] = StyleSheetProperty( "paddingbottom", String::toStr( rect.Bottom ) );
	} else if ( name == "layout_margin" ) {
		value = String::toLower( String::trim( value ) );
		Rect rect( NodeAttribute( name, value ).asRect() );
		mProperties[ "layout_marginleft" ] = StyleSheetProperty( "layout_marginleft", String::toStr( rect.Left ) );
		mProperties[ "layout_marginright" ] = StyleSheetProperty( "layout_marginright", String::toStr( rect.Right ) );
		mProperties[ "layout_margintop" ] = StyleSheetProperty( "layout_margintop", String::toStr( rect.Top ) );
		mProperties[ "layout_marginbottom" ] = StyleSheetProperty( "layout_marginbottom", String::toStr( rect.Bottom ) );
	} else {
		mProperties[ name ] = StyleSheetProperty( name, value );
	}
}

}}}

