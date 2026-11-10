#include "harnesssessionrecovery.hpp"
#include "codexinvocation.hpp"
#include "codexsessionrelay.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <eepp/core/string.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/process.hpp>
#include <eepp/system/sys.hpp>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace EE::System;

namespace eterm {

namespace {

#if EE_PLATFORM == EE_PLATFORM_WIN
constexpr char PluginPathSeparator = ';';
#else
constexpr char PluginPathSeparator = ':';
#endif

bool validPath( const std::string& path, bool allowEmpty = false ) {
	if ( path.empty() )
		return allowEmpty;
	if ( FileSystem::isRelativePath( path ) || path.size() > 4096 )
		return false;
	for ( unsigned char c : path ) {
		if ( c < 32 || c == 127 )
			return false;
	}
	return true;
}

// Metadata is data, never executable shell text. Session names and partial IDs are deliberately
// excluded: both can resolve to another conversation. Paths cannot contain terminal controls.
bool validDescriptor( const nlohmann::json& value ) {
	if ( !value.is_object() || !value.contains( "version" ) || value["version"] != 1 )
		return false;
	for ( const auto* key : { "harness", "session_id", "cwd", "transcript" } ) {
		if ( !value.contains( key ) || !value[key].is_string() )
			return false;
	}
	const auto& kind = value["harness"].get_ref<const std::string&>();
	const auto& id = value["session_id"].get_ref<const std::string&>();
	if ( ( kind != "codex" && kind != "claude" ) || id.size() != 36 )
		return false;
	for ( size_t i = 0; i < id.size(); ++i ) {
		const char c = id[i];
		if ( i == 8 || i == 13 || i == 18 || i == 23 ) {
			if ( c != '-' )
				return false;
		} else if ( !( ( c >= '0' && c <= '9' ) || ( c >= 'a' && c <= 'f' ) ||
					   ( c >= 'A' && c <= 'F' ) ) ) {
			return false;
		}
	}
	for ( const auto* key : { "cwd", "transcript" } ) {
		if ( !validPath( value[key].get_ref<const std::string&>() ) )
			return false;
	}
	if ( value.contains( "home" ) &&
		 ( !value["home"].is_string() ||
		   !validPath( value["home"].get_ref<const std::string&>(), true ) ) )
		return false;
	if ( value.contains( "mode" ) && value["mode"] != "daemon" && value["mode"] != "standalone" )
		return false;
	if ( value.contains( "socket" ) &&
		 ( !value["socket"].is_string() ||
		   !validPath( value["socket"].get_ref<const std::string&>() ) ) )
		return false;
	return true;
}

// Helpers run before any application/renderer/configuration initialization. Ancestry is
// inspected only on attach and SessionStart, never per frame.
ProcessID decimalPid( std::string_view value ) {
	ProcessID pid = 0;
	const auto result = std::from_chars( value.data(), value.data() + value.size(), pid );
	return result.ec == std::errc{} && result.ptr == value.data() + value.size() && pid > 0 ? pid
																							: 0;
}

bool paneChild() {
	const auto shell = decimalPid( Sys::getEnv( "ETERM_HARNESS_SHELL_PID" ) );
	return shell && Sys::getProcessInfo( Sys::getProcessID() ).parentID == shell;
}

bool oneShot( const std::string& kind, const std::vector<std::string>& args ) {
	if ( kind == "codex" )
		return !CodexInvocation::interactive( args );
	// Claude exports its mode to hooks: "cli" is the interactive TUI, anything else (sdk-cli for
	// --print) is not. This survives npm installs, whose process title overwrites the visible argv.
	const auto entrypoint = Sys::getEnv( "CLAUDE_CODE_ENTRYPOINT" );
	if ( !entrypoint.empty() )
		return entrypoint != "cli";
	for ( const auto& arg : args ) {
		const auto flag = std::string_view( arg ).substr( 0, arg.find( '=' ) );
		if ( flag == "--remote" || flag == "--print" || flag == "--bare" || flag == "--bg" ||
			 flag == "--background" || flag == "--no-session-persistence" ||
			 ( kind == "claude" && flag == "-p" ) )
			return true;
	}
	return !args.empty() &&
		   ( args.front() == "exec" || args.front() == "review" || args.front() == "app-server" );
}

bool nodeProcessName( std::string_view name ) {
	return name == "nodejs" || ( String::startsWith( name, "node" ) &&
								 std::all_of( name.begin() + 4, name.end(),
											  []( char c ) { return c >= '0' && c <= '9'; } ) );
}

ProcessID hookOwner( const std::string& kind ) {
	const ProcessID shell = decimalPid( Sys::getEnv( "ETERM_HARNESS_SHELL_PID" ) );
	if ( !shell )
		return 0;
	ProcessID pid = Sys::getProcessID();
	ProcessID owner = 0;
	for ( int depth = 0; depth < 32; ++depth ) {
		auto info = Sys::getProcessInfo( pid );
		info.name = String::toLower( info.name );
		if ( String::endsWith( info.name, ".exe" ) )
			info.name.resize( info.name.size() - 4 );
		// npm launchers remain in the ancestry on POSIX too. Validate the package entry
		// point rather than treating an arbitrary node process as a harness owner.
		std::vector<std::string> nodeArguments;
		bool npmLauncher = false;
		// Recent Node releases label their main thread MainThread on Linux.
		if ( nodeProcessName( info.name ) || info.name == "mainthread" ) {
			nodeArguments = Sys::getProcessInfo( pid, true ).arguments;
			if ( kind == "claude" && !nodeArguments.empty() && nodeArguments[0] == "claude" &&
				 std::all_of( nodeArguments.begin() + 1, nodeArguments.end(),
							  []( const std::string& arg ) { return arg.empty(); } ) ) {
				// npm Claude sets process.title, which overwrites the visible argv (leaving only
				// empty arguments after it); macOS keeps the node name. Only Claude runs this
				// hook, so the title identifies its process.
				info.name = "claude";
			} else if ( nodeArguments.size() >= 2 ) {
				auto executable =
					String::toLower( FileSystem::fileNameFromPath( nodeArguments[0] ) );
				if ( String::endsWith( executable, ".exe" ) )
					executable.resize( executable.size() - 4 );
				if ( !nodeProcessName( executable ) )
					return 0;
				// npm puts a symlink to the package entry point on PATH; node sees that path.
				std::string script = FileSystem::getRealPath( nodeArguments[1] );
				if ( script.empty() )
					script = nodeArguments[1];
				String::replaceAll( script, "\\", "/" );
				npmLauncher = String::endsWith( script, kind == "codex"
															? "/@openai/codex/bin/codex.js"
															: "/@anthropic-ai/claude-code/cli.js" );
			}
		}
		if ( npmLauncher && !owner )
			owner = pid;
		if ( info.name == "codex" || info.name == "claude" ) {
			if ( owner || info.name != kind )
				return 0;
			owner = pid;
		}
		if ( info.parentID == shell ) {
			if ( !owner )
				return 0;
			if ( pid != owner ) {
				const auto helper = Sys::getProcessInfo( pid, true ).arguments;
				const bool ownHelper = helper.size() >= 3 &&
									   helper[0] == Sys::getProcessFilePath() &&
									   helper[1] == "--harness-session" &&
									   ( helper[2] == "resume" || helper[2] == "codex" );
#if EE_PLATFORM == EE_PLATFORM_WIN
				if ( !ownHelper && !npmLauncher && info.name != "cmd" )
#else
				if ( !ownHelper && !npmLauncher )
#endif
					return 0;
			}

			auto arguments = Sys::getProcessInfo( owner, true ).arguments;
			if ( arguments.empty() )
				return 0;
			arguments.erase( arguments.begin() );
			if ( owner == pid && npmLauncher )
				arguments.erase( arguments.begin() );
			return oneShot( kind, arguments ) ? 0 : owner;
		}
		if ( info.parentID <= 1 || info.parentID == pid )
			return 0;
		pid = info.parentID;
	}
	return 0;
}

} // namespace

nlohmann::json HarnessSessionRecovery::descriptor( const std::string& harness,
												   const nlohmann::json& payload,
												   const std::string& home ) {
	if ( !payload.is_object() )
		return {};
	nlohmann::json value{ { "version", 1 },
						  { "harness", harness },
						  { "session_id", payload.value( "session_id", nlohmann::json{} ) },
						  { "cwd", payload.value( "cwd", nlohmann::json{} ) },
						  { "transcript", payload.value( "transcript_path", nlohmann::json{} ) },
						  { "home", home } };
	return validDescriptor( value ) ? value : nlohmann::json{};
}

void HarnessSessionRecovery::publish( const std::string& directory, const nlohmann::json& record ) {
	const auto data = record.is_null()
						  ? std::string{}
						  : record.dump( -1, ' ', false, nlohmann::json::error_handler_t::replace );
	if ( data.size() > 16384 || !FileSystem::isDirectory( directory ) )
		return;
	// A failed atomic write keeps the previous file, which may name another conversation:
	// remove it so nothing is saved rather than the wrong session. Hooks stay silent.
	const auto path = directory + "/active.json";
	if ( !FileSystem::fileWriteAtomic( path, data ) )
		FileSystem::fileRemove( path );
}

std::optional<int> HarnessSessionRecovery::command( int argc, char* argv[] ) {
	if ( argc < 2 || std::string_view( argv[1] ) != "--harness-session" )
		return {};
	const std::string_view operation = argc > 2 ? argv[2] : "";
	const auto directory = Sys::getEnv( "ETERM_HARNESS_DIRECTORY" );
	try {
		if ( operation == "hook" ) {
			// Hook stdout can become model context. Missing opt-in, malformed inputs and failed
			// ancestry checks are deliberately silent and cannot block the harness.
			if ( argc != 4 || directory.empty() )
				return 0;
			const std::string kind = argv[3];
			if ( kind != "codex" && kind != "claude" )
				return 0;
			std::string input;
			std::array<char, 4096> buffer;
			while ( std::cin.read( buffer.data(), buffer.size() ) || std::cin.gcount() ) {
				input.append( buffer.data(), static_cast<size_t>( std::cin.gcount() ) );
				if ( input.size() > 65536 )
					return 0;
			}
			auto payload = nlohmann::json::parse( input, nullptr, false );
			if ( !payload.is_object() || payload.value( "hook_event_name", "" ) != "SessionStart" ||
				 ( payload.contains( "agent_id" ) && !payload["agent_id"].is_null() &&
				   payload["agent_id"] != "" ) )
				return 0;
			const ProcessID owner = hookOwner( kind );
			if ( !owner )
				return 0;
			auto value =
				descriptor( kind, payload,
							Sys::getEnv( kind == "codex" ? "CODEX_HOME" : "CLAUDE_CONFIG_DIR" ) );
			if ( !value.is_null() ) {
				value["owner_pid"] = owner;
				value["owner_started"] = Sys::getProcessCreationTime( owner );
			}
			publish( directory, value );
			return 0;
		}
		if ( operation == "attach" ) {
			const ProcessID shell = argc == 4 ? decimalPid( argv[3] ) : 0;
			return !directory.empty() && shell &&
						   Sys::getProcessInfo( Sys::getProcessID() ).parentID == shell &&
						   Sys::getProcessInfo( shell ).parentID ==
							   decimalPid( Sys::getEnv( "ETERM_HARNESS_EMULATOR_PID" ) )
					   ? 0
					   : 1;
		}
		if ( operation == "codex" ) {
			const int first = argc > 3 && std::string_view( argv[3] ) == "--" ? 4 : 3;
			return CodexSessionRelay::run( std::vector<std::string>( argv + first, argv + argc ) );
		}
		if ( operation == "resume" && argc == 3 ) {
			auto request = Sys::getEnv( "ETERM_RESUME_SESSION" );
			Sys::setEnv( "ETERM_RESUME_SESSION", "" );
			if ( directory.empty() || !paneChild() || request.empty() )
				return 0;
			auto value = nlohmann::json::parse( request, nullptr, false );
			if ( request.size() > 16384 || !validDescriptor( value ) ||
				 !FileSystem::isDirectory( value["cwd"].get_ref<const std::string&>() ) ||
				 FileSystem::fileSize( value["transcript"].get_ref<const std::string&>() ) == 0 ) {
				std::cerr << "eterm: saved agent session is unavailable; opening a shell.\n";
				return 0;
			}
			const auto kind = value["harness"].get<std::string>();
			const auto id = value["session_id"].get<std::string>();
			const auto home = value.value( "home", "" );
			if ( !home.empty() )
				Sys::setEnv( kind == "codex" ? "CODEX_HOME" : "CLAUDE_CONFIG_DIR", home.c_str() );
			const bool changed =
				FileSystem::changeWorkingDirectory( value["cwd"].get_ref<const std::string&>() );
			if ( !changed )
				return 0;
			std::cerr << "eterm: trying to resume " << kind << " session " << id << '\n';
			int result;
			if ( kind == "codex" && value.value( "mode", "standalone" ) == "daemon" ) {
				Sys::setEnv( "ETERM_CODEX_SOCKET", value.value( "socket", "" ).c_str() );
				result = CodexSessionRelay::run( { "resume", id } );
			} else {
				result = Process::runForeground(
					Sys::which( kind ),
					kind == "codex" ? std::vector<std::string>{ "resume", "--no-daemon", id }
									: std::vector<std::string>{ "--resume", id } );
			}
			if ( result )
				std::cerr << "eterm: agent resume exited with status " << result
						  << "; the shell is available.\n";
			return 0;
		}
	} catch ( const std::exception& ) {
		if ( operation != "hook" )
			std::cerr << "eterm: agent recovery failed; the shell is available.\n";
		return operation == "hook" ? 0 : 1;
	}
	std::cerr << "Usage: eterm --harness-session {attach <shell-pid>|hook "
				 "<codex|claude>|resume|codex -- [args]}\n";
	return 2;
}

HarnessSessionRecovery::~HarnessSessionRecovery() {
	for ( const auto& entry : mDirectories )
		FileSystem::dirRemoveAll( entry.second );
}

void HarnessSessionRecovery::resetEnvironment(
	std::unordered_map<std::string, std::string>& environment ) {
	// A nested eterm must not retain a plugin owned by the parent's disposable directory.
	// The relay clears ETERM_HARNESS_DIRECTORY in its child, so track the plugin separately.
	const auto plugin = Sys::getEnv( "ETERM_HARNESS_CLAUDE_PLUGIN" );
	if ( !plugin.empty() ) {
		auto plugins =
			String::split( Sys::getEnv( "CLAUDE_CODE_PLUGIN_DIRS" ), PluginPathSeparator );
		plugins.erase( std::remove( plugins.begin(), plugins.end(), plugin ), plugins.end() );
		environment["CLAUDE_CODE_PLUGIN_DIRS"] = String::join( plugins, PluginPathSeparator );
	}
	// Nested eterm launches must not consume their parent's request or inherit a relay's bearer
	// token. Inspect every key, including when a relay child has an empty directory.
	for ( const auto* key :
		  { "ETERM_HARNESS_DIRECTORY", "ETERM_HARNESS_BINARY", "ETERM_RESUME_SESSION",
			"ETERM_HARNESS_FISH", "ETERM_HARNESS_EMULATOR_PID", "ETERM_HARNESS_SHELL_PID",
			"ETERM_CODEX_SOCKET", "ETERM_CODEX_RELAY_TOKEN", "ETERM_HARNESS_POWERSHELL",
			"ETERM_HARNESS_CLAUDE_PLUGIN" } ) {
		if ( !Sys::getEnv( key ).empty() )
			environment[key] = "";
	}
}

std::string
HarnessSessionRecovery::prepare( std::unordered_map<std::string, std::string>& environment,
								 const nlohmann::json& resume ) {
#if EE_PLATFORM == EE_PLATFORM_MACOS || EE_PLATFORM == EE_PLATFORM_LINUX || \
	EE_PLATFORM == EE_PLATFORM_BSD || EE_PLATFORM == EE_PLATFORM_WIN ||     \
	EE_PLATFORM == EE_PLATFORM_HAIKU
	const auto directory = FileSystem::createTempDirectory( "eterm-harness-" );
	if ( directory.empty() ) {
		Log::warning(
			"eterm: harness session recovery could not create a private temporary directory." );
		return {};
	}

#if EE_PLATFORM == EE_PLATFORM_WIN
	const auto helper = FileSystem::fileRemoveFileName( Sys::getProcessFilePath() ) +
#ifdef EE_DEBUG
						"eterm-session-debug.exe";
#else
						"eterm-session.exe";
#endif
	if ( !FileSystem::fileExists( helper ) ) {
		Log::warning( "eterm: harness session recovery could not start: console helper '%s' is "
					  "missing. Reinstall eterm with eterm-session.exe beside the application.",
					  helper.c_str() );
		FileSystem::dirRemoveAll( directory );
		return {};
	}
	environment["ETERM_HARNESS_BINARY"] = helper;
#else
	environment["ETERM_HARNESS_BINARY"] = Sys::getProcessFilePath();
#endif
	// Claude loads this hook only for the pane's session, alongside the user's settings and
	// plugins. Exec-form arguments keep helper paths literal on every platform, with no shell.
	nlohmann::json plugin{ { "name", "eterm-session-recovery" }, { "version", "1.0.0" } };
	plugin["hooks"]["SessionStart"][0]["hooks"][0] = {
		{ "type", "command" },
		{ "command", environment["ETERM_HARNESS_BINARY"] },
		{ "args", { "--harness-session", "hook", "claude" } } };
	if ( directory.find( PluginPathSeparator ) != std::string::npos ||
		 !FileSystem::makeDir( directory + "/.claude-plugin" ) ||
		 !FileSystem::fileWrite( directory + "/.claude-plugin/plugin.json", plugin.dump() ) ) {
		Log::warning( "eterm: harness session recovery could not prepare the Claude hook." );
		FileSystem::dirRemoveAll( directory );
		environment["ETERM_HARNESS_BINARY"] = "";
		return {};
	}
	auto found = environment.find( "CLAUDE_CODE_PLUGIN_DIRS" );
	auto plugins = found == environment.end() ? Sys::getEnv( "CLAUDE_CODE_PLUGIN_DIRS" )
											  : std::move( found->second );
	if ( !plugins.empty() )
		plugins += PluginPathSeparator;
	plugins += directory;
	environment["CLAUDE_CODE_PLUGIN_DIRS"] = std::move( plugins );
	environment["ETERM_HARNESS_CLAUDE_PLUGIN"] = directory;
	environment["ETERM_HARNESS_DIRECTORY"] = directory;
	environment["ETERM_HARNESS_EMULATOR_PID"] = std::to_string( Sys::getProcessID() );
	environment["ETERM_HARNESS_POWERSHELL"] =
		Sys::getResourcePath() + "assets/shell-integration/harness-session.ps1";

	environment["ETERM_HARNESS_FISH"] =
		Sys::getResourcePath() + "assets/shell-integration/harness-session.fish";
	if ( !resume.is_null() ) {
		// The helper validates this again before launch. A bounded invalid request produces a
		// shell message instead of silently selecting a different conversation.
		const auto request = resume.dump();
		environment["ETERM_RESUME_SESSION"] = request.size() <= 16384 ? request : "{}";
	}
	return directory;
#else
	(void)environment;
	(void)resume;
	return {};
#endif
}

void HarnessSessionRecovery::track( const UITerminal* terminal, std::string directory ) {
	if ( !directory.empty() )
		mDirectories.emplace( terminal, std::move( directory ) );
}

void HarnessSessionRecovery::remove( const UITerminal* terminal ) {
	auto found = mDirectories.find( terminal );
	if ( found == mDirectories.end() )
		return;
	FileSystem::dirRemoveAll( found->second );
	mDirectories.erase( found );
}

nlohmann::json HarnessSessionRecovery::capture( const UITerminal* terminal ) const {
#if EE_PLATFORM == EE_PLATFORM_MACOS || EE_PLATFORM == EE_PLATFORM_LINUX || \
	EE_PLATFORM == EE_PLATFORM_BSD || EE_PLATFORM == EE_PLATFORM_WIN ||     \
	EE_PLATFORM == EE_PLATFORM_HAIKU
	auto found = mDirectories.find( terminal );
	if ( found == mDirectories.end() )
		return {};
	const auto path = found->second + "/active.json";
	if ( FileSystem::fileSize( path ) > 16384 )
		return {};
	std::string data;
	if ( !FileSystem::fileGet( path, data ) || data.empty() )
		return {};
	try {
		auto value = nlohmann::json::parse( data, nullptr, false );
		if ( !validDescriptor( value ) || !value.contains( "owner_pid" ) ||
			 !value["owner_pid"].is_number_unsigned() )
			return {};
		const auto pid = value["owner_pid"].get<ProcessID>();
		if ( pid == 0 )
			return {};
		// A start time distinguishes the owner from an unrelated process that reused its PID.
		if ( Sys::getProcessInfo( pid ).name.empty() ||
			 ( value.contains( "owner_started" ) &&
			   value["owner_started"] != Sys::getProcessCreationTime( pid ) ) )
			return {};
		value.erase( "owner_pid" );
		value.erase( "owner_started" );
		return value;
	} catch ( const nlohmann::json::exception& ) {
		return {};
	}
#else
	(void)terminal;
	return {};
#endif
}

} // namespace eterm
