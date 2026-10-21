#include "freedesktopiconlookup.hpp"

#include <eepp/core/string.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>

#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstdlib>

using namespace EE::System;

namespace EE { namespace UI { namespace Private {

namespace {

std::string withSlash( std::string path ) {
	FileSystem::dirAddSlashAtEnd( path );
	return path;
}

using IniSections = std::unordered_map<std::string, std::unordered_map<std::string, std::string>>;

/** Minimal desktop-file style parser: [Section] and key=value lines. */
IniSections parseIni( const std::string& text ) {
	IniSections sections;
	std::unordered_map<std::string, std::string>* current = nullptr;
	for ( auto line : String::split( text, '\n' ) ) {
		String::trimInPlace( line, '\r' );
		String::trimInPlace( line );
		if ( line.empty() || line[0] == '#' || line[0] == ';' )
			continue;
		if ( line.front() == '[' && line.back() == ']' ) {
			current = &sections[line.substr( 1, line.size() - 2 )];
			continue;
		}
		size_t equal = line.find( '=' );
		if ( !current || equal == std::string::npos )
			continue;
		std::string key = String::trim( line.substr( 0, equal ) );
		std::string value = String::trim( line.substr( equal + 1 ) );
		( *current )[key] = value;
	}
	return sections;
}

std::string iniValue( const IniSections& ini, const std::string& section, const std::string& key ) {
	auto found = ini.find( section );
	if ( found == ini.end() )
		return {};
	auto value = found->second.find( key );
	return value == found->second.end() ? std::string() : value->second;
}

int toInt( const std::string& text, int fallback ) {
	int value = fallback;
	return !text.empty() && String::fromString( value, text ) ? value : fallback;
}

#ifdef EE_SYSTEM_ICONS_FREEDESKTOP

/** Output of a command (first line, trimmed), empty if it fails. Blocking. */
std::string commandOutput( const std::string& program, const std::string& arguments ) {
	if ( Sys::which( program ).empty() )
		return {};
	std::string command = program + " " + arguments + " 2>/dev/null";
	FILE* pipe = popen( command.c_str(), "r" );
	if ( !pipe )
		return {};
	std::string output;
	char buffer[256];
	while ( fgets( buffer, sizeof( buffer ), pipe ) )
		output += buffer;
	pclose( pipe );
	auto lines = String::split( output, '\n' );
	return lines.empty() ? std::string() : String::trim( lines.front() );
}

std::string unquote( std::string text ) {
	if ( text.size() >= 2 && ( text.front() == '\'' || text.front() == '"' ) &&
		 text.back() == text.front() )
		return text.substr( 1, text.size() - 2 );
	return text;
}

std::string configHome() {
	std::string config = Sys::getEnv( "XDG_CONFIG_HOME" );
	if ( config.empty() )
		config = withSlash( Sys::getUserDirectory() ) + ".config";
	return withSlash( config );
}

std::string kdeIconTheme() {
	std::string text;
	if ( FileSystem::fileGet( configHome() + "kdeglobals", text ) )
		return iniValue( parseIni( text ), "Icons", "Theme" );
	return {};
}

std::string gtkSettingsIconTheme() {
	for ( const char* file : { "gtk-4.0/settings.ini", "gtk-3.0/settings.ini" } ) {
		std::string text;
		if ( FileSystem::fileGet( configHome() + file, text ) ) {
			std::string value = iniValue( parseIni( text ), "Settings", "gtk-icon-theme-name" );
			if ( !value.empty() )
				return unquote( value );
		}
	}
	return {};
}

#endif

} // namespace

// ---------------------------------------------------------------------------------------------
// FreedesktopIconLookup
// ---------------------------------------------------------------------------------------------

struct FreedesktopIconLookup::Theme {
	enum class Type { Fixed, Scalable, Threshold };

	struct Dir {
		int size{ 0 };
		int scale{ 1 };
		int minSize{ 0 };
		int maxSize{ 0 };
		int threshold{ 2 };
		Type type{ Type::Threshold };
		/** Icon name -> file. */
		std::unordered_map<std::string, std::string> files;
	};

	std::string name;
	std::vector<std::string> inherits;
	std::vector<Dir> dirs;

	static bool matchesSize( const Dir& dir, int size, int scale ) {
		if ( dir.scale != scale )
			return false;
		switch ( dir.type ) {
			case Type::Fixed:
				return dir.size == size;
			case Type::Scalable:
				return dir.minSize <= size && size <= dir.maxSize;
			case Type::Threshold:
				return dir.size - dir.threshold <= size && size <= dir.size + dir.threshold;
		}
		return false;
	}

