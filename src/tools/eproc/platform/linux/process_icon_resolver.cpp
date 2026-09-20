#include "process_icon_resolver.hpp"

#include <dirent.h>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>

#include <eepp/core/string.hpp>
#include <eepp/system/fileinfo.hpp>
#include <eepp/system/log.hpp>

using namespace EE;
using namespace EE::System;

namespace eproc {

// The icon themes probed for an icon name, in preference order. Every one of them is optional.
static const char* kIconThemes[] = { "hicolor", "breeze", "Adwaita" };

// The unthemed icon directory, searched after all the themes.
static const char* kIconPixmapDir = "/usr/share/pixmaps/";

// Sizes present in a theme, and the extensions an icon file may use. The two extra large sizes at
// the end are not part of the classic set but are the only ones a few applications ship.
static const char* kIconSizes[] = { "16x16", "22x22",	"24x24",	"32x32",   "48x48",
									"64x64", "128x128", "scalable", "256x256", "512x512" };
static const char* kIconExtensions[] = { ".png", ".svg", ".xpm" };

// Sizes preferred for the PNG pass, in order; the scalable SVG comes right after them.
static const char* kPreferredPngSizes[] = { "24x24", "32x32", "48x48" };

// Suffixes of a wrapper or bundle binary that the corresponding desktop id does not carry.
static const char* kExecutableSuffixes[] = { ".bin", ".sh", ".py", ".pl", ".run" };

// Directory names that identify a location rather than an application, and are never an app id.
static const char* kGenericDirNames[] = { "bin", "sbin",  "lib",   "lib64",	  "libexec",	 "usr",
										  "opt", "local", "share", "program", "applications" };

static bool isGenericDirName( std::string_view name ) {
	for ( const char* generic : kGenericDirNames ) {
		if ( name == generic )
			return true;
	}
	return false;
}

// Everything after the last separator. The kernel appends " (deleted)" to a /proc/<pid>/exe link
// when the binary has been replaced or removed, which would never match an index key. The result is
// always a new string, so the argument is only ever read.
static std::string baseName( std::string_view path ) {
	std::string name( path );
	size_t slash = name.find_last_of( '/' );
	if ( slash != std::string::npos )
		name.erase( 0, slash + 1 );
	if ( String::endsWith( name, " (deleted)" ) )
		name.resize( name.size() - strlen( " (deleted)" ) );
	return name;
}

// First token of an XDG Exec value, with the field codes (%U, %F, %u, %f, %%, ...) removed: that is
// the executable name the desktop entry is keyed by.
static std::string execToken( const std::string& exec ) {
	size_t start = exec.find_first_not_of( " \t" );
	if ( start == std::string::npos )
		return {};

	std::string token;
	if ( exec[start] == '"' ) {
		size_t end = exec.find( '"', start + 1 );
		if ( end == std::string::npos )
			return {};
		token = exec.substr( start + 1, end - start - 1 );
	} else {
		size_t end = exec.find_first_of( " \t", start );
		token = end == std::string::npos ? exec.substr( start ) : exec.substr( start, end - start );
	}

	std::string cleaned;
	cleaned.reserve( token.size() );
	for ( size_t i = 0; i < token.size(); i++ ) {
		if ( token[i] == '%' && i + 1 < token.size() ) {
			if ( token[i + 1] == '%' )
				cleaned += '%';
			i++;
			continue;
		}
		cleaned += token[i];
	}

	return baseName( cleaned );
}

// True when an icon name carries its own extension, in which case it is never extended further: an
// "Icon=foo.svg" entry must not be probed as "foo.svg.png".
static bool hasIconExtension( const std::string& name ) {
	for ( const char* extension : kIconExtensions ) {
		if ( String::endsWith( name, extension ) )
			return true;
	}
	return false;
}

static bool allowsExtension( const std::string& name, const char* extension ) {
	return !hasIconExtension( name ) || String::endsWith( name, extension );
}

static std::string themeIconPath( const char* theme, const char* size, const std::string& name ) {
	std::string path = "/usr/share/icons/";
	path += theme;
	path += '/';
	path += size;
	path += "/apps/";
	path += name;
	return path;
}

// Maps an icon name to the icon file that backs it. Preference order: 32x32 PNG, then 48x48 PNG,
// then the scalable SVG, then any other icon of any size and known extension, and finally the
// unthemed /usr/share/pixmaps directory.
static std::string findIconFile( const std::string& iconName ) {
	if ( iconName.empty() )
		return {};

	// An absolute path (used by a few hand written entries) is taken as is.
	if ( iconName[0] == '/' ) {
		FileInfo icon( iconName );
		return icon.isRegularFile() ? iconName : std::string();
	}

	// "pixmaps/<name>" refers to the unthemed pixmap directory.
	std::string name = String::startsWith( iconName, "pixmaps/" )
						   ? iconName.substr( strlen( "pixmaps/" ) )
						   : iconName;
	if ( name.empty() )
		return {};

	if ( allowsExtension( name, ".png" ) ) {
		for ( const char* size : kPreferredPngSizes ) {
			for ( const char* theme : kIconThemes ) {
				std::string path = themeIconPath( theme, size, name + ".png" );
				if ( FileInfo( path ).isRegularFile() )
					return path;
			}
		}
	}

	if ( allowsExtension( name, ".svg" ) ) {
		for ( const char* theme : kIconThemes ) {
			std::string path = themeIconPath( theme, "scalable", name + ".svg" );
			if ( FileInfo( path ).isRegularFile() )
				return path;
		}
	}

	for ( const char* theme : kIconThemes ) {
		for ( const char* size : kIconSizes ) {
			for ( const char* extension : kIconExtensions ) {
				if ( !allowsExtension( name, extension ) )
					continue;
				std::string path = themeIconPath( theme, size, name + extension );
				if ( FileInfo( path ).isRegularFile() )
					return path;
			}
		}
	}

	for ( const char* extension : kIconExtensions ) {
		if ( !allowsExtension( name, extension ) )
			continue;
		std::string path = std::string( kIconPixmapDir ) + name + extension;
		if ( FileInfo( path ).isRegularFile() )
			return path;
	}

	// Some entries point straight at an extension-less file in the pixmap directory.
	std::string nakedPath = std::string( kIconPixmapDir ) + name;
	if ( FileInfo( nakedPath ).isRegularFile() )
		return nakedPath;

	return {};
}

// Every directory that can hold desktop entries, in XDG precedence order: the user's own entries
// shadow the system-wide ones, which is why they are scanned (and therefore indexed) first.
static std::vector<std::string> desktopDirectories() {
	std::vector<std::string> dirs;

	auto addDirectory = [&dirs]( const std::string& dir ) {
		if ( dir.empty() )
			return;
		for ( const std::string& existing : dirs ) {
			if ( existing == dir )
				return;
		}
		dirs.push_back( dir );
	};

	auto addDataDirectory = [&addDirectory]( const std::string& dataDir ) {
		if ( dataDir.empty() )
			return;
		addDirectory( dataDir.back() == '/' ? dataDir + "applications"
											: dataDir + "/applications" );
	};

	// XDG_DATA_HOME defaults to $HOME/.local/share.
	const char* dataHome = std::getenv( "XDG_DATA_HOME" );
	if ( dataHome && *dataHome ) {
		addDataDirectory( dataHome );
	} else {
		const char* home = std::getenv( "HOME" );
		if ( home && *home )
			addDataDirectory( std::string( home ) + "/.local/share" );
	}

	// XDG_DATA_DIRS defaults to /usr/local/share:/usr/share. The list is ordered, earlier entries
	// taking precedence; addDirectory() keeps that order and drops the duplicates.
	const char* dataDirs = std::getenv( "XDG_DATA_DIRS" );
	const std::string dataDirList =
		( dataDirs && *dataDirs ) ? dataDirs : "/usr/local/share:/usr/share";
	for ( std::string dataDir : String::split( dataDirList, ':' ) ) {
		// A trailing separator would turn into a doubled one in addDataDirectory().
		while ( !dataDir.empty() && dataDir.back() == '/' )
			dataDir.pop_back();
		addDataDirectory( dataDir );
	}

	// The standard locations are always tried, even with a trimmed down environment.
	addDirectory( "/usr/local/share/applications" );
	addDirectory( "/usr/share/applications" );

	return dirs;
}

// Reads the [Desktop Entry] group of a .desktop file. Only the unlocalized keys are considered:
// "Key[lang]=..." only ever repeats an already seen "Key=" value.
static void parseDesktopEntry( const std::string& path, std::string& exec, std::string& wmClass,
							   std::string& icon, bool& hidden ) {
	std::ifstream file( path );
	if ( !file.is_open() )
		return;

	std::string line;
	bool inGroup = false;
	while ( std::getline( file, line ) ) {
		if ( !line.empty() && line.back() == '\r' )
			line.pop_back();

		if ( !line.empty() && line[0] == '[' ) {
			// Past the desktop entry group there is nothing to index.
			if ( inGroup )
				break;
			size_t end = line.find( ']' );
			inGroup =
				end != std::string::npos && line.compare( 0, end + 1, "[Desktop Entry]" ) == 0;
			continue;
		}

		if ( !inGroup || line.empty() || line[0] == '#' )
			continue;

		size_t separator = line.find( '=' );
		if ( separator == std::string::npos )
			continue;

		// Key and value are trimmed as views of the line, so no string is copied per entry line.
		const std::string_view lineView( line );
		const std::string_view key = String::trim( lineView.substr( 0, separator ), " \t\r\n" );
		if ( key.empty() || key.find( '[' ) != std::string_view::npos )
			continue;

		const std::string_view value = String::trim( lineView.substr( separator + 1 ), " \t\r\n" );
		if ( value.empty() )
			continue;

		if ( key == "Exec" ) {
			if ( exec.empty() )
				exec = value;
		} else if ( key == "StartupWMClass" ) {
			if ( wmClass.empty() )
				wmClass = value;
		} else if ( key == "Icon" ) {
			if ( icon.empty() )
				icon = value;
		} else if ( key == "Hidden" ) {
			// A hidden entry belongs to a removed or shadowed application.
			if ( value == "true" )
				hidden = true;
		}
	}
}

// Desktop ids are the desktop file basename without the ".desktop" suffix. An executable path
// rarely spells one out, so derive the plausible ids from it: the executable name without a common
// wrapper suffix, and the application directory it lives in (/usr/lib/firefox/firefox).
static std::vector<std::string> desktopIdCandidates( const std::string& exePath,
													 const std::string& exeName ) {
	std::vector<std::string> ids;

	auto addId = [&ids, &exeName]( std::string_view id ) {
		if ( id.empty() || id == exeName )
			return;
		for ( const std::string& existing : ids ) {
			if ( existing == id )
				return;
		}
		ids.emplace_back( id );
	};

	const std::string_view exeNameView( exeName );
	for ( const char* suffix : kExecutableSuffixes ) {
		if ( String::endsWith( exeName, suffix ) ) {
			addId( exeNameView.substr( 0, exeName.size() - strlen( suffix ) ) );
			break;
		}
	}

	// The directory holding the executable, which is where a few vendors keep the app id
	// (/usr/lib/firefox/firefox, /usr/lib/libreoffice/program/soffice).
	size_t lastSlash = exePath.find_last_of( '/' );
	if ( lastSlash != std::string::npos && lastSlash > 0 ) {
		const std::string parent = baseName( std::string_view( exePath ).substr( 0, lastSlash ) );
		if ( !isGenericDirName( parent ) )
			addId( parent );
	}

	return ids;
}

const std::string& ProcessIconResolver::lookupKey( const std::string& key ) const {
	if ( key.empty() )
		return mEmpty;

	auto it = mExecIndex.find( key );
	if ( it != mExecIndex.end() )
		return it->second;

	it = mIdIndex.find( key );
	if ( it != mIdIndex.end() )
		return it->second;

	return mEmpty;
}

const std::string& ProcessIconResolver::resolveIconName( const std::string& iconName ) {
	auto it = mIconPathIndex.find( iconName );
	if ( it != mIconPathIndex.end() )
		return it->second;

	std::string path = findIconFile( iconName );
	return mIconPathIndex.emplace( iconName, std::move( path ) ).first->second;
}

void ProcessIconResolver::scanDirectory( const std::string& dir ) {
	DIR* handle = opendir( dir.c_str() );
	if ( !handle )
		return;

	struct dirent* entry;
	while ( ( entry = readdir( handle ) ) != nullptr ) {
		const std::string fileName = entry->d_name;
		if ( !String::endsWith( fileName, ".desktop" ) )
			continue;

		// A directory named "something.desktop" is not a desktop entry, and neither is a file
		// that cannot be read.
		const std::string path = dir + "/" + fileName;
		if ( !FileInfo( path ).isRegularFile() )
			continue;

		indexDesktopFile( path );
	}

	closedir( handle );
}

void ProcessIconResolver::indexDesktopFile( const std::string& path ) {
	std::string exec, wmClass, icon;
	bool hidden = false;
	parseDesktopEntry( path, exec, wmClass, icon, hidden );
	if ( hidden || icon.empty() )
		return;

	// The desktop file id is the weakest key: it only matches an executable that happens to be
	// named after its desktop file, so it is kept apart and never shadows a real key.
	std::string id = baseName( path );
	if ( String::endsWith( id, ".desktop" ) )
		id.resize( id.size() - strlen( ".desktop" ) );
	if ( !id.empty() && mIdIndex.find( id ) == mIdIndex.end() )
		mIdIndex.emplace( id, icon );

	// First entry wins, which is the user's own one because it is scanned first.
	if ( !wmClass.empty() && mExecIndex.find( wmClass ) == mExecIndex.end() )
		mExecIndex.emplace( wmClass, icon );

	std::string executable = execToken( exec );
	if ( !executable.empty() && mExecIndex.find( executable ) == mExecIndex.end() )
		mExecIndex.emplace( executable, icon );
}

void ProcessIconResolver::buildIndex() {
	mIndexBuilt = true;

	for ( const std::string& dir : desktopDirectories() )
		scanDirectory( dir );

	if ( mExecIndex.empty() && mIdIndex.empty() ) {
		Log::warning( "eproc: no desktop entries found, process icons will be unavailable" );
	}
}

const std::string& ProcessIconResolver::iconFor( const std::string& exePath,
												 const std::string& name ) {
	std::lock_guard<std::mutex> lock( mMutex );

	if ( !mIndexBuilt )
		buildIndex();

	std::string cacheKey = exePath;
	cacheKey += '|';
	cacheKey += name;

	auto cached = mResultCache.find( cacheKey );
	if ( cached != mResultCache.end() )
		return cached->second;

	const std::string exeName = baseName( exePath );

	std::string iconName = lookupKey( exeName );
	if ( iconName.empty() ) {
		for ( const std::string& id : desktopIdCandidates( exePath, exeName ) ) {
			iconName = lookupKey( id );
			if ( !iconName.empty() )
				break;
		}
	}
	if ( iconName.empty() ) {
		// A blank name has no key to look up.
		const std::string_view trimmedName = String::trim( std::string_view( name ), " \t\r\n" );
		if ( !trimmedName.empty() )
			iconName = lookupKey( std::string( trimmedName ) );
	}

	if ( iconName.empty() )
		return mResultCache.emplace( std::move( cacheKey ), std::string() ).first->second;

	return mResultCache.emplace( std::move( cacheKey ), resolveIconName( iconName ) ).first->second;
}

} // namespace eproc
