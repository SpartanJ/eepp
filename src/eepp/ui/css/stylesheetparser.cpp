#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/ui/css/stylesheetselectorparser.hpp>
#include <eepp/ui/css/stylesheetpropertiesparser.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/pack.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/system/filesystem.hpp>

using namespace EE::System;

namespace EE { namespace UI { namespace CSS {

StyleSheetParser::StyleSheetParser() {
}

bool StyleSheetParser::loadFromStream( IOStream& stream ) {
	mCSS.resize( stream.getSize(), '\0' );
	stream.read( &mCSS[0], stream.getSize() );
	return parse();
}

bool StyleSheetParser::loadFromFile( const std::string& filename ) {
	if ( !FileSystem::fileExists( filename ) && PackManager::instance()->isFallbackToPacksActive() ) {
		std::string path( filename );
		Pack * pack = PackManager::instance()->exists( path );

		if ( NULL != pack ) {
			return loadFromPack( pack, path );
		}

		return false;
	}

	IOStreamFile stream( filename );
	return loadFromStream( stream );
}

bool StyleSheetParser::loadFromPack( Pack * pack, std::string filePackPath ) {
	if ( NULL == pack )
		return false;

	bool Ret = false;

	SafeDataPointer PData;

	if ( pack->isOpen() && pack->extractFileToMemory( filePackPath, PData ) ) {
		Ret = loadFromMemory( PData.data, PData.size );
	}

	return Ret;
}

bool StyleSheetParser::loadFromMemory( const Uint8* RAWData, const Uint32& size ) {
	IOStreamMemory stream( (const char*)RAWData, size );
	return loadFromStream( stream );
}

void StyleSheetParser::print() {
	mStyleSheet.print();

	std::cout << "Comments: " << std::endl;

	for ( std::size_t i = 0; i < mComments.size(); i++ ) {
		std::cout << mComments[i] << std::endl;
	}
}

bool StyleSheetParser::loadFromString( const std::string& str ) {
	if ( str.empty() )
		return false;

	return loadFromMemory( (const Uint8*)&str[0], str.size() );
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
					StyleSheetStyle node( it->getName(), propertiesParse.getProperties() );

					mStyleSheet.addStyle( node );
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

}}}