	static int sizeDistance( const Dir& dir, int size, int scale ) {
		int wanted = size * scale;
		switch ( dir.type ) {
			case Type::Fixed:
				return std::abs( dir.size * dir.scale - wanted );
			case Type::Scalable:
				if ( wanted < dir.minSize * dir.scale )
					return dir.minSize * dir.scale - wanted;
				if ( wanted > dir.maxSize * dir.scale )
					return wanted - dir.maxSize * dir.scale;
				return 0;
			case Type::Threshold:
				if ( wanted < ( dir.size - dir.threshold ) * dir.scale )
					return dir.minSize * dir.scale - wanted;
				if ( wanted > ( dir.size + dir.threshold ) * dir.scale )
					return wanted - dir.maxSize * dir.scale;
				return 0;
		}
		return INT_MAX;
	}

	std::string lookup( const std::string& icon, int size, int scale ) const {
		for ( const auto& dir : dirs ) {
			if ( !matchesSize( dir, size, scale ) )
				continue;
			auto found = dir.files.find( icon );
			if ( found != dir.files.end() )
				return found->second;
		}
		const std::string* best = nullptr;
		int bestDistance = INT_MAX;
		for ( const auto& dir : dirs ) {
			auto found = dir.files.find( icon );
			if ( found == dir.files.end() )
				continue;
			int distance = sizeDistance( dir, size, scale );
			// Equal distances: prefer the larger icon (downscaling looks better).
			if ( distance < bestDistance ) {
				bestDistance = distance;
				best = &found->second;
			}
		}
		return best ? *best : std::string();
	}
};

/** Adds files of a directory to an icon map: name -> path, preferring PNG then SVG. */
static void indexIconFiles( const std::string& dir,
							std::unordered_map<std::string, std::string>& files ) {
	for ( const auto& file : FileSystem::filesGetInPath( dir, false, false, false ) ) {
		std::string extension = FileSystem::fileExtension( file );
		if ( extension != "png" && extension != "svg" )
			continue;
		std::string name = file.substr( 0, file.size() - extension.size() - 1 );
		auto found = files.find( name );
		if ( found == files.end() ||
			 ( extension == "png" && FileSystem::fileExtension( found->second ) != "png" ) ) {
			files[name] = dir + file;
		}
	}
}

std::vector<std::string> FreedesktopIconLookup::defaultSearchPaths() {
	std::vector<std::string> paths;
#ifdef EE_SYSTEM_ICONS_FREEDESKTOP
	std::string home = withSlash( Sys::getUserDirectory() );
	paths.push_back( home + ".icons" );
	for ( const auto& dir : Sys::getDataSearchPaths() )
		paths.push_back( withSlash( dir ) + "icons" );
	paths.push_back( "/usr/share/pixmaps" );
#endif
	return paths;
}

std::string FreedesktopIconLookup::detectThemeName() {
#ifdef EE_SYSTEM_ICONS_FREEDESKTOP
	std::string desktop = String::toLower( Sys::getEnv( "XDG_CURRENT_DESKTOP" ) );
	auto is = [&]( const char* name ) { return desktop.find( name ) != std::string::npos; };
	std::string theme;
	if ( is( "kde" ) ) {
		theme = kdeIconTheme();
		return theme.empty() ? "breeze" : theme;
	}
	if ( is( "xfce" ) ) {
		theme = commandOutput( "xfconf-query", "-c xsettings -p /Net/IconThemeName" );
	} else if ( is( "cinnamon" ) ) {
		theme = unquote(
			commandOutput( "gsettings", "get org.cinnamon.desktop.interface icon-theme" ) );
	} else if ( is( "mate" ) ) {
		theme = unquote( commandOutput( "gsettings", "get org.mate.interface icon-theme" ) );
	}
	if ( theme.empty() && !desktop.empty() ) {
		theme =
			unquote( commandOutput( "gsettings", "get org.gnome.desktop.interface icon-theme" ) );
	}
	if ( theme.empty() )
		theme = gtkSettingsIconTheme();
	if ( theme.empty() )
		theme = kdeIconTheme();
	return theme;
#else
	return {};
#endif
}

FreedesktopIconLookup::FreedesktopIconLookup( const std::string& themeName,
											  std::vector<std::string> searchPaths ) :
	mSearchPaths( std::move( searchPaths ) ) {
	for ( auto& path : mSearchPaths )
		FileSystem::dirAddSlashAtEnd( path );
	auto installed = [&]( const std::string& name ) {
		if ( name.empty() )
			return false;
		for ( const auto& path : mSearchPaths ) {
			if ( FileSystem::fileExists( path + name + "/index.theme" ) )
				return true;
		}
		return false;
	};
	for ( const std::string& candidate : { themeName, std::string( "breeze" ),
										   std::string( "Adwaita" ), std::string( "hicolor" ) } ) {
		if ( installed( candidate ) ) {
			mThemeName = candidate;
			break;
		}
	}
	std::unordered_set<std::string> seen{ "hicolor" };
	if ( !mThemeName.empty() && mThemeName != "hicolor" )
		addTheme( mThemeName, seen );
	// hicolor is every theme's last parent.
	seen.erase( "hicolor" );
	addTheme( "hicolor", seen );
	for ( const auto& path : mSearchPaths )
		indexIconFiles( path, mUnthemed );
}

FreedesktopIconLookup::~FreedesktopIconLookup() {}

void FreedesktopIconLookup::addTheme( const std::string& name,
									  std::unordered_set<std::string>& seen ) {
	if ( !seen.insert( name ).second )
		return;
	std::string text;
	for ( const auto& path : mSearchPaths ) {
		if ( FileSystem::fileGet( path + name + "/index.theme", text ) )
			break;
	}
	if ( text.empty() )
		return;
	IniSections ini = parseIni( text );
	auto theme = std::make_unique<Theme>();
	theme->name = name;
	for ( const auto& parent : String::split( iniValue( ini, "Icon Theme", "Inherits" ), ',' ) ) {
		if ( !String::trim( parent ).empty() )
			theme->inherits.push_back( String::trim( parent ) );
	}
	std::vector<std::string> dirNames;
	for ( const char* key : { "Directories", "ScaledDirectories" } ) {
		for ( const auto& dir : String::split( iniValue( ini, "Icon Theme", key ), ',' ) ) {
			if ( !String::trim( dir ).empty() &&
				 std::find( dirNames.begin(), dirNames.end(), String::trim( dir ) ) ==
					 dirNames.end() )
				dirNames.push_back( String::trim( dir ) );
		}
	}
	for ( const auto& dirName : dirNames ) {
		Theme::Dir dir;
		dir.size = toInt( iniValue( ini, dirName, "Size" ), 0 );
		if ( dir.size <= 0 )
			continue;
		dir.scale = toInt( iniValue( ini, dirName, "Scale" ), 1 );
		dir.minSize = toInt( iniValue( ini, dirName, "MinSize" ), dir.size );
		dir.maxSize = toInt( iniValue( ini, dirName, "MaxSize" ), dir.size );
		dir.threshold = toInt( iniValue( ini, dirName, "Threshold" ), 2 );
		std::string type = iniValue( ini, dirName, "Type" );
		dir.type = type == "Fixed"		? Theme::Type::Fixed
				   : type == "Scalable" ? Theme::Type::Scalable
										: Theme::Type::Threshold;
		// Earlier search paths win: user icons override system ones.
		for ( const auto& path : mSearchPaths ) {
			std::string full = path + name + "/" + dirName + "/";
			if ( FileSystem::isDirectory( full ) )
				indexIconFiles( full, dir.files );
		}
		if ( !dir.files.empty() )
			theme->dirs.push_back( std::move( dir ) );
	}
	std::vector<std::string> parents = theme->inherits;
	mThemes.push_back( std::move( theme ) );
	for ( const auto& parent : parents )
		addTheme( parent, seen );
}

bool FreedesktopIconLookup::isValid() const {
	return !mThemes.empty();
}

const std::string& FreedesktopIconLookup::getThemeName() const {
	return mThemeName;
}

std::string FreedesktopIconLookup::find( const std::vector<std::string>& names, int size,
										 int scale ) const {
	// Desktop entries may name an icon file directly instead of a themed icon.
	for ( const auto& name : names ) {
		if ( !name.empty() && !FileSystem::isRelativePath( name ) &&
			 FileSystem::fileExists( name ) )
			return name;
	}
	for ( const auto& theme : mThemes ) {
		for ( const auto& name : names ) {
			std::string path = theme->lookup( name, size, scale );
			if ( !path.empty() )
				return path;
		}
	}
	for ( const auto& name : names ) {
		auto found = mUnthemed.find( name );
		if ( found != mUnthemed.end() )
			return found->second;
	}
	return {};
}

}}} // namespace EE::UI::Private
