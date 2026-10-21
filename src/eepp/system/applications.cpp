#include <eepp/system/applications.hpp>

#include <algorithm>
#include <eepp/core/containers.hpp>
#include <eepp/core/small_vector.hpp>
#include <eepp/core/string.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <mutex>

#if EE_PLATFORM == EE_PLATFORM_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined( EE_PLATFORM_POSIX )
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

using namespace EE;
using namespace EE::System;

namespace EE { namespace System {

namespace {

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
std::vector<std::string> xdgDirs( const char* homeVar, const char* homeDefault, const char* dirsVar,
								  const char* dirsDefault ) {
	std::vector<std::string> dirs;
	std::string home = Sys::getEnv( homeVar );
	if ( home.empty() ) {
		home = Sys::getUserDirectory();
		FileSystem::dirAddSlashAtEnd( home );
		home += homeDefault;
	}
	dirs.push_back( std::move( home ) );
	std::string others = Sys::getEnv( dirsVar );
	if ( others.empty() )
		others = dirsDefault;
	for ( auto& dir : String::split( others, ':' ) )
		dirs.push_back( std::move( dir ) );
	for ( auto& dir : dirs )
		FileSystem::dirAddSlashAtEnd( dir );
	return dirs;
}

/** Key=Value pairs of one [group] of an INI style file (desktop entries, mimeapps.list). */
SmallUnorderedMap<std::string, std::string, 16> readGroup( const std::string& data,
														   const std::string& group ) {
	SmallUnorderedMap<std::string, std::string, 16> values;
	bool inGroup = false;
	for ( auto& line : String::split( data, '\n' ) ) {
		String::trimInPlace( line );
		if ( line.empty() || line[0] == '#' )
			continue;
		if ( line[0] == '[' ) {
			inGroup = line == "[" + group + "]";
			continue;
		}
		if ( !inGroup )
			continue;
		size_t eq = line.find( '=' );
		if ( eq == std::string::npos )
			continue;
		std::string key = String::trim( line.substr( 0, eq ) );
		if ( !values.count( key ) )
			values[std::move( key )] = String::trim( line.substr( eq + 1 ) );
	}
	return values;
}

std::vector<std::string> splitList( const std::string& value ) {
	std::vector<std::string> out;
	for ( auto& item : String::split( value, ';' ) ) {
		if ( !item.empty() )
			out.push_back( item );
	}
	return out;
}

#endif

#if EE_PLATFORM != EE_PLATFORM_MACOS && EE_PLATFORM != EE_PLATFORM_WIN
std::string fileURI( const std::string& path ) {
	return URI::fromFSPath( path ).toString();
}
#endif

} // namespace

#if EE_PLATFORM != EE_PLATFORM_MACOS && EE_PLATFORM != EE_PLATFORM_WIN
struct Applications::Registry {
	std::mutex mutex;
	UnorderedMap<std::string, Application> apps;
	UnorderedMap<std::string, std::vector<std::string>> byMime;
	UnorderedMap<std::string, std::vector<std::string>> defaults;
	UnorderedMap<std::string, std::vector<std::string>> removed;
	bool loaded{ false };

