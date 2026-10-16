#include "apps.hpp"
#include "mime.hpp"
#include <algorithm>
#include <eepp/core/string.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined( EE_PLATFORM_POSIX )
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

using namespace EE;
using namespace EE::System;

namespace eefm {

namespace {

std::vector<std::string> xdgDirs( const char* homeVar, const char* homeDefault, const char* dirsVar,
								  const char* dirsDefault ) {
	std::vector<std::string> dirs;
	std::string home = Sys::getEnv( homeVar );
	if ( home.empty() ) {
		home = Sys::getUserDirectory();
		FileSystem::dirAddSlashAtEnd( home );
		home += homeDefault;
	}
	dirs.push_back( home );
	std::string others = Sys::getEnv( dirsVar );
	if ( others.empty() )
		others = dirsDefault;
	for ( const auto& dir : String::split( others, ':' ) )
		dirs.push_back( dir );
	for ( auto& dir : dirs )
		FileSystem::dirAddSlashAtEnd( dir );
	return dirs;
}

/** Key=Value pairs of one [group] of an INI style file (desktop entries, mimeapps.list). */
std::unordered_map<std::string, std::string> readGroup( const std::string& data,
														const std::string& group ) {
	std::unordered_map<std::string, std::string> values;
	bool inGroup = false;
	for ( auto line : String::split( data, '\n' ) ) {
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
			values[key] = String::trim( line.substr( eq + 1 ) );
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

std::string fileURI( const std::string& path ) {
	return URI::fromFSPath( path ).toString();
}

} // namespace

Applications& Applications::instance() {
	static Applications apps;
	return apps;
}

void Applications::load() {
	if ( mLoaded )
		return;
	mLoaded = true;
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
			if ( !String::endsWith( file, ".desktop" ) || mApps.count( file ) )
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
			app.name = entry["Name"];
			app.exec = entry["Exec"];
			app.terminal = entry["Terminal"] == "true";
			mApps[file] = app;
			// NoDisplay apps still handle types (they are just not shown in menus).
			for ( const auto& mime : splitList( entry["MimeType"] ) )
				mByMime[mime].push_back( file );
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
				auto& ids = mDefaults[mime];
				for ( const auto& id : splitList( value ) ) {
					if ( std::find( ids.begin(), ids.end(), id ) == ids.end() )
						ids.push_back( id );
				}
			}
		}
		for ( const auto& [mime, value] : readGroup( data, "Removed Associations" ) ) {
			for ( const auto& id : splitList( value ) )
				mRemoved[mime].push_back( id );
		}
	}
#endif
}

std::vector<Application> Applications::forMime( const std::string& mime ) {
	auto types = MimeDatabase::instance().withParents( mime );
	std::lock_guard<std::mutex> lock( mMutex );
	load();
	std::vector<Application> out;
	auto addId = [&]( const std::string& id, const std::string& type ) {
		auto app = mApps.find( id );
		if ( app == mApps.end() )
			return;
		auto removed = mRemoved.find( type );
		if ( removed != mRemoved.end() && std::find( removed->second.begin(), removed->second.end(),
													 id ) != removed->second.end() )
			return;
		for ( const auto& existing : out ) {
			if ( existing.id == id )
				return;
		}
		out.push_back( app->second );
	};
	// Preferences of the exact type and its parents first, then every handler.
	for ( const auto& type : types ) {
		auto defaults = mDefaults.find( type );
		if ( defaults != mDefaults.end() ) {
			for ( const auto& id : defaults->second )
				addId( id, type );
		}
	}
	for ( const auto& type : types ) {
		auto handlers = mByMime.find( type );
		if ( handlers != mByMime.end() ) {
			for ( const auto& id : handlers->second )
				addId( id, type );
		}
	}
	return out;
}

std::optional<Application> Applications::defaultFor( const std::string& mime ) {
	auto apps = forMime( mime );
	if ( apps.empty() )
		return std::nullopt;
	return apps.front();
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
				args.push_back( current );
			current.clear();
			hasToken = false;
		} else {
			current += c;
		}
	}
	if ( hasToken || !current.empty() )
		args.push_back( current );
	return args;
}

