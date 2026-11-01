#include "shellintegration.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>

using namespace EE;
using namespace EE::System;

namespace eterm { namespace System {

// Shared by Unix PTYs and ConPTY. Let PowerShell load its profiles normally, then install the
// hook through -Command. Commands, scripts and unrecognized startup options remain untouched.
static std::string powerShellIntegrationCommand( const std::string& program,
												 const std::vector<std::string>& args ) {
	const auto shell = String::toLower( FileSystem::fileNameFromPath( program ) );
	if ( shell != "pwsh" && shell != "pwsh.exe" && shell != "powershell" &&
		 shell != "powershell.exe" )
		return {};
	for ( const auto& argument : args ) {
		const auto option = String::toLower( argument );
		if ( option != "-nologo" && option != "-noprofile" && option != "-noexit" &&
			 option != "-login" && option != "-l" )
			return {};
	}
	auto script = Sys::getResourcePath() + "assets/shell-integration/powershell.ps1";
	if ( !FileSystem::fileExists( script ) )
		return {};
	// PowerShell single-quoted literals escape apostrophes by doubling them. Forward slashes
	// also keep Windows paths independent of the command-line quoting rules for backslashes.
	std::string command = "try { . '";
	for ( char character : script ) {
		if ( character == '\'' )
			command += '\'';
#ifdef _WIN32
		if ( character == '\\' )
			character = '/';
#endif
		command += character;
	}
	command += "' } catch {}"; // An execution policy may disallow scripts; keep the shell usable.
	return command;
}

#ifndef _WIN32
// Fish only enables its built-in OSC 7 hook for a terminal allowlist. Install ours after the
// user's config, without changing any files or relying on separately packaged shell resources.
// --init-command also runs for `fish -c`, so explicitly leave noninteractive commands alone.
static constexpr char FishDirectoryIntegration[] = R"fish(
if status is-interactive; and test "$ETERM_SHELL_INTEGRATION" != 0
    function __eterm_report_working_directory --on-variable PWD
        if not status is-command-substitution; and not set -q INSIDE_EMACS
            printf '\e]7;file://localhost%s\a' (string escape --style=url -- $PWD)
        end
    end
    __eterm_report_working_directory
end
)fish";

enum class UnixIntegration { None, Fish, Bash, BashRc, Zsh };

static UnixIntegration
prepareUnixEnvironment( const std::string& executable, const std::vector<std::string>& args,
						std::unordered_map<std::string, std::string>& environment ) {
	const auto shell = FileSystem::fileNameFromPath( executable );
	if ( shell == "fish" )
		return UnixIntegration::Fish;
	if ( shell != "bash" && shell != "zsh" )
		return UnixIntegration::None;
	// Only inject into ordinary interactive launches. Commands, scripts, POSIX mode and custom
	// startup options keep their exact invocation instead of guessing how to wrap them.
	bool noProfile = false;
	bool noRc = false;
	bool login = false;
	std::string rcFile;
	for ( size_t i = 0; i < args.size(); ++i ) {
		const auto& arg = args[i];
		if ( arg == "--login" || arg == "--interactive" ) {
			login |= arg == "--login";
			continue;
		}
		if ( arg.size() > 1 && arg[0] == '-' && arg[1] != '-' &&
			 arg.find_first_not_of( "il", 1 ) == std::string::npos ) {
			login |= arg.find( 'l' ) != std::string::npos;
			continue;
		}
		if ( shell == "bash" ) {
			if ( arg == "--noprofile" ) {
				noProfile = true;
				continue;
			}
			if ( arg == "--norc" ) {
				noRc = true;
				continue;
			}
			if ( ( arg == "--rcfile" || arg == "--init-file" ) && i + 1 < args.size() ) {
				rcFile = args[++i];
				continue;
			}
		}
		return UnixIntegration::None;
	}
	const auto directory = Sys::getResourcePath() + "assets/shell-integration";
	if ( !FileSystem::fileExists( directory + "/directory-reporting.sh" ) )
		return UnixIntegration::None;
	if ( shell == "bash" ) {
		if ( environment.count( "POSIXLY_CORRECT" ) ||
			 !FileSystem::fileExists( directory + "/bash.bash" ) )
			return UnixIntegration::None;
#if EE_PLATFORM == EE_PLATFORM_MACOS
		// Apple's Bash 3.2 cannot bootstrap login shells through ENV. Keep that invocation
		// intact; ordinary interactive sessions still use --rcfile on every Bash version.
		if ( login && executable == "/bin/bash" )
			return UnixIntegration::None;
#endif
		if ( login ) {
			environment["ETERM_BASH_ENV_SET"] = environment.count( "ENV" ) ? "1" : "0";
			environment["ETERM_BASH_ORIGINAL_ENV"] = environment["ENV"];
			environment["ENV"] = directory + "/bash.bash";
			environment["ETERM_BASH_POSIX_BOOTSTRAP"] = "1";
		}
		environment["ETERM_BASH_NOPROFILE"] = noProfile ? "1" : "0";
		environment["ETERM_BASH_NORC"] = noRc ? "1" : "0";
		environment["ETERM_BASH_RCFILE"] = std::move( rcFile );
	} else {
		if ( !FileSystem::fileExists( directory + "/zsh/.zshrc" ) )
			return UnixIntegration::None;
		environment["ETERM_ZDOTDIR_SET"] = environment.count( "ZDOTDIR" ) ? "1" : "0";
		environment["ETERM_ORIGINAL_ZDOTDIR"] = environment["ZDOTDIR"];
		environment["ZDOTDIR"] = directory + "/zsh";
	}
	environment["ETERM_INTEGRATION_DIR"] = directory;
	return shell == "bash" ? ( login ? UnixIntegration::Bash : UnixIntegration::BashRc )
						   : UnixIntegration::Zsh;
}

#endif

std::vector<std::string>
ShellIntegration::prepare( const std::string& executable, const std::vector<std::string>& args,
						   std::unordered_map<std::string, std::string>& environment ) {
	const auto enabled = environment.find( "ETERM_SHELL_INTEGRATION" );
	if ( ( enabled != environment.end() && enabled->second == "0" ) ||
		 ( enabled == environment.end() && Sys::getEnv( "ETERM_SHELL_INTEGRATION" ) == "0" ) )
		return {};
	auto command = powerShellIntegrationCommand( executable, args );
	if ( !command.empty() ) {
		std::vector<std::string> modified;
		modified.reserve( args.size() + 3 );
		modified.insert( modified.end(), args.begin(), args.end() );
		modified.emplace_back( "-NoExit" );
		modified.emplace_back( "-Command" );
		modified.emplace_back( std::move( command ) );
		return modified;
	}
#ifndef _WIN32
	const auto integration = prepareUnixEnvironment( executable, args, environment );
	if ( integration == UnixIntegration::None || integration == UnixIntegration::Zsh )
		return {};
	std::vector<std::string> modified;
	modified.reserve( args.size() + 2 );
	if ( integration == UnixIntegration::Fish ) {
		modified.emplace_back( "--init-command" );
		modified.emplace_back( FishDirectoryIntegration );
	} else if ( integration == UnixIntegration::Bash ) {
		// ENV supplies a startup file for interactive POSIX bash; it restores normal mode and
		// loads the user's usual startup files before installing the prompt hook.
		modified.emplace_back( "--posix" );
		modified.emplace_back( "--noprofile" );
	} else if ( integration == UnixIntegration::BashRc ) {
		modified.emplace_back( "--rcfile" );
		modified.emplace_back( environment.at( "ETERM_INTEGRATION_DIR" ) + "/bash.bash" );
	}
	for ( size_t i = 0; i < args.size(); ++i ) {
		if ( integration == UnixIntegration::Bash || integration == UnixIntegration::BashRc ) {
			if ( args[i] == "--norc" )
				continue;
			if ( args[i] == "--rcfile" || args[i] == "--init-file" ) {
				++i;
				continue;
			}
		}
		modified.emplace_back( args[i] );
	}
	return modified;
#else
	return {};
#endif
}

}} // namespace eterm::System