	void load();
};

Applications::Registry& Applications::registry() {
	static Registry data;
	return data;
}

void Applications::Registry::load() {
	if ( loaded )
		return;
	loaded = true;
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
	auto dataDirs = Sys::getDataSearchPaths();
	for ( auto& dir : dataDirs )
		FileSystem::dirAddSlashAtEnd( dir );
	// Earlier directories win: desktop ids already seen are skipped.
	for ( const auto& base : dataDirs ) {
		std::string dir = base + "applications/";
		if ( !FileSystem::isDirectory( dir ) )
			continue;
		for ( const auto& file : FileSystem::filesGetInPath( dir ) ) {
			if ( !String::endsWith( file, ".desktop" ) || apps.count( file ) )
				continue;
			std::string data;
			if ( !FileSystem::fileGet( dir + file, data ) )
				continue;
			auto entry = readGroup( data, "Desktop Entry" );
			if ( entry["Type"] != "Application" || entry["Hidden"] == "true" ||
				 entry["Exec"].empty() || entry["Name"].empty() )
				continue;
			Application app;
			app.id = file;
			app.name = std::move( entry["Name"] );
			app.exec = std::move( entry["Exec"] );
			app.icon = std::move( entry["Icon"] );
			app.terminal = entry["Terminal"] == "true";
			apps.emplace( file, std::move( app ) );
			// NoDisplay apps still handle types (they are just not shown in menus).
			for ( const auto& mime : splitList( entry["MimeType"] ) )
				byMime[mime].push_back( file );
		}
	}
	// mimeapps.list: user config first, so its defaults come first.
	auto configDirs = xdgDirs( "XDG_CONFIG_HOME", ".config", "XDG_CONFIG_DIRS", "/etc/xdg" );
	std::vector<std::string> lists;
	for ( const auto& dir : configDirs )
		lists.push_back( dir + "mimeapps.list" );
	for ( const auto& dir : dataDirs )
		lists.push_back( dir + "applications/mimeapps.list" );
	for ( const auto& list : lists ) {
		std::string data;
		if ( !FileSystem::fileGet( list, data ) )
			continue;
		for ( const auto& group : { "Default Applications", "Added Associations" } ) {
			for ( const auto& [mime, value] : readGroup( data, group ) ) {
				auto& ids = defaults[mime];
				for ( const auto& id : splitList( value ) ) {
					if ( std::find( ids.begin(), ids.end(), id ) == ids.end() )
						ids.push_back( id );
				}
			}
		}
		for ( const auto& [mime, value] : readGroup( data, "Removed Associations" ) ) {
			for ( const auto& id : splitList( value ) )
				removed[mime].push_back( id );
		}
	}
#endif
}

#endif

#if EE_PLATFORM != EE_PLATFORM_MACOS && EE_PLATFORM != EE_PLATFORM_WIN
std::vector<Application> Applications::forFile( const URI&,
												const std::vector<std::string>& mimeTypes ) {
	auto& data = registry();
	std::lock_guard<std::mutex> lock( data.mutex );
	data.load();
	std::vector<Application> out;
	auto addId = [&]( const std::string& id, const std::string& type ) {
		auto app = data.apps.find( id );
		if ( app == data.apps.end() )
			return;
		auto removed = data.removed.find( type );
		if ( removed != data.removed.end() &&
			 std::find( removed->second.begin(), removed->second.end(), id ) !=
				 removed->second.end() )
			return;
		for ( const auto& existing : out ) {
			if ( existing.id == id )
				return;
		}
		out.push_back( app->second );
	};
	// Preferences of the exact type and its parents first, then every handler.
	for ( const auto& type : mimeTypes ) {
		auto defaults = data.defaults.find( type );
		if ( defaults != data.defaults.end() ) {
			for ( const auto& id : defaults->second )
				addId( id, type );
		}
	}
	for ( const auto& type : mimeTypes ) {
		auto handlers = data.byMime.find( type );
		if ( handlers != data.byMime.end() ) {
			for ( const auto& id : handlers->second )
				addId( id, type );
		}
	}
	return out;
}
#endif

std::optional<Application> Applications::defaultFor( const URI& uri,
													 const std::vector<std::string>& mimeTypes ) {
	auto apps = forFile( uri, mimeTypes );
	if ( apps.empty() )
		return std::nullopt;
	return std::move( apps.front() );
}

std::vector<std::string> Applications::splitExec( const std::string& exec ) {
	std::vector<std::string> args;
	std::string current;
	bool quoted = false, hasToken = false;
	for ( size_t i = 0; i < exec.size(); ++i ) {
		char c = exec[i];
		if ( quoted ) {
			if ( c == '\\' && i + 1 < exec.size() ) {
				current += exec[++i];
			} else if ( c == '"' ) {
				quoted = false;
			} else {
				current += c;
			}
		} else if ( c == '"' ) {
			quoted = true;
			hasToken = true;
		} else if ( c == ' ' || c == '\t' ) {
			if ( hasToken || !current.empty() )
				args.push_back( std::move( current ) );
			current.clear();
			hasToken = false;
		} else {
			current += c;
		}
	}
	if ( hasToken || !current.empty() )
		args.push_back( std::move( current ) );
	return args;
}

#if EE_PLATFORM != EE_PLATFORM_WIN
bool Applications::launch( const Application& app, const std::vector<std::string>& paths ) {
#if EE_PLATFORM == EE_PLATFORM_MACOS
	if ( app.exec.empty() || paths.empty() )
		return false;
	// Give Launch Services the bundle, rather than starting its executable directly. Argument
	// boundaries preserve spaces, Unicode and leading dashes without shell interpolation.
	std::vector<std::string> argv{ "/usr/bin/open", "-a", app.exec, "--" };
	argv.insert( argv.end(), paths.begin(), paths.end() );
	return spawn( argv );
#else
	std::vector<std::string> argv;
	bool filesUsed = false;
	bool singleFile = false;
	for ( const auto& arg : splitExec( app.exec ) ) {
		if ( arg == "%f" || arg == "%u" ) {
			// Single-file applications get one process per file below.
			singleFile = true;
			if ( !paths.empty() )
				argv.push_back( arg == "%u" ? fileURI( paths[0] ) : paths[0] );
			filesUsed = true;
			continue;
		}
		if ( arg == "%F" || arg == "%U" ) {
			for ( const auto& path : paths )
				argv.push_back( arg == "%U" ? fileURI( path ) : path );
			filesUsed = true;
			continue;
		}
		if ( arg == "%i" || arg == "%k" || arg == "%d" || arg == "%D" || arg == "%n" ||
			 arg == "%N" || arg == "%v" || arg == "%m" )
			continue;
		std::string expanded;
		for ( size_t i = 0; i < arg.size(); ++i ) {
			if ( arg[i] == '%' && i + 1 < arg.size() ) {
				char code = arg[++i];
				if ( code == '%' )
					expanded += '%';
				else if ( code == 'c' )
					expanded += app.name;
				continue;
			}
			expanded += arg[i];
		}
		argv.push_back( std::move( expanded ) );
	}
	if ( argv.empty() )
		return false;
	if ( !filesUsed )
		argv.insert( argv.end(), paths.begin(), paths.end() );
	if ( app.terminal ) {
		std::string terminal = Sys::getEnv( "TERMINAL" );
		if ( terminal.empty() )
			terminal = Sys::which( "konsole" ).empty() ? "xterm" : "konsole";
		argv.insert( argv.begin(), { terminal, "-e" } );
	}
	if ( !spawn( argv ) )
		return false;
	// Both %f and %u handlers take one file per invocation.
	if ( singleFile ) {
		for ( size_t i = 1; i < paths.size(); ++i ) {
			if ( !launch( app, { paths[i] } ) )
				return false;
		}
	}
	return true;
#endif
}
#endif

bool Applications::launchCommand( const std::string& command,
								  const std::vector<std::string>& paths ) {
	auto argv = splitExec( command );
	if ( argv.empty() )
		return false;
	argv.insert( argv.end(), paths.begin(), paths.end() );
	return spawn( argv );
}

bool Applications::spawn( const std::vector<std::string>& argv, const std::string& workingDir,
						  bool console ) {
	if ( argv.empty() )
		return false;
#if EE_PLATFORM == EE_PLATFORM_WIN
	std::string commandLine;
	for ( const auto& arg : argv ) {
		if ( !commandLine.empty() )
			commandLine += ' ';
		commandLine += '"';
		size_t slashes = 0;
		for ( char c : arg ) {
			if ( c == '\\' ) {
				++slashes;
				continue;
			}
			commandLine.append( c == '"' ? slashes * 2 + 1 : slashes, '\\' );
			commandLine += c;
			slashes = 0;
		}
		commandLine.append( slashes * 2, '\\' );
		commandLine += '"';
	}
	std::wstring wide = String( commandLine ).toWideString();
	std::wstring dir = String( workingDir ).toWideString();
	STARTUPINFOW si{};
	si.cb = sizeof( si );
	PROCESS_INFORMATION pi{};
	// A new process group would start with Ctrl+C disabled: shells get their own console instead,
	// which already keeps them apart from the caller.
	DWORD flags = console ? CREATE_NEW_CONSOLE : DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP;
	if ( !CreateProcessW( nullptr, wide.data(), nullptr, nullptr, FALSE, flags, nullptr,
						  dir.empty() ? nullptr : dir.c_str(), &si, &pi ) )
		return false;
	CloseHandle( pi.hThread );
	CloseHandle( pi.hProcess );
	return true;
#elif defined( EE_PLATFORM_POSIX ) && EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN && \
	EE_PLATFORM != EE_PLATFORM_IOS && EE_PLATFORM != EE_PLATFORM_ANDROID
	(void)console;
	SmallVector<char*, 16> args;
	args.reserve( argv.size() + 1 );
	for ( const auto& arg : argv )
		args.push_back( const_cast<char*>( arg.c_str() ) );
	args.push_back( nullptr );
	int pipeFds[2];
	if ( pipe( pipeFds ) != 0 )
		return false;
	if ( fcntl( pipeFds[1], F_SETFD, FD_CLOEXEC ) == -1 ) {
		close( pipeFds[0] );
		close( pipeFds[1] );
		return false;
	}
	// Double fork: the grandchild is adopted by init, so no zombie is left behind. The pipe
	// reports whether exec succeeded.
	pid_t child = fork();
	if ( child < 0 ) {
		close( pipeFds[0] );
		close( pipeFds[1] );
		return false;
	}
	if ( child == 0 ) {
		close( pipeFds[0] );
		setsid();
		pid_t grandchild = fork();
		if ( grandchild != 0 )
			_exit( grandchild < 0 ? 1 : 0 );
		if ( workingDir.empty() || chdir( workingDir.c_str() ) == 0 )
			execvp( args[0], args.data() );
		char failed = 1;
		ssize_t written = write( pipeFds[1], &failed, 1 );
		(void)written;
		_exit( 127 );
	}
	close( pipeFds[1] );
	int status = 0;
	pid_t waited;
	do {
		waited = waitpid( child, &status, 0 );
	} while ( waited < 0 && errno == EINTR );
	char failed = 0;
	ssize_t got;
	do {
		got = read( pipeFds[0], &failed, 1 );
	} while ( got < 0 && errno == EINTR );
	close( pipeFds[0] );
	return waited == child && WIFEXITED( status ) && WEXITSTATUS( status ) == 0 && got == 0;
#else
	(void)workingDir;
	(void)console;
	return false;
#endif
}

}} // namespace EE::System