bool Applications::launch( const Application& app, const std::vector<std::string>& paths ) {
	std::vector<std::string> argv;
	bool filesUsed = false;
	for ( const auto& arg : splitExec( app.exec ) ) {
		if ( arg == "%f" || arg == "%u" ) {
			// Single-file applications get one process per file below.
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
		argv.push_back( expanded );
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
	bool ok = spawn( argv );
	// "%f" applications take one file per process.
	if ( ok && paths.size() > 1 && app.exec.find( "%f" ) != std::string::npos ) {
		for ( size_t i = 1; i < paths.size(); ++i )
			launch( app, { paths[i] } );
	}
	return ok;
}

bool Applications::launchCommand( const std::string& command,
								  const std::vector<std::string>& paths ) {
	auto argv = splitExec( command );
	if ( argv.empty() )
		return false;
	argv.insert( argv.end(), paths.begin(), paths.end() );
	return spawn( argv );
}

namespace {

#if EE_PLATFORM != EE_PLATFORM_WIN && EE_PLATFORM != EE_PLATFORM_MACOS
/** The terminal set in KDE's settings (kdeglobals, [General] TerminalApplication). */
std::string kdeTerminal() {
	std::string config = Sys::getEnv( "XDG_CONFIG_HOME" );
	if ( config.empty() )
		config = Sys::getUserDirectory() + "/.config";
	std::string contents;
	if ( !FileSystem::fileGet( config + "/kdeglobals", contents ) )
		return {};
	bool general = false;
	for ( auto line : String::split( contents, '\n' ) ) {
		line = String::trim( line );
		if ( !line.empty() && line[0] == '[' )
			general = line == "[General]";
		else if ( general && String::startsWith( line, "TerminalApplication=" ) )
			return String::trim( line.substr( strlen( "TerminalApplication=" ) ) );
	}
	return {};
}
#endif

} // namespace

bool Applications::openTerminal( const std::string& directory ) {
	// eepp's own terminal when installed (or built next to eefm); the directory is also passed
	// as its working directory argument, over the one in its config.
	std::string eterm = Sys::which( "eterm" );
	if ( eterm.empty() ) {
		std::string sibling = Sys::getProcessPath() + "eterm";
#if EE_PLATFORM == EE_PLATFORM_WIN
		sibling += ".exe";
#endif
		if ( FileSystem::fileExists( sibling ) )
			eterm = sibling;
	}
	if ( !eterm.empty() && spawn( { eterm, directory }, directory ) )
		return true;
#if EE_PLATFORM == EE_PLATFORM_WIN
	// A console of its own: Windows opens it in the default terminal (Windows Terminal or the
	// console host, as set in the system settings).
	std::string powershell = Sys::which( "powershell.exe" );
	return spawn( { powershell.empty() ? "cmd.exe" : powershell }, directory, true );
#elif EE_PLATFORM == EE_PLATFORM_MACOS
	return spawn( { "open", "-a", "Terminal", directory } );
#else
	// Otherwise the user's default terminal: $TERMINAL, the desktop's own setting and default,
	// the distribution's alternative, then the common ones.
	std::vector<std::string> candidates;
	std::string preferred = Sys::getEnv( "TERMINAL" );
	if ( !preferred.empty() )
		candidates.push_back( preferred );
	candidates.push_back( "xdg-terminal-exec" );
	std::string desktop = String::toLower( Sys::getEnv( "XDG_CURRENT_DESKTOP" ) );
	if ( desktop.find( "kde" ) != std::string::npos ) {
		std::string kde = kdeTerminal();
		if ( !kde.empty() )
			candidates.push_back( kde );
		candidates.push_back( "konsole" );
	} else if ( desktop.find( "gnome" ) != std::string::npos ) {
		candidates.insert( candidates.end(), { "ptyxis", "kgx", "gnome-terminal" } );
	} else if ( desktop.find( "xfce" ) != std::string::npos ) {
		candidates.push_back( "xfce4-terminal" );
	} else if ( desktop.find( "mate" ) != std::string::npos ) {
		candidates.push_back( "mate-terminal" );
	} else if ( desktop.find( "lxqt" ) != std::string::npos ) {
		candidates.push_back( "qterminal" );
	}
	candidates.push_back( "x-terminal-emulator" );
	for ( const char* name : { "konsole", "gnome-terminal", "kgx", "xfce4-terminal", "alacritty",
							   "kitty", "foot", "wezterm", "xterm" } )
		candidates.push_back( name );
	for ( const auto& candidate : candidates ) {
		if ( candidate.find( '/' ) == std::string::npos && Sys::which( candidate ).empty() )
			continue;
		if ( spawn( { candidate }, directory ) )
			return true;
	}
	return false;
#endif
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
		commandLine += '"' + arg + '"';
	}
	std::wstring wide = String( commandLine ).toWideString();
	std::wstring dir = String( workingDir ).toWideString();
	STARTUPINFOW si{};
	si.cb = sizeof( si );
	PROCESS_INFORMATION pi{};
	// A new process group would start with Ctrl+C disabled: shells get their own console instead,
	// which already keeps them apart from eefm.
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
	std::vector<char*> args;
	for ( const auto& arg : argv )
		args.push_back( const_cast<char*>( arg.c_str() ) );
	args.push_back( nullptr );
	int pipeFds[2];
	if ( pipe( pipeFds ) != 0 )
		return false;
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
		fcntl( pipeFds[1], F_SETFD, FD_CLOEXEC );
		if ( !workingDir.empty() && chdir( workingDir.c_str() ) != 0 ) {
		}
		execvp( args[0], args.data() );
		char failed = 1;
		ssize_t written = write( pipeFds[1], &failed, 1 );
		(void)written;
		_exit( 127 );
	}
	close( pipeFds[1] );
	int status = 0;
	waitpid( child, &status, 0 );
	char failed = 0;
	ssize_t got = read( pipeFds[0], &failed, 1 );
	close( pipeFds[0] );
	return WIFEXITED( status ) && WEXITSTATUS( status ) == 0 && got <= 0;
#else
	(void)workingDir;
	(void)console;
	return false;
#endif
}

} // namespace eefm
