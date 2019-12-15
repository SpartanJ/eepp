#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/pack.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/ui/css/stylesheetpropertiesparser.hpp>
#include <eepp/ui/css/stylesheetselectorparser.hpp>

using namespace EE::System;

namespace EE { namespace UI { namespace CSS {

StyleSheetParser::StyleSheetParser() {}

bool StyleSheetParser::loadFromStream( IOStream& stream ) {
	mCSS.resize( stream.getSize(), '\0' );
	stream.read( &mCSS[0], stream.getSize() );
	return parse( mCSS );
}

bool StyleSheetParser::loadFromFile( const std::string& filename ) {
	if ( !FileSystem::fileExists( filename ) &&
		 PackManager::instance()->isFallbackToPacksActive() ) {
		std::string path( filename );
		Pack* pack = PackManager::instance()->exists( path );

		if ( NULL != pack ) {
			return loadFromPack( pack, path );
		}

		return false;
	}

	IOStreamFile stream( filename );
	return loadFromStream( stream );
}

bool StyleSheetParser::loadFromPack( Pack* pack, std::string filePackPath ) {
	if ( NULL == pack )
		return false;

	bool Ret = false;

	ScopedBuffer buffer;

	if ( pack->isOpen() && pack->extractFileToMemory( filePackPath, buffer ) ) {
		Ret = loadFromMemory( buffer.get(), buffer.length() );
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

StyleSheet& StyleSheetParser::getStyleSheet() {
	return mStyleSheet;
}

bool StyleSheetParser::parse( const std::string& css ) {
	ReadState rs = ReadingSelector;
	std::size_t pos = 0;
	std::string buffer;

	while ( pos < css.size() ) {
		switch ( rs ) {
			case ReadingSelector: {
				pos = readSelector( css, rs, pos, buffer );

				if ( String::startsWith( buffer, "@media" ) ) {
					std::size_t mediaClosePos = String::findCloseBracket( css, pos - 1, '{', '}' );

					if ( mediaClosePos != std::string::npos ) {
						std::string mediaStr = css.substr( pos, mediaClosePos - 1 );
						MediaQueryList::ptr& oldQueryList = mMediaQueryList;
						mMediaQueryList = MediaQueryList::parse( buffer );
						rs = ReadingSelector;
						parse( mediaStr );
						mMediaQueryList = oldQueryList;
						pos = mediaClosePos + 1;
					}
				}

				break;
			}
			case ReadingComment: {
				pos = readComment( css, rs, pos, buffer );
				break;
			}
			case ReadingProperty: {
				pos = readProperty( css, rs, pos, buffer );
				break;
			}
			default:
				break;
		}
	}

	return true;
}

int StyleSheetParser::readSelector( const std::string& css, ReadState& rs, std::size_t pos,
									std::string& buffer ) {
	buffer.clear();

	while ( pos < css.size() ) {
		if ( css[pos] == '/' && css.size() > pos + 1 && css[pos + 1] == '*' ) {
			rs = ReadingComment;
			return pos;
		}

		if ( css[pos] == '{' ) {
			rs = ReadingProperty;
			return pos + 1;
		}

		if ( css[pos] != '\n' && css[pos] != '\t' )
			buffer += css[pos];

		pos++;
	}

	return pos;
}

int StyleSheetParser::readComment( const std::string& css, ReadState& rs, std::size_t pos,
								   std::string& buffer ) {
	buffer.clear();

	while ( pos < css.size() ) {
		if ( css[pos] == '*' && css.size() > pos + 1 && css[pos + 1] == '/' ) {
			rs = ReadingSelector;

			buffer += css[pos];
			buffer += css[pos + 1];

			mComments.push_back( buffer );

			return pos + 2;
		}

		buffer += css[pos];

		pos++;
	}

	return pos;
}

int StyleSheetParser::readProperty( const std::string& css, ReadState& rs, std::size_t pos,
									std::string& buffer ) {
	std::string selectorName( buffer );

	buffer.clear();

	while ( pos < css.size() ) {
		if ( css[pos] == '{' ) {
			pos++;
			continue;
		} else if ( css[pos] == '}' ) {
			selectorName = String::trim( selectorName );

			StyleSheetSelectorParser selectorParse( selectorName );
			StyleSheetPropertiesParser propertiesParse( buffer );

			if ( !selectorParse.selectors.empty() ) {
				for ( auto it = selectorParse.selectors.begin();
					  it != selectorParse.selectors.end(); ++it ) {
					StyleSheetStyle node( it->getName(), propertiesParse.getProperties(),
										  propertiesParse.getVariables(),
										  mMediaQueryList );

					mStyleSheet.addStyle( node );
				}
			}

			rs = ReadingSelector;
			return pos + 1;
		}

		if ( css[pos] != '\n' && css[pos] != '\t' )
			buffer += css[pos];

		pos++;
	}

	return pos;
}

}}} // namespace EE::UI::CSS
