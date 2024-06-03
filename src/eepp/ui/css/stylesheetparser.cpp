#include <algorithm>
#include <eepp/network/http.hpp>
#include <eepp/network/uri.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/functionstring.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/pack.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/system/virtualfilesystem.hpp>
#include <eepp/ui/css/keyframesdefinition.hpp>
#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/ui/css/stylesheetpropertiesparser.hpp>
#include <eepp/ui/css/stylesheetselectorparser.hpp>
#include <iostream>

using namespace EE::Network;
using namespace EE::System;

namespace EE { namespace UI { namespace CSS {

StyleSheetParser::StyleSheetParser() : mLoaded( false ) {}

bool StyleSheetParser::loadFromStream( IOStream& stream ) {
	Clock elapsed;
	std::vector<std::string> importedList;
	mCSS.resize( stream.getSize() );
	stream.read( &mCSS[0], stream.getSize() );
	bool ok = parse( mCSS, importedList );
	Log::info( "StyleSheet loaded in: %4.3f ms.", elapsed.getElapsedTime().asMilliseconds() );
	mLoaded = ok;
	return ok;
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

bool StyleSheetParser::loadFromString( const std::string_view& str ) {
	if ( str.empty() )
		return false;

	return loadFromMemory( (const Uint8*)str.data(), str.size() );
}

StyleSheet& StyleSheetParser::getStyleSheet() {
	return mStyleSheet;
}

const bool& StyleSheetParser::isLoaded() const {
	return mLoaded;
}

bool StyleSheetParser::parse( std::string& css, std::vector<std::string>& importedList ) {
	ReadState rs = ReadingSelector;
	std::size_t pos = 0;
	std::string buffer;
	std::size_t size = css.size();

	// Check UTF-8 BOM header
	if ( size >= 3 && (char)0xef == css[0] && (char)0xbb == css[1] && (char)0xbf == css[2] )
		pos = 3; // skip BOM header

	while ( pos < size ) {
		switch ( rs ) {
			case ReadingSelector: {
				pos = readSelector( css, rs, pos, buffer );

				if ( buffer[0] == '@' ) {
					if ( String::startsWith( buffer, "@media" ) ) {
						mediaParse( css, rs, pos, buffer, importedList );
					} else if ( String::startsWith( buffer, "@import" ) ) {
						importParse( css, pos, buffer, importedList );
					} else if ( String::startsWith( buffer, "@keyframes" ) ) {
						keyframesParse( css, rs, pos, buffer );
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
	std::size_t initialPos = pos;
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

		if ( css[pos] != '\n' && css[pos] != '\r' && css[pos] != '\t' )
			buffer += css[pos];

		if ( css[pos] == ';' && String::startsWith( buffer, "@import" ) ) {
			return initialPos;
		}

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
			StyleSheetPropertiesParser propertiesParser( buffer );

			if ( !selectorParse.selectors.empty() ) {
				for ( auto it = selectorParse.selectors.begin();
					  it != selectorParse.selectors.end(); ++it ) {
					std::shared_ptr<StyleSheetStyle> node = std::make_shared<StyleSheetStyle>(
						it->getName(), propertiesParser.getProperties(),
						propertiesParser.getVariables(), mMediaQueryList );

					mStyleSheet.addStyle( node );
				}
			}

			rs = ReadingSelector;
			return pos + 1;
		}

		if ( css[pos] != '\n' && css[pos] != '\r' && css[pos] != '\t' )
			buffer += css[pos];

		pos++;
	}

	return pos;
}

std::string StyleSheetParser::importCSS( std::string path,
										 std::vector<std::string>& importedList ) {
	if ( String::startsWith( path, "file://" ) ) {
		path = path.substr( 7 );
	}

	if ( FileSystem::fileExists( path ) ) {
		ScopedBuffer buffer;

		if ( FileSystem::fileGet( path, buffer ) ) {
			importedList.push_back( path );
			return std::string( reinterpret_cast<const char*>( buffer.get() ), buffer.size() );
		}
	} else if ( String::startsWith( path, "http://" ) || String::startsWith( path, "https://" ) ) {
		Http::Response response = Http::get( URI( path ), Seconds( 5 ) );
		if ( response.getStatus() == Http::Response::Status::Ok ) {
			importedList.push_back( path );
			return response.getBody();
		}
	} else if ( VFS::instance()->fileExists( path ) ) {
		IOStream* stream = VFS::instance()->getFileFromPath( path );
		ScopedBuffer buffer( stream->getSize() );
		stream->read( reinterpret_cast<char*>( buffer.get() ), buffer.size() );
		importedList.push_back( path );
		return std::string( reinterpret_cast<const char*>( buffer.get() ) );
	} else {
		if ( PackManager::instance()->isFallbackToPacksActive() ) {
			Pack* pack = PackManager::instance()->exists( path );

			if ( std::find( importedList.begin(), importedList.end(), path ) ==
				 importedList.end() ) {
				if ( NULL != pack ) {
					Log::debug( "Loading css from pack: %s", path.c_str() );
					ScopedBuffer buffer;
					if ( pack->isOpen() && pack->extractFileToMemory( path, buffer ) ) {
						importedList.push_back( path );
						return std::string( reinterpret_cast<const char*>( buffer.get() ) );
					}
				}
			}
		}
	}

	return std::string();
}

void StyleSheetParser::mediaParse( std::string& css, ReadState& rs, std::size_t& pos,
								   std::string& buffer, std::vector<std::string>& importedList ) {
	std::size_t mediaClosePos = String::findCloseBracket( css, pos - 1, '{', '}' );

	if ( mediaClosePos != std::string::npos ) {
		std::string mediaStr = css.substr( pos, mediaClosePos - pos );
		MediaQueryList::ptr oldQueryList = mMediaQueryList;
		mMediaQueryList = MediaQueryList::parse( buffer );
		rs = ReadingSelector;
		parse( mediaStr, importedList );
		mMediaQueryList = oldQueryList;
		pos = mediaClosePos + 1;
	}
}

void StyleSheetParser::importParse( std::string& css, std::size_t& pos, std::string& buffer,
									std::vector<std::string>& importedList ) {
	std::string::size_type endImport = css.find_first_of( ";", pos );

	if ( endImport == std::string::npos ) {
		endImport = css.find_first_of( "\n", pos );
		if ( endImport == std::string::npos ) {
			endImport = css.size() - 1;
		}
	}

	std::string import( css.substr( pos, endImport - pos ) );
	std::vector<std::string> tokens = String::split( import, " " );

	if ( tokens.size() >= 2 ) {
		std::string path( tokens[1] );
		std::string mediaStr;

		if ( tokens.size() > 2 ) {
			for ( size_t i = 2; i < tokens.size(); i++ ) {
				mediaStr += tokens[i] + " ";
			}
		}

		FunctionString function( FunctionString::parse( path ) );

		if ( function.getName() == "url" && !function.getParameters().empty() ) {
			path = function.getParameters().at( 0 );
		}

		if ( std::find( importedList.begin(), importedList.end(), path ) == importedList.end() ) {
			std::string newCss( importCSS( path, importedList ) );

			if ( !newCss.empty() ) {
				if ( !mediaStr.empty() ) {
					mediaStr.insert( 0, "@media " );
					mediaStr.append( "{\n" );
					newCss = mediaStr + newCss + "\n}";
				}

				std::string head( css.substr( 0, pos ) );
				std::string tail( css.substr( endImport + 1 ) );
				css = head + newCss + tail;

				buffer.clear();
			} else {
				pos = endImport + 1;
			}

		} else {
			pos = endImport + 1;
		}
	} else {
		pos = endImport + 1;
	}
}

void StyleSheetParser::keyframesParse( std::string& css, ReadState& rs, std::size_t& pos,
									   std::string& buffer ) {
	std::size_t keyframesClosePos = String::findCloseBracket( css, pos - 1, '{', '}' );

	if ( keyframesClosePos != std::string::npos ) {
		StyleSheetParser keyframeParser;
		keyframeParser.loadFromMemory( reinterpret_cast<const Uint8*>( &css[pos] ),
									   keyframesClosePos - pos );
		const std::vector<std::shared_ptr<StyleSheetStyle>>& styles =
			keyframeParser.getStyleSheet().getStyles();

		std::string name(
			String::trim( String::trim( buffer.substr( buffer.find_first_of( " " ) ) ), '"' ) );

		mStyleSheet.addKeyframes( KeyframesDefinition::parseKeyframes( name, styles ) );
	}

	rs = ReadingSelector;
	pos = keyframesClosePos + 1;
}

}}} // namespace EE::UI::CSS
