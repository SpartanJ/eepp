#include "utest.h"
#include <eepp/graphics/image.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/doc/textformat.hpp>

using namespace EE::Graphics;
using namespace EE::System;
using namespace EE::UI::Doc;

TextFormat::Encoding encodingFromString( const std::string_view& str ) {
	if ( str == "utf16be" )
		return TextFormat::Encoding::UTF16BE;
	if ( str == "utf16le" )
		return TextFormat::Encoding::UTF16LE;
	if ( str == "win1252" )
		return TextFormat::Encoding::Latin1;
	if ( str == "shiftjis" )
		return TextFormat::Encoding::Shift_JIS;
	if ( str == "utf8" )
		return TextFormat::Encoding::UTF8;
	return TextFormat::Encoding::UTF8;
}

UTEST( TextFormat, autodetect ) {
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
	auto files = FileSystem::filesInfoGetInPath( std::string{ "assets/textformat" }, false, true );
	for ( const auto& file : files ) {
		if ( file.isDirectory() )
			continue;
		auto fnParts( String::split( file.getFileName(), '.' ) );
		ASSERT_EQ( fnParts.size(), 5UL );
		auto lang = fnParts[0];
		auto encoding = encodingFromString( fnParts[1] );
		auto newLine = TextFormat::stringToLineEnding( String::toUpper( fnParts[2] ) );
		auto bom = fnParts[3] == "bom";
		IOStreamFile stream( file.getFilepath() );
		auto textFormat = TextFormat::autodetect( stream );
		ASSERT_EQ_MSG(
			textFormat.bom, bom,
			String::format( "%s %d vs %d", file.getFileName(), textFormat.bom, bom ).c_str() );
		ASSERT_EQ_MSG( textFormat.encoding, encoding,
					   String::format( "%s %s vs %s", file.getFileName(),
									   TextFormat::encodingToString( textFormat.encoding ),
									   TextFormat::encodingToString( encoding ) )
						   .c_str() );
		ASSERT_EQ_MSG( textFormat.newLine, newLine,
					   String::format( "%s %s vs %s", file.getFileName(),
									   TextFormat::lineEndingToString( textFormat.newLine ),
									   TextFormat::lineEndingToString( newLine ) )
						   .c_str() );
	}
}

UTEST( TextFormat, autodetectProject ) {
	std::string projectRoot = Sys::getProcessPath();
	FileSystem::dirRemoveSlashAtEnd( projectRoot );
	projectRoot = FileSystem::fileRemoveFileName( projectRoot );
	FileSystem::dirRemoveSlashAtEnd( projectRoot );
	projectRoot = FileSystem::fileRemoveFileName( projectRoot ) + "src";
	std::function<void( const std::string& )> checkFolder;
	auto getEncoding = []( const std::string& filename ) {
		static std::unordered_map<std::string, TextFormat::Encoding> KNOWN_ENCS = {
			{ "SDL_sunaudio.c", TextFormat::Encoding::Latin1 },
			{ "default_cursor.h", TextFormat::Encoding::Latin1 },
			{ "InfoPlist.strings", TextFormat::Encoding::UTF16BE },
			{ "version.rc", TextFormat::Encoding::Latin1 },
			{ "SDL_RLEaccel.c", TextFormat::Encoding::Latin1 },
		};
		auto found = KNOWN_ENCS.find( filename );
		if ( found != KNOWN_ENCS.end() )
			return found->second;
		return TextFormat::Encoding::UTF8;
	};
	checkFolder = [&]( const std::string& folder ) {
		auto files = FileSystem::filesInfoGetInPath( folder, false, true, true, true );
		for ( const auto& file : files ) {
			if ( file.isDirectory() ) {
				checkFolder( file.getFilepath() );
				continue;
			}
			auto extension = file.getExtension();
			if ( "a" == extension || "zip" == extension || "dll" == extension ||
				 "dat" == extension || "cur" == extension || "icns" == extension ||
				 "wav" == extension || Image::isImageExtension( file.getFilepath() ) ||
				 LuaPattern::hasMatches( file.getFilepath(), "SDL2%-%d+%.%d+%.%d+" ) )
				continue;
			IOStreamFile stream( file.getFilepath() );
			auto expectedEncoding = getEncoding( file.getFileName() );
			auto textFormat = TextFormat::autodetect( stream );
			EXPECT_EQ_MSG( textFormat.encoding, expectedEncoding,
						   String::format( "%s %s vs %s", file.getFilepath(),
										   TextFormat::encodingToString( textFormat.encoding ),
										   TextFormat::encodingToString( expectedEncoding ) )
							   .c_str() );
		}
	};
	checkFolder( projectRoot );
}
