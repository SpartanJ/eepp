#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/ui/css/stylesheetselectorparser.hpp>
#include <eepp/ui/css/stylesheetpropertiesparser.hpp>
#include <eepp/system/iostreamfile.hpp>

namespace EE { namespace UI { namespace CSS {

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

StyleSheet &StyleSheetParser::getStyleSheet() {
	return mStyleSheet;
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
