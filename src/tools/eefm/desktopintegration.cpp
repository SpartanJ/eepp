#include "desktopintegration.hpp"

#include <eepp/core/string.hpp>
#include <eepp/system/applications.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>

#if defined( EE_PLATFORM_POSIX )
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

using namespace EE;
using namespace EE::System;

namespace eefm {

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

bool DesktopIntegration::openTerminal( const std::string& directory ) {
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
	if ( !eterm.empty() && Applications::spawn( { eterm, directory }, directory ) )
		return true;
#if EE_PLATFORM == EE_PLATFORM_WIN
	// A console of its own: Windows opens it in the default terminal (Windows Terminal or the
	// console host, as set in the system settings).
	std::string powershell = Sys::which( "powershell.exe" );
	return Applications::spawn( { powershell.empty() ? "cmd.exe" : powershell }, directory, true );
#elif EE_PLATFORM == EE_PLATFORM_MACOS
	return Applications::spawn( { "open", "-a", "Terminal", directory } );
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
		if ( Applications::spawn( { candidate }, directory ) )
			return true;
	}
	return false;
#endif
}

DesktopIntegration::ExecutableKind DesktopIntegration::executableKind( const std::string& path ) {
#if defined( EE_PLATFORM_POSIX ) && EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN && \
	EE_PLATFORM != EE_PLATFORM_IOS && EE_PLATFORM != EE_PLATFORM_ANDROID
	struct stat info;
	if ( stat( path.c_str(), &info ) != 0 || !S_ISREG( info.st_mode ) ||
		 access( path.c_str(), X_OK ) != 0 )
		return ExecutableKind::None;
	int fd = open( path.c_str(), O_RDONLY | O_CLOEXEC );
	if ( fd < 0 )
		return ExecutableKind::None;
	unsigned char header[4]{};
	const ssize_t size = read( fd, header, sizeof( header ) );
	close( fd );
	if ( size >= 2 && header[0] == '#' && header[1] == '!' )
		return ExecutableKind::Script;
	if ( size < 4 )
		return ExecutableKind::None;
	static const unsigned char programMagic[][4] = {
		{ 0x7f, 'E', 'L', 'F' },								// ELF
		{ 0xfe, 0xed, 0xfa, 0xce }, { 0xce, 0xfa, 0xed, 0xfe }, // Mach-O 32-bit
		{ 0xfe, 0xed, 0xfa, 0xcf }, { 0xcf, 0xfa, 0xed, 0xfe }, // Mach-O 64-bit
		{ 0xca, 0xfe, 0xba, 0xbe } };							// Mach-O universal
	for ( const auto& magic : programMagic ) {
		if ( std::memcmp( header, magic, sizeof( header ) ) == 0 )
			return ExecutableKind::Program;
	}
	return ExecutableKind::None;
#else
	(void)path;
	return ExecutableKind::None;
#endif
}

} // namespace eefm
