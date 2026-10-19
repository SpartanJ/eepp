#include "trash.hpp"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <eepp/core/string.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// windows.h first: shellapi.h depends on it.
#include <windows.h>

#include <shellapi.h>
#elif defined( EE_PLATFORM_POSIX )
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

using namespace EE;
using namespace EE::System;

namespace eefm {

namespace {

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD

constexpr bool SpecTrash = true;

std::string homeTrash() {
	return Sys::getDataPath( "Trash" ) + "/";
}

/** Percent-encodes a path for the Path= key of a .trashinfo file. */
std::string encodePath( const std::string& path ) {
	static constexpr char Hex[] = "0123456789ABCDEF";
	std::string out;
	for ( unsigned char c : path ) {
		if ( std::isalnum( c ) || c == '/' || c == '-' || c == '_' || c == '.' || c == '~' ) {
			out += static_cast<char>( c );
		} else {
			out += '%';
			out += Hex[c >> 4];
			out += Hex[c & 15];
		}
	}
	return out;
}

std::string decodePath( const std::string& value ) {
	std::string out;
	for ( size_t i = 0; i < value.size(); ++i ) {
		if ( value[i] == '%' && i + 2 < value.size() ) {
			int hi = std::isdigit( value[i + 1] ) ? value[i + 1] - '0'
												  : ( std::toupper( value[i + 1] ) - 'A' + 10 );
			int lo = std::isdigit( value[i + 2] ) ? value[i + 2] - '0'
												  : ( std::toupper( value[i + 2] ) - 'A' + 10 );
			out += static_cast<char>( hi * 16 + lo );
			i += 2;
		} else {
			out += value[i];
		}
	}
	return out;
}

/** Mount point of path: the topmost ancestor on the same device. */
std::string mountPointOf( const std::string& path ) {
	struct stat st{};
	if ( lstat( path.c_str(), &st ) != 0 )
		return {};
	dev_t device = st.st_dev;
	std::string current = path;
	while ( current.size() > 1 ) {
		std::string parent = FileSystem::fileRemoveFileName( current );
		if ( parent.size() > 1 && parent.back() == '/' )
			parent.pop_back();
		if ( parent.empty() )
			parent = "/";
		struct stat pst{};
		if ( stat( parent.c_str(), &pst ) != 0 || pst.st_dev != device )
			break;
		if ( parent == current )
			break;
		current = parent;
	}
	return current;
}

/** Picks a free name in trash/files and atomically reserves it with its info file. */
std::string writeInfo( const std::string& trashDir, const std::string& name,
					   const std::string& recordedPath, std::string& trashedName ) {
	auto [base, extension] = FileSystem::fileNameSplitExtension( name );
	char date[32];
	std::time_t now = std::time( nullptr );
	std::tm local{};
	localtime_r( &now, &local );
	std::strftime( date, sizeof( date ), "%Y-%m-%dT%H:%M:%S", &local );
	std::string content =
		"[Trash Info]\nPath=" + encodePath( recordedPath ) + "\nDeletionDate=" + date + "\n";
	for ( int i = 0; i < 10000; ++i ) {
		std::string candidate = i == 0 ? name : base + "." + String::toString( i + 1 ) + extension;
		std::string infoPath = trashDir + "info/" + candidate + ".trashinfo";
		// O_EXCL on the info file is the reservation (the spec's locking scheme).
		int fd = open( infoPath.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0600 );
		if ( fd < 0 ) {
			if ( errno == EEXIST )
				continue;
			return std::strerror( errno );
		}
		if ( FileSystem::fileExists( trashDir + "files/" + candidate ) ) {
			close( fd );
			unlink( infoPath.c_str() );
			continue;
		}
		bool ok =
			write( fd, content.data(), content.size() ) == static_cast<ssize_t>( content.size() );
		close( fd );
		if ( !ok ) {
			unlink( infoPath.c_str() );
			return "Could not write the trash information file";
		}
		trashedName = candidate;
		return {};
	}
	return "Too many files with the same name in the trash";
}

std::string trashInto( const std::string& trashDir, const std::string& path,
					   const std::string& recordedPath ) {
	FileSystem::makeDir( trashDir + "files", true, 0700 );
	FileSystem::makeDir( trashDir + "info", true, 0700 );
	std::string name = FileSystem::fileNameFromPath( path );
	std::string trashedName;
	std::string error = writeInfo( trashDir, name, recordedPath, trashedName );
	if ( !error.empty() )
		return error;
	if ( ::rename( path.c_str(), ( trashDir + "files/" + trashedName ).c_str() ) != 0 ) {
		int code = errno;
		unlink( ( trashDir + "info/" + trashedName + ".trashinfo" ).c_str() );
		errno = code;
		return std::strerror( code );
	}
	return {};
}

/** Trash folder for a path that is not on the home trash's file system. */
std::string topDirTrash( const std::string& path, std::string& recordedPath ) {
	std::string top = mountPointOf( path );
	if ( top.empty() )
		return {};
	std::string uid = String::toString( static_cast<Uint64>( getuid() ) );
	std::string topSlash = top == "/" ? top : top + "/";
	// $topdir/.Trash/$uid (only when .Trash is a sticky, non-symlink directory)...
	struct stat st{};
	std::string shared = topSlash + ".Trash";
	if ( lstat( shared.c_str(), &st ) == 0 && S_ISDIR( st.st_mode ) && ( st.st_mode & S_ISVTX ) ) {
		std::string dir = shared + "/" + uid + "/";
		FileSystem::makeDir( dir, true, 0700 );
		if ( FileSystem::isDirectory( dir ) ) {
			recordedPath = path.substr( topSlash.size() );
			return dir;
		}
	}
	// ...otherwise $topdir/.Trash-$uid.
	std::string dir = topSlash + ".Trash-" + uid + "/";
	FileSystem::makeDir( dir, true, 0700 );
	if ( !FileSystem::isDirectory( dir ) )
		return {};
	recordedPath = path.substr( topSlash.size() );
	return dir;
}

#else

constexpr bool SpecTrash = false;

#endif

std::string absolute( std::string path ) {
	while ( path.size() > 1 && ( path.back() == '/' || path.back() == '\\' ) )
		path.pop_back();
	return path;
}

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
/** .trashinfo file of a trashed entry ("<trash>/files/<name>" -> "<trash>/info/<name>.trashinfo").
 */
std::string infoPathOf( const std::string& trashedPath ) {
	std::string path = absolute( trashedPath );
	std::string trashDir =
		FileSystem::fileRemoveFileName( absolute( FileSystem::fileRemoveFileName( path ) ) );
	return trashDir + "info/" + FileSystem::fileNameFromPath( path ) + ".trashinfo";
}
#endif

/** Runs action on each path. @return One "name: error" line per failure. */
template <typename Action>
std::string forEachPath( const std::vector<std::string>& paths, Action action ) {
	std::string errors;
	for ( const auto& path : paths ) {
		std::string error = action( path );
		if ( !error.empty() )
			errors += FileSystem::fileNameFromPath( path ) + ": " + error + "\n";
	}
	return errors;
}

} // namespace

bool Trash::supported( const URI& uri ) {
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD || \
	EE_PLATFORM == EE_PLATFORM_MACOS || EE_PLATFORM == EE_PLATFORM_WIN
	return uri.getScheme() == "file" && !contains( uri );
#else
	(void)uri;
	return false;
#endif
}

std::optional<URI> Trash::location() {
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
	return URI::fromFSPath( homeTrash() + "files/" );
#elif EE_PLATFORM == EE_PLATFORM_MACOS
	std::string home = Sys::getUserDirectory();
	FileSystem::dirAddSlashAtEnd( home );
	return URI::fromFSPath( home + ".Trash/" );
#else
	return std::nullopt;
#endif
}

bool Trash::contains( const URI& uri ) {
	auto trash = location();
	if ( !trash || uri.getScheme() != "file" )
		return false;
	std::string path = uri.getFSPath();
	std::string root = trash->getFSPath();
	return String::startsWith( path, root ) || String::startsWith( path + "/", root ) ||
		   path.find( "/.Trash-" ) != std::string::npos;
}

std::string Trash::moveToTrash( const std::string& rawPath ) {
	std::string path = absolute( rawPath );
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
	std::string error = trashInto( homeTrash(), path, path );
	if ( error.empty() )
		return {};
	if ( errno != EXDEV )
		return error;
	std::string recorded;
	std::string topTrash = topDirTrash( path, recorded );
	if ( topTrash.empty() )
		return "This file system has no trash";
	return trashInto( topTrash, path, recorded );
#elif EE_PLATFORM == EE_PLATFORM_MACOS
	std::string target = location()->getFSPath();
	FileSystem::makeDir( target, true );
	std::string name = FileSystem::fileNameFromPath( path );
	std::string destination = target + name;
	for ( int i = 2; FileSystem::fileExists( destination ) && i < 10000; ++i )
		destination = target + name + " " + String::toString( i );
	if ( ::rename( path.c_str(), destination.c_str() ) != 0 )
		return std::strerror( errno );
	return {};
#elif EE_PLATFORM == EE_PLATFORM_WIN
	std::wstring from = String( path ).toWideString();
	from.push_back( L'\0' ); // Double null terminated list.
	SHFILEOPSTRUCTW op{};
	op.wFunc = FO_DELETE;
	op.pFrom = from.c_str();
	op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
	int result = SHFileOperationW( &op );
	if ( result != 0 || op.fAnyOperationsAborted )
		return "Could not move to the Recycle Bin (error " + String::toString( result ) + ")";
	return {};
#else
	(void)path;
	return "The trash is not supported on this platform";
#endif
}

std::optional<std::string> Trash::originalPath( const std::string& trashedPath ) {
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
	std::string infoPath = infoPathOf( trashedPath );
	std::string trashDir =
		FileSystem::fileRemoveFileName( absolute( FileSystem::fileRemoveFileName( infoPath ) ) );
	std::string info;
	if ( !FileSystem::fileGet( infoPath, info ) )
		return std::nullopt;
	for ( const auto& line : String::split( info, '\n' ) ) {
		if ( !String::startsWith( line, "Path=" ) )
			continue;
		std::string original = decodePath( line.substr( 5 ) );
		if ( !original.empty() && original[0] != '/' ) {
			// Relative to the top directory of a $topdir/.Trash-$uid trash.
			std::string top = absolute( trashDir );
			size_t marker = top.find( "/.Trash" );
			if ( marker != std::string::npos )
				original = top.substr( 0, marker + 1 ) + original;
		}
		return original;
	}
	return std::nullopt;
#else
	(void)trashedPath;
	return std::nullopt;
#endif
}

std::string Trash::restore( const std::string& trashedPath ) {
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
	std::string path = absolute( trashedPath );
	auto original = originalPath( path );
	if ( !original )
		return "The original location of this item is unknown";
	if ( FileSystem::fileExists( *original ) )
		return "\"" + *original + "\" already exists";
	std::string parent = FileSystem::fileRemoveFileName( *original );
	if ( !parent.empty() )
		FileSystem::makeDir( parent, true );
	if ( ::rename( path.c_str(), original->c_str() ) != 0 )
		return std::strerror( errno );
	forget( path );
	return {};
#else
	(void)trashedPath;
	return "Restoring is not supported on this platform";
#endif
}

std::string Trash::moveToTrash( const std::vector<std::string>& paths ) {
	return forEachPath( paths, []( const std::string& path ) { return moveToTrash( path ); } );
}

std::string Trash::restore( const std::vector<std::string>& trashedPaths ) {
	return forEachPath( trashedPaths, []( const std::string& path ) { return restore( path ); } );
}

void Trash::forget( const std::string& trashedPath ) {
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
	unlink( infoPathOf( trashedPath ).c_str() );
#else
	(void)trashedPath;
#endif
}

std::string Trash::empty() {
	if constexpr ( !SpecTrash ) {
		auto trash = location();
		if ( !trash )
			return "The trash is not supported on this platform";
		std::string dir = trash->getFSPath();
		for ( const auto& name : FileSystem::filesGetInPath( dir ) ) {
			if ( FileSystem::isDirectory( dir + name ) )
				FileSystem::dirRemoveAll( dir + name );
			else
				FileSystem::fileRemove( dir + name );
		}
		return {};
	}
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
	std::string trash = homeTrash();
	for ( const char* sub : { "files/", "info/" } ) {
		std::string dir = trash + sub;
		for ( const auto& name : FileSystem::filesGetInPath( dir ) ) {
			std::string entry = dir + name;
			// Never follow a symlinked directory out of the trash.
			struct stat st{};
			if ( lstat( entry.c_str(), &st ) == 0 && S_ISDIR( st.st_mode ) )
				FileSystem::dirRemoveAll( entry );
			else
				FileSystem::fileRemove( entry );
		}
	}
	FileSystem::fileRemove( trash + "directorysizes" );
#endif
	return {};
}

} // namespace eefm
