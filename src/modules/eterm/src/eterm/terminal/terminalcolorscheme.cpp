#include <eepp/system/clock.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/inifile.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/packmanager.hpp>
#include <eterm/terminal/terminalcolorscheme.hpp>

namespace eterm { namespace Terminal {

TerminalColorScheme TerminalColorScheme::getDefault() {
	return { "eterm",
			 Color( "#abb2bf" ),
			 Color( "#1e2127" ),
			 Color( "#abb2bf" ),
			 { Color( "#1e2127" ), Color( "#e06c75" ), Color( "#98c379" ), Color( "#d19a66" ),
			   Color( "#61afef" ), Color( "#c678dd" ), Color( "#56b6c2" ), Color( "#abb2bf" ),
			   Color( "#5c6370" ), Color( "#e06c75" ), Color( "#98c379" ), Color( "#d19a66" ),
			   Color( "#61afef" ), Color( "#c678dd" ), Color( "#56b6c2" ), Color( "#ffffff" ) } };
}

std::vector<TerminalColorScheme> TerminalColorScheme::loadFromStream( IOStream& stream ) {
	Clock clock;
	std::vector<TerminalColorScheme> colorSchemes;
	TerminalColorScheme refColorScheme( getDefault() );
	IniFile ini( stream );
	for ( size_t keyIdx = 0; keyIdx < ini.getNumKeys(); keyIdx++ ) {
		TerminalColorScheme colorScheme;
		std::string name( ini.getKeyName( keyIdx ) );
		colorScheme.setName( name );
		colorScheme.mForeground = Color( ini.getValue( name, "foreground", "white" ) );
		colorScheme.mBackground = Color( ini.getValue( name, "background", "black" ) );
		colorScheme.mCursor = Color( ini.getValue( name, "cursor", "white" ) );
		auto palette( String::split( ini.getValue( name, "palette" ), ';' ) );
		auto psize = palette.size();
		colorScheme.mPalette.resize( psize );
		for ( size_t i = 0; i < psize; ++i )
			colorScheme.mPalette[i] = Color( palette[i] );
		colorSchemes.emplace_back( colorScheme );
	}
	Log::info( "Terminal Color Schemes loaded in %.2fms.",
			   clock.getElapsedTime().asMilliseconds() );
	return colorSchemes;
}

std::vector<TerminalColorScheme> TerminalColorScheme::loadFromFile( const std::string& path ) {
	if ( !FileSystem::fileExists( path ) && PackManager::instance()->isFallbackToPacksActive() ) {
		std::string pathFix( path );
		Pack* pack = PackManager::instance()->exists( pathFix );
		if ( NULL != pack ) {
			return loadFromPack( pack, pathFix );
		}
		return {};
	}
	IOStreamFile stream( path );
	return loadFromStream( stream );
}

std::vector<TerminalColorScheme> TerminalColorScheme::loadFromMemory( const void* data,
																	  std::size_t sizeInBytes ) {
	IOStreamMemory stream( (const char*)data, sizeInBytes );
	return loadFromStream( stream );
}

std::vector<TerminalColorScheme> TerminalColorScheme::loadFromPack( Pack* pack,
																	std::string filePackPath ) {
	if ( NULL == pack )
		return {};
	ScopedBuffer buffer;
	if ( pack->isOpen() && pack->extractFileToMemory( filePackPath, buffer ) ) {
		return loadFromMemory( buffer.get(), buffer.length() );
	}
	return {};
}

TerminalColorScheme::TerminalColorScheme() {}

TerminalColorScheme::TerminalColorScheme( const std::string& name, const Color& foreground,
										  const Color& background, const Color& cursor,
										  const std::vector<Color> palette ) :
	mName( name ),
	mForeground( foreground ),
	mBackground( background ),
	mCursor( cursor ),
	mPalette( palette ) {}

const std::string& TerminalColorScheme::getName() const {
	return mName;
}

const Color& TerminalColorScheme::getForeground() const {
	return mForeground;
}

const Color& TerminalColorScheme::getBackground() const {
	return mBackground;
}

const Color& TerminalColorScheme::getCursor() const {
	return mCursor;
}

const std::vector<Color>& TerminalColorScheme::getPalette() const {
	return mPalette;
}

const Color& TerminalColorScheme::getPaletteIndex( const size_t& index ) const {
	eeASSERT( index < mPalette.size() );
	return mPalette[index];
}

size_t TerminalColorScheme::getPaletteSize() const {
	return mPalette.size();
}

void TerminalColorScheme::setName( const std::string& name ) {
	mName = name;
}

}} // namespace eterm::Terminal
