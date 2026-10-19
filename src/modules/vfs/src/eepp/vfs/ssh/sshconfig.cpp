#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <eepp/core/string.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/process.hpp>
#include <eepp/system/sha1.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/vfs/ssh/sshconfig.hpp>
#include <set>

#if EE_PLATFORM == EE_PLATFORM_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pwd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace EE { namespace VFS {

// ---------------------------------------------------------------------------------------------
// Patterns
// ---------------------------------------------------------------------------------------------

bool SSHPattern::match( std::string_view str, std::string_view pattern, bool caseInsensitive ) {
	// Iterative wildcard matching with single-star backtracking (linear in practice).
	size_t s = 0, p = 0, starP = std::string_view::npos, starS = 0;
	auto equal = [caseInsensitive]( char a, char b ) {
		if ( caseInsensitive ) {
			return std::tolower( static_cast<unsigned char>( a ) ) ==
				   std::tolower( static_cast<unsigned char>( b ) );
		}
		return a == b;
	};
	while ( s < str.size() ) {
		if ( p < pattern.size() && ( pattern[p] == '?' || equal( pattern[p], str[s] ) ) ) {
			++s;
			++p;
		} else if ( p < pattern.size() && pattern[p] == '*' ) {
			starP = p++;
			starS = s;
		} else if ( starP != std::string_view::npos ) {
			p = starP + 1;
			s = ++starS;
		} else {
			return false;
		}
	}
	while ( p < pattern.size() && pattern[p] == '*' )
		++p;
	return p == pattern.size();
}

SSHPattern::ListMatch SSHPattern::matchList( std::string_view str, std::string_view patternList,
											 bool caseInsensitive ) {
	ListMatch result = ListMatch::NoMatch;
	size_t start = 0;
	while ( start <= patternList.size() ) {
		size_t end = patternList.find( ',', start );
		if ( end == std::string_view::npos )
			end = patternList.size();
		std::string_view pattern = patternList.substr( start, end - start );
		bool negated = !pattern.empty() && pattern[0] == '!';
		if ( negated )
			pattern.remove_prefix( 1 );
		if ( match( str, pattern, caseInsensitive ) ) {
			if ( negated )
				return ListMatch::Negated;
			result = ListMatch::Match;
		}
		start = end + 1;
	}
	return result;
}

// ---------------------------------------------------------------------------------------------
// Parsing
// ---------------------------------------------------------------------------------------------

namespace {

bool isSpace( char c ) {
	return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f';
}

std::string toLower( std::string_view str ) {
	std::string out( str );
	for ( auto& c : out )
		c = static_cast<char>( std::tolower( static_cast<unsigned char>( c ) ) );
	return out;
}

} // namespace

bool SSHConfigFile::splitArguments( std::string_view s, std::vector<std::string>& args ) {
	size_t i = 0;
	while ( true ) {
		while ( i < s.size() && isSpace( s[i] ) )
			++i;
		if ( i >= s.size() )
			return true;
		// Comments start at the beginning of an argument.
		if ( s[i] == '#' )
			return true;
		std::string arg;
		char quote = 0;
		for ( ; i < s.size(); ++i ) {
			char c = s[i];
			if ( quote == 0 && isSpace( c ) )
				break;
			if ( c == '\\' && i + 1 < s.size() &&
				 ( s[i + 1] == '\'' || s[i + 1] == '"' || s[i + 1] == '\\' ||
				   ( quote == 0 && s[i + 1] == ' ' ) ) ) {
				arg += s[++i];
			} else if ( quote == 0 && ( c == '"' || c == '\'' ) ) {
				quote = c;
			} else if ( quote != 0 && c == quote ) {
				quote = 0;
			} else {
				arg += c;
			}
		}
		if ( quote != 0 )
			return false;
		args.push_back( std::move( arg ) );
	}
}

SSHConfigFile SSHConfigFile::parse( std::string_view text, std::string path ) {
	SSHConfigFile file;
	file.path = std::move( path );
	Uint32 lineNumber = 0;
	size_t pos = 0;
	while ( pos <= text.size() ) {
		size_t end = text.find( '\n', pos );
		if ( end == std::string_view::npos )
			end = text.size();
		std::string_view line = text.substr( pos, end - pos );
		pos = end + 1;
		++lineNumber;

		size_t i = 0;
		while ( i < line.size() && isSpace( line[i] ) )
			++i;
		if ( i >= line.size() || line[i] == '#' )
			continue;
		// Keyword: up to whitespace or '=' (OpenSSH strdelim()).
		size_t k = i;
		while ( k < line.size() && !isSpace( line[k] ) && line[k] != '=' )
			++k;
		SSHConfigLine entry;
		entry.keyword = toLower( line.substr( i, k - i ) );
		entry.line = lineNumber;
		// Separator: whitespace, or exactly one '=' optionally surrounded by whitespace.
		while ( k < line.size() && isSpace( line[k] ) )
			++k;
		if ( k < line.size() && line[k] == '=' ) {
			++k;
			while ( k < line.size() && isSpace( line[k] ) )
				++k;
		}
		std::string_view rest = line.substr( k );
		while ( !rest.empty() && isSpace( rest.back() ) )
			rest.remove_suffix( 1 );
		entry.rawArgs = std::string( rest );
		if ( !splitArguments( rest, entry.args ) ) {
			file.errors.push_back( String::format( "%s line %u: invalid quotes", file.path,
												   static_cast<unsigned int>( lineNumber ) ) );
			continue;
		}
		if ( entry.args.empty() ) {
			file.errors.push_back(
				String::format( "%s line %u: no argument after keyword \"%s\"", file.path,
								static_cast<unsigned int>( lineNumber ), entry.keyword ) );
			continue;
		}
		file.lines.push_back( std::move( entry ) );
	}
	return file;
}

// ---------------------------------------------------------------------------------------------
// Option table
// ---------------------------------------------------------------------------------------------

namespace {

enum class Merge : Uint8 {
	/** The first obtained value wins (most options). */
	FirstWins,
	/** Every occurrence appends (IdentityFile, CertificateFile, forwards). */
	Append,
	/** First wins, value is the raw remainder of the line (commands). */
	Raw,
	/** Handled explicitly by the evaluator. */
	Special,
};

enum class Tokens : Uint8 {
	None,
	/** %% and %h (HostName). */
	HostOnly,
	/** %% %h %n %p %r (ProxyCommand, ProxyJump). */
	Proxy,
	/** %% %C %d %h %i %j %k %L %l %n %p %r %u (paths and commands). */
	Full,
};

struct OptionDef {
	const char* keyword;
	Merge merge;
	Tokens tokens;
	bool environment;
	bool tilde;
};

constexpr OptionDef OptionTable[] = {
	{ "host", Merge::Special, Tokens::None, false, false },
	{ "match", Merge::Special, Tokens::None, false, false },
	{ "include", Merge::Special, Tokens::Full, true, true },
	{ "hostname", Merge::FirstWins, Tokens::HostOnly, false, false },
	{ "user", Merge::FirstWins, Tokens::None, false, false },
	{ "port", Merge::FirstWins, Tokens::None, false, false },
	{ "identityfile", Merge::Append, Tokens::Full, true, true },
	{ "certificatefile", Merge::Append, Tokens::Full, true, true },
	{ "identityagent", Merge::FirstWins, Tokens::Full, true, true },
	{ "identitiesonly", Merge::FirstWins, Tokens::None, false, false },
	{ "userknownhostsfile", Merge::FirstWins, Tokens::Full, true, true },
	{ "globalknownhostsfile", Merge::FirstWins, Tokens::None, false, true },
	{ "stricthostkeychecking", Merge::FirstWins, Tokens::None, false, false },
	{ "hostkeyalias", Merge::FirstWins, Tokens::None, false, false },
	{ "checkhostip", Merge::FirstWins, Tokens::None, false, false },
	{ "proxyjump", Merge::Special, Tokens::Proxy, false, false },
	{ "proxycommand", Merge::Special, Tokens::Proxy, false, false },
	{ "connecttimeout", Merge::FirstWins, Tokens::None, false, false },
	{ "serveraliveinterval", Merge::FirstWins, Tokens::None, false, false },
	{ "serveralivecountmax", Merge::FirstWins, Tokens::None, false, false },
	{ "preferredauthentications", Merge::FirstWins, Tokens::None, false, false },
	{ "pubkeyauthentication", Merge::FirstWins, Tokens::None, false, false },
	{ "passwordauthentication", Merge::FirstWins, Tokens::None, false, false },
	{ "kbdinteractiveauthentication", Merge::FirstWins, Tokens::None, false, false },
	{ "numberofpasswordprompts", Merge::FirstWins, Tokens::None, false, false },
	{ "batchmode", Merge::FirstWins, Tokens::None, false, false },
	{ "compression", Merge::FirstWins, Tokens::None, false, false },
	{ "ciphers", Merge::FirstWins, Tokens::None, false, false },
	{ "macs", Merge::FirstWins, Tokens::None, false, false },
	{ "kexalgorithms", Merge::FirstWins, Tokens::None, false, false },
	{ "hostkeyalgorithms", Merge::FirstWins, Tokens::None, false, false },
	{ "pubkeyacceptedalgorithms", Merge::FirstWins, Tokens::None, false, false },
	{ "hostbasedacceptedalgorithms", Merge::FirstWins, Tokens::None, false, false },
	{ "casignaturealgorithms", Merge::FirstWins, Tokens::None, false, false },
	{ "tag", Merge::FirstWins, Tokens::None, false, false },
	{ "ignoreunknown", Merge::FirstWins, Tokens::None, false, false },
	{ "canonicaldomains", Merge::FirstWins, Tokens::None, false, false },
	{ "canonicalizefallbacklocal", Merge::FirstWins, Tokens::None, false, false },
	{ "canonicalizehostname", Merge::FirstWins, Tokens::None, false, false },
	{ "canonicalizemaxdots", Merge::FirstWins, Tokens::None, false, false },
	{ "canonicalizepermittedcnames", Merge::FirstWins, Tokens::None, false, false },
	{ "controlpath", Merge::FirstWins, Tokens::Full, true, true },
	{ "controlmaster", Merge::FirstWins, Tokens::None, false, false },
	{ "controlpersist", Merge::FirstWins, Tokens::None, false, false },
	{ "localcommand", Merge::Raw, Tokens::None, false, false },
	{ "remotecommand", Merge::Raw, Tokens::Full, false, false },
	{ "knownhostscommand", Merge::Raw, Tokens::Full, true, false },
	{ "sendenv", Merge::Special, Tokens::None, false, false },
	{ "setenv", Merge::FirstWins, Tokens::None, false, false },
	{ "localforward", Merge::Append, Tokens::None, false, false },
	{ "remoteforward", Merge::Append, Tokens::None, false, false },
	{ "dynamicforward", Merge::Append, Tokens::None, false, false },
	{ "revokedhostkeys", Merge::FirstWins, Tokens::Full, true, true },
	// Accepted (first wins) for compatibility; not used by the SFTP client.
	{ "addkeystoagent", Merge::FirstWins, Tokens::None, false, false },
	{ "addressfamily", Merge::FirstWins, Tokens::None, false, false },
	{ "bindaddress", Merge::FirstWins, Tokens::None, false, false },
	{ "bindinterface", Merge::FirstWins, Tokens::None, false, false },
	{ "channeltimeout", Merge::FirstWins, Tokens::None, false, false },
	{ "clearallforwardings", Merge::FirstWins, Tokens::None, false, false },
	{ "connectionattempts", Merge::FirstWins, Tokens::None, false, false },
	{ "enableescapecommandline", Merge::FirstWins, Tokens::None, false, false },
	{ "enablesshkeysign", Merge::FirstWins, Tokens::None, false, false },
	{ "escapechar", Merge::FirstWins, Tokens::None, false, false },
	{ "exitonforwardfailure", Merge::FirstWins, Tokens::None, false, false },
	{ "fingerprinthash", Merge::FirstWins, Tokens::None, false, false },
	{ "forkafterauthentication", Merge::FirstWins, Tokens::None, false, false },
	{ "forwardagent", Merge::FirstWins, Tokens::None, false, false },
	{ "forwardx11", Merge::FirstWins, Tokens::None, false, false },
	{ "forwardx11timeout", Merge::FirstWins, Tokens::None, false, false },
	{ "forwardx11trusted", Merge::FirstWins, Tokens::None, false, false },
	{ "gatewayports", Merge::FirstWins, Tokens::None, false, false },
	{ "gssapiauthentication", Merge::FirstWins, Tokens::None, false, false },
	{ "gssapidelegatecredentials", Merge::FirstWins, Tokens::None, false, false },
	{ "hashknownhosts", Merge::FirstWins, Tokens::None, false, false },
	{ "hostbasedauthentication", Merge::FirstWins, Tokens::None, false, false },
	{ "ipqos", Merge::FirstWins, Tokens::None, false, false },
	{ "kbdinteractivedevices", Merge::FirstWins, Tokens::None, false, false },
	{ "loglevel", Merge::FirstWins, Tokens::None, false, false },
	{ "logverbose", Merge::FirstWins, Tokens::None, false, false },
	{ "nohostauthenticationforlocalhost", Merge::FirstWins, Tokens::None, false, false },
	{ "obscurekeystroketiming", Merge::FirstWins, Tokens::None, false, false },
	{ "permitlocalcommand", Merge::FirstWins, Tokens::None, false, false },
	{ "permitremoteopen", Merge::FirstWins, Tokens::None, false, false },
	{ "pkcs11provider", Merge::FirstWins, Tokens::None, false, false },
	{ "proxyusefdpass", Merge::FirstWins, Tokens::None, false, false },
	{ "rekeylimit", Merge::FirstWins, Tokens::None, false, false },
	{ "requesttty", Merge::FirstWins, Tokens::None, false, false },
	{ "requiredrsasize", Merge::FirstWins, Tokens::None, false, false },
	{ "securitykeyprovider", Merge::FirstWins, Tokens::None, false, false },
	{ "sessiontype", Merge::FirstWins, Tokens::None, false, false },
	{ "stdinnull", Merge::FirstWins, Tokens::None, false, false },
	{ "streamlocalbindmask", Merge::FirstWins, Tokens::None, false, false },
	{ "streamlocalbindunlink", Merge::FirstWins, Tokens::None, false, false },
	{ "syslogfacility", Merge::FirstWins, Tokens::None, false, false },
	{ "tcpkeepalive", Merge::FirstWins, Tokens::None, false, false },
	{ "tunnel", Merge::FirstWins, Tokens::None, false, false },
	{ "tunneldevice", Merge::FirstWins, Tokens::None, false, false },
	{ "updatehostkeys", Merge::FirstWins, Tokens::None, false, false },
	{ "verifyhostkeydns", Merge::FirstWins, Tokens::None, false, false },
	{ "versionaddendum", Merge::FirstWins, Tokens::None, false, false },
	{ "visualhostkey", Merge::FirstWins, Tokens::None, false, false },
	{ "warnweakcrypto", Merge::FirstWins, Tokens::None, false, false },
	{ "xauthlocation", Merge::FirstWins, Tokens::None, false, false },
	{ "refuseconnection", Merge::FirstWins, Tokens::None, false, false },
};

/** Deprecated spellings accepted by OpenSSH. */
constexpr std::pair<const char*, const char*> OptionAliases[] = {
	{ "challengeresponseauthentication", "kbdinteractiveauthentication" },
	{ "pubkeyacceptedkeytypes", "pubkeyacceptedalgorithms" },
	{ "hostbasedkeytypes", "hostbasedacceptedalgorithms" },
	{ "hostbasedacceptedkeytypes", "hostbasedacceptedalgorithms" },
	{ "dsaauthentication", "pubkeyauthentication" },
};

const OptionDef* findOption( std::string_view keyword ) {
	for ( const auto& alias : OptionAliases ) {
		if ( keyword == alias.first )
			keyword = alias.second;
	}
	for ( const auto& def : OptionTable ) {
		if ( keyword == def.keyword )
			return &def;
	}
	return nullptr;
}

constexpr int MaxIncludeDepth = 16;

bool hasWildcard( std::string_view str ) {
	return str.find_first_of( "*?" ) != std::string_view::npos;
}

bool parseBool( const std::string& value, bool fallback ) {
	std::string v = toLower( value );
	if ( v == "yes" || v == "true" )
		return true;
	if ( v == "no" || v == "false" )
		return false;
	return fallback;
}

std::string joinArgs( const std::vector<std::string>& args ) {
	std::string out;
	for ( const auto& arg : args ) {
		if ( !out.empty() )
			out += ' ';
		out += arg;
	}
	return out;
}

bool isIPAddress( const std::string& host ) {
	unsigned char buffer[16];
	return inet_pton( AF_INET, host.c_str(), buffer ) == 1 ||
		   inet_pton( AF_INET6, host.c_str(), buffer ) == 1;
}

} // namespace

// ---------------------------------------------------------------------------------------------
// Evaluator
// ---------------------------------------------------------------------------------------------

class SSHConfigEvaluator::Impl {
  public:
	explicit Impl( SSHConfigContext&& context ) : mCtx( std::move( context ) ) {
		mCtx.originalHost = toLower( mCtx.originalHost );
	}

	std::variant<SSHResolvedConfig, std::string> evaluate() {
		mValues.clear();
		mWarnings.clear();
		mError.clear();
		if ( mCtx.userOverride )
			mValues["user"] = { *mCtx.userOverride };
		if ( mCtx.portOverride )
			mValues["port"] = { String::toString( *mCtx.portOverride ) };

		// Pass 1.
		mHost = mCtx.originalHost;
		mFinalPass = false;
		mWantFinal = false;
		runSources();
		if ( !mError.empty() )
			return mError;

		// Apply HostName now (OpenSSH: "If the user has replaced the hostname then take it into
		// use now").
		std::string host = mCtx.originalHost;
		if ( auto it = mValues.find( "hostname" ); it != mValues.end() ) {
			auto expanded =
				expand( it->second.front(), Tokens::HostOnly, false, false, "HostName", &host );
			if ( expanded )
				host = toLower( *expanded );
		}

		bool canonicalized = false;
		std::string canonicalize = toLower( value( "canonicalizehostname", "no" ) );
		if ( canonicalize == "yes" || canonicalize == "always" ) {
			bool proxied = mValues.count( "proxyjump" ) || mValues.count( "proxycommand" );
			if ( !proxied || canonicalize == "always" ) {
				auto result = canonicalizeHost( host );
				if ( !mError.empty() )
					return mError;
				if ( result ) {
					canonicalized = *result != host;
					host = *result;
				}
			}
		}
		if ( canonicalize != "no" && canonicalize != "false" )
			mWantFinal = true;

		if ( mWantFinal ) {
			// The final pass keeps every value obtained so far (first wins), uses the current
			// target name for Host matching and lets "Match canonical/final" match.
			mValues["hostname"] = { host };
			mHost = host;
			mFinalPass = true;
			runSources();
			if ( !mError.empty() )
				return mError;
		}

		return build( host, canonicalized );
	}

	SSHConfigContext mCtx;

  private:
	void runSources() {
		std::vector<std::string> stack;
		for ( const auto& source : mCtx.sources ) {
			auto text = mCtx.readFile ? mCtx.readFile( source.path ) : std::nullopt;
			if ( !text )
				continue;
			stack.clear();
			processFile( source.path, *text, source.includeBase, 0, stack );
			if ( !mError.empty() )
				return;
		}
	}

	void processFile( const std::string& path, const std::string& text,
					  const std::string& includeBase, int depth, std::vector<std::string>& stack ) {
		auto& file = parsedFile( path, text );
		if ( !file.errors.empty() ) {
			mError = file.errors.front();
			return;
		}
		stack.push_back( path );
		bool active = true;
		for ( const auto& line : file.lines ) {
			if ( line.keyword == "host" ) {
				active = matchHost( line.args );
				continue;
			}
			if ( line.keyword == "match" ) {
				active = matchCriteria( line, path );
				if ( !mError.empty() )
					break;
				continue;
			}
			if ( !active )
				continue;
			if ( line.keyword == "include" ) {
				processInclude( line, includeBase, depth, stack );
				if ( !mError.empty() )
					break;
				continue;
			}
			apply( line, path );
		}
		stack.pop_back();
	}

	const SSHConfigFile& parsedFile( const std::string& path, const std::string& text ) {
		auto it = mParsed.find( path );
		if ( it == mParsed.end() )
			it = mParsed.emplace( path, SSHConfigFile::parse( text, path ) ).first;
		return it->second;
	}

	void processInclude( const SSHConfigLine& line, const std::string& includeBase, int depth,
						 std::vector<std::string>& stack ) {
		if ( depth + 1 > MaxIncludeDepth ) {
			mError = "Include nested too deeply";
			return;
		}
		for ( const auto& arg : line.args ) {
			auto expanded = expand( arg, Tokens::Full, true, true, "Include" );
			if ( !expanded )
				continue;
			std::string pattern = *expanded;
			if ( !isAbsolutePath( pattern ) )
				pattern = joinPath( includeBase, pattern );
			for ( const auto& file : glob( pattern ) ) {
				if ( std::find( stack.begin(), stack.end(), file ) != stack.end() ) {
					mWarnings.push_back( "Include cycle ignored: " + file );
					continue;
				}
				auto text = mCtx.readFile ? mCtx.readFile( file ) : std::nullopt;
				if ( !text )
					continue;
				// Included content shares the including file's include base and is evaluated
				// in place; its Host/Match lines only affect the included file.
				processFile( file, *text, includeBase, depth + 1, stack );
				if ( !mError.empty() )
					return;
			}
		}
	}

	bool matchHost( const std::vector<std::string>& patterns ) const {
		bool matched = false;
		for ( const auto& arg : patterns ) {
			std::string_view pattern( arg );
			bool negated = !pattern.empty() && pattern[0] == '!';
			if ( negated )
				pattern.remove_prefix( 1 );
			if ( SSHPattern::match( mHost, pattern ) ) {
				if ( negated )
					return false;
				matched = true;
			}
		}
		return matched;
	}

	/** Port of OpenSSH match_cfg_line(). */
	bool matchCriteria( const SSHConfigLine& line, const std::string& path ) {
		const auto& args = line.args;
		bool result = true;
		for ( size_t i = 0; i < args.size(); ++i ) {
			std::string attribute = toLower( args[i] );
			bool negate = !attribute.empty() && attribute[0] == '!';
			if ( negate )
				attribute.erase( 0, 1 );
			bool matched;
			if ( attribute == "all" ) {
				if ( i + 1 != args.size() ) {
					String::formatTo(
						mError, "%s line %u: Match all must appear alone or after canonical/final",
						path, static_cast<unsigned int>( line.line ) );
					return false;
				}
				matched = true;
			} else if ( attribute == "canonical" || attribute == "final" ) {
				// Either criterion requests a final pass; both only match during it.
				mWantFinal = true;
				matched = mFinalPass;
			} else {
				if ( i + 1 >= args.size() ) {
					String::formatTo( mError, "%s line %u: missing argument for Match %s", path,
									  static_cast<unsigned int>( line.line ), attribute );
					return false;
				}
				const std::string& arg = args[++i];
				if ( attribute == "host" ) {
					matched = listMatches( matchTargetHost(), arg );
				} else if ( attribute == "originalhost" ) {
					matched = listMatches( mCtx.originalHost, arg );
				} else if ( attribute == "user" ) {
					matched = listMatches( value( "user", mCtx.localUser ), arg );
				} else if ( attribute == "localuser" ) {
					matched = listMatches( mCtx.localUser, arg );
				} else if ( attribute == "tagged" ) {
					matched = listMatches( value( "tag", "" ), arg );
				} else if ( attribute == "command" ) {
					matched = listMatches( mCtx.command, arg );
				} else if ( attribute == "version" ) {
					matched = listMatches( mCtx.version, arg );
				} else if ( attribute == "sessiontype" ) {
					matched = listMatches( mCtx.sessionType, arg );
				} else if ( attribute == "localnetwork" ) {
					matched = false;
					if ( mCtx.localNetworkContains ) {
						for ( const auto& cidr : String::split( arg, ',' ) )
							matched = matched || mCtx.localNetworkContains( cidr );
					}
				} else if ( attribute == "exec" ) {
					// Commands only run while the line can still match.
					if ( !result ) {
						matched = false;
					} else if ( !mCtx.exec ) {
						mWarnings.push_back( String::format( "Match exec disabled: \"%s\"", arg ) );
						matched = false;
					} else {
						std::string host = matchTargetHost();
						auto command =
							expand( arg, Tokens::Full, false, false, "Match exec", &host );
						matched = command && mCtx.exec( *command );
					}
				} else {
					String::formatTo( mError, "%s line %u: unsupported Match attribute %s", path,
									  static_cast<unsigned int>( line.line ), attribute );
					return false;
				}
			}
			if ( negate )
				matched = !matched;
			if ( !matched )
				result = false;
		}
		return result;
	}

	static bool listMatches( const std::string& str, const std::string& list ) {
		return SSHPattern::matchList( str, list ) == SSHPattern::ListMatch::Match;
	}

	/** Host used by "Match host": HostName (expanded with %h) if set so far, else the target. */
	std::string matchTargetHost() {
		auto it = mValues.find( "hostname" );
		if ( it == mValues.end() )
			return mHost;
		std::string host = mHost;
		auto expanded =
			expand( it->second.front(), Tokens::HostOnly, false, false, "HostName", &host );
		return expanded ? toLower( *expanded ) : mHost;
	}

	void apply( const SSHConfigLine& line, const std::string& path ) {
		const OptionDef* def = findOption( line.keyword );
		if ( !def ) {
			std::string ignore = value( "ignoreunknown", "" );
			if ( ignore.empty() || !listMatches( line.keyword, ignore ) ) {
				mWarnings.push_back( String::format( "%s line %u: unsupported option \"%s\"", path,
													 static_cast<unsigned int>( line.line ),
													 line.keyword ) );
			}
			return;
		}
		std::string keyword( def->keyword );
		switch ( def->merge ) {
			case Merge::FirstWins:
				mValues.try_emplace( keyword, line.args );
				break;
			case Merge::Raw:
				mValues.try_emplace( keyword, std::vector<std::string>{ line.rawArgs } );
				break;
			case Merge::Append: {
				// Like OpenSSH's add_identity_file()/forward handling, exact duplicates are
				// ignored; this also keeps the final re-parse pass from appending twice.
				auto& list = mValues[keyword];
				for ( const auto& arg : line.args ) {
					if ( std::find( list.begin(), list.end(), arg ) == list.end() )
						list.push_back( arg );
				}
				break;
			}
			case Merge::Special:
				applySpecial( keyword, line );
				break;
		}
	}

	void applySpecial( const std::string& keyword, const SSHConfigLine& line ) {
		if ( keyword == "proxycommand" ) {
			// ProxyCommand and ProxyJump compete: whichever comes first wins.
			if ( !mValues.count( "proxyjump" ) )
				mValues.try_emplace( keyword, std::vector<std::string>{ line.rawArgs } );
		} else if ( keyword == "proxyjump" ) {
			if ( !mValues.count( "proxycommand" ) )
				mValues.try_emplace( keyword, line.args );
		} else if ( keyword == "sendenv" ) {
			auto& list = mValues[keyword];
			for ( const auto& arg : line.args ) {
				if ( !arg.empty() && arg[0] == '-' ) {
					std::string_view pattern( arg );
					pattern.remove_prefix( 1 );
					list.erase( std::remove_if( list.begin(), list.end(),
												[&]( const std::string& v ) {
													return SSHPattern::match( v, pattern );
												} ),
								list.end() );
				} else {
					list.push_back( arg );
				}
			}
		}
	}

	std::string value( const std::string& keyword, const std::string& fallback ) const {
		auto it = mValues.find( keyword );
		return it != mValues.end() && !it->second.empty() ? it->second.front() : fallback;
	}

	// -----------------------------------------------------------------------------------------
	// Expansion
	// -----------------------------------------------------------------------------------------

	struct TokenValues {
		std::string host, original, port, user, jump, alias;
	};

	/** Token values as currently known. `host` overrides %h. */
	TokenValues currentTokens( const std::string* host ) const {
		TokenValues t;
		t.host = host ? *host : value( "hostname", mHost );
		t.original = mCtx.originalHost;
		t.port = value( "port", "22" );
		t.user = value( "user", mCtx.localUser );
		auto jump = mValues.find( "proxyjump" );
		t.jump = jump != mValues.end() ? joinArgs( jump->second ) : std::string();
		if ( toLower( t.jump ) == "none" )
			t.jump.clear();
		t.alias = value( "hostkeyalias", mCtx.originalHost );
		return t;
	}

	std::optional<std::string> expand( const std::string& input, Tokens tokens, bool environment,
									   bool tilde, const char* what,
									   const std::string* host = nullptr ) {
		return expandWith( input, tokens, environment, tilde, what, currentTokens( host ) );
	}

	std::optional<std::string> expandWith( const std::string& input, Tokens tokens,
										   bool environment, bool tilde, const char* what,
										   const TokenValues& t ) {
		std::string str( input );
		if ( tilde && !str.empty() && str[0] == '~' ) {
			size_t slash = str.find( '/' );
			std::string name =
				str.substr( 1, slash == std::string::npos ? std::string::npos : slash - 1 );
			std::optional<std::string> home;
			if ( name.empty() )
				home = mCtx.homeDirectory;
			else if ( mCtx.userHome )
				home = mCtx.userHome( name );
			if ( !home ) {
				mWarnings.push_back( String::format( "%s: unknown user in \"%s\"", what, input ) );
				return std::nullopt;
			}
			str = *home + ( slash == std::string::npos ? std::string() : str.substr( slash ) );
		}
		if ( environment ) {
			std::string out;
			for ( size_t i = 0; i < str.size(); ++i ) {
				if ( str[i] == '$' && i + 1 < str.size() && str[i + 1] == '{' ) {
					size_t end = str.find( '}', i + 2 );
					if ( end == std::string::npos ) {
						mWarnings.push_back(
							String::format( "%s: unterminated ${ in \"%s\"", what, input ) );
						return std::nullopt;
					}
					std::string name = str.substr( i + 2, end - i - 2 );
					auto env = mCtx.getEnv ? mCtx.getEnv( name ) : std::nullopt;
					if ( !env ) {
						// OpenSSH ignores the setting when a referenced variable is unset.
						mWarnings.push_back( String::format(
							"%s: environment variable %s is not set", what, name ) );
						return std::nullopt;
					}
					out += *env;
					i = end;
				} else {
					out += str[i];
				}
			}
			str = std::move( out );
		}
		if ( tokens == Tokens::None )
			return str;
		std::string out;
		out.reserve( str.size() );
		for ( size_t i = 0; i < str.size(); ++i ) {
			if ( str[i] != '%' ) {
				out += str[i];
				continue;
			}
			if ( i + 1 >= str.size() ) {
				mWarnings.push_back( String::format( "%s: trailing %% in \"%s\"", what, input ) );
				return std::nullopt;
			}
			char token = str[++i];
			auto value = tokenValue( token, tokens, t );
			if ( !value ) {
				mWarnings.push_back(
					String::format( "%s: unknown token %%%c in \"%s\"", what, token, input ) );
				return std::nullopt;
			}
			out += *value;
		}
		return out;
	}

	std::optional<std::string> tokenValue( char token, Tokens tokens, const TokenValues& t ) const {
		if ( token == '%' )
			return std::string( "%" );
		if ( token == 'h' )
			return t.host;
		if ( tokens == Tokens::HostOnly )
			return std::nullopt;
		switch ( token ) {
			case 'n':
				return t.original;
			case 'p':
				return t.port;
			case 'r':
				return t.user;
			default:
				break;
		}
		if ( tokens == Tokens::Proxy )
			return std::nullopt;
		switch ( token ) {
			case 'C':
				return System::SHA1::fromString( mCtx.localHostName + t.host + t.port + t.user +
												 t.jump )
					.toHexString();
			case 'd':
				return mCtx.homeDirectory;
			case 'i':
				return mCtx.localUid;
			case 'j':
				return t.jump;
			case 'k':
				return t.alias;
			case 'L': {
				size_t dot = mCtx.localHostName.find( '.' );
				return mCtx.localHostName.substr( 0, dot );
			}
			case 'l':
				return mCtx.localHostName;
			case 'u':
				return mCtx.localUser;
			case 'T':
				return std::string( "NONE" );
			default:
				return std::nullopt;
		}
	}

	// -----------------------------------------------------------------------------------------
	// Include globbing
	// -----------------------------------------------------------------------------------------

	static bool isAbsolutePath( const std::string& path ) {
		if ( path.empty() )
			return false;
#if EE_PLATFORM == EE_PLATFORM_WIN
		if ( path.size() >= 2 && path[1] == ':' )
			return true;
		if ( path[0] == '\\' )
			return true;
#endif
		return path[0] == '/';
	}

	static std::string joinPath( const std::string& base, const std::string& name ) {
		if ( base.empty() )
			return name;
		if ( base.back() == '/' || base.back() == '\\' )
			return base + name;
		return base + "/" + name;
	}

	/** Expands wildcards in any path component; results are lexically sorted per level. */
	std::vector<std::string> glob( const std::string& pattern ) {
		std::string normalized( pattern );
#if EE_PLATFORM == EE_PLATFORM_WIN
		String::replaceAll( normalized, "\\", "/" );
#endif
		if ( !hasWildcard( normalized ) )
			return { normalized };
		std::vector<std::string> parts = String::split( normalized, '/' );
		std::string root = normalized[0] == '/' ? "/" : "";
		std::vector<std::string> current{ root };
		for ( size_t i = 0; i < parts.size(); ++i ) {
			std::vector<std::string> next;
			for ( const auto& base : current ) {
				if ( !hasWildcard( parts[i] ) ) {
					next.push_back( base.empty() ? parts[i] : joinPath( base, parts[i] ) );
					continue;
				}
				if ( !mCtx.listDirectory )
					continue;
				std::vector<std::string> names = mCtx.listDirectory( base.empty() ? "." : base );
				std::sort( names.begin(), names.end() );
				for ( const auto& name : names ) {
					// Like glob(3), wildcards do not match leading dots.
					if ( !name.empty() && name[0] == '.' && parts[i][0] != '.' )
						continue;
					if ( SSHPattern::match( name, parts[i] ) )
						next.push_back( base.empty() ? name : joinPath( base, name ) );
				}
			}
			current = std::move( next );
		}
		return current;
	}

	// -----------------------------------------------------------------------------------------
	// Canonicalization
	// -----------------------------------------------------------------------------------------

	bool cnameAllowed( const std::string& source, const std::string& target ) const {
		auto it = mValues.find( "canonicalizepermittedcnames" );
		if ( it == mValues.end() )
			return false;
		for ( const auto& rule : it->second ) {
			if ( toLower( rule ) == "none" )
				return false;
			size_t colon = rule.find( ':' );
			if ( colon == std::string::npos )
				continue;
			if ( listMatches( source, rule.substr( 0, colon ) ) &&
				 listMatches( target, rule.substr( colon + 1 ) ) )
				return true;
		}
		return false;
	}

	/** @return The canonical host, nullopt when left unchanged; sets mError on hard failure. */
	std::optional<std::string> canonicalizeHost( const std::string& host ) {
		if ( !mCtx.resolve || host.empty() || isIPAddress( host ) )
			return std::nullopt;
		if ( host.back() == '.' ) {
			std::string name = host.substr( 0, host.size() - 1 );
			return mCtx.resolve( host ) ? std::optional<std::string>( name ) : std::nullopt;
		}
		int maxDots = 1;
		String::fromString( maxDots, value( "canonicalizemaxdots", "1" ) );
		if ( static_cast<int>( std::count( host.begin(), host.end(), '.' ) ) > maxDots )
			return std::nullopt;
		auto domains = mValues.find( "canonicaldomains" );
		if ( domains != mValues.end() ) {
			for ( const auto& domain : domains->second ) {
				std::string candidate = host + "." + domain;
				auto resolved = mCtx.resolve( candidate + "." );
				if ( !resolved )
					continue;
				std::string name = toLower( *resolved );
				while ( !name.empty() && name.back() == '.' )
					name.pop_back();
				if ( name != candidate && cnameAllowed( candidate, name ) )
					return name;
				return candidate;
			}
		}
		if ( !parseBool( value( "canonicalizefallbacklocal", "yes" ), true ) ) {
			String::formatTo( mError, "Could not resolve host \"%s\" in any canonical domain",
							  host );
		}
		return std::nullopt;
	}

	// -----------------------------------------------------------------------------------------
	// Result
	// -----------------------------------------------------------------------------------------

	std::vector<std::string> expandList( const std::string& keyword,
										 const std::vector<std::string>& values,
										 const TokenValues& tokens ) {
		const OptionDef* def = findOption( keyword );
		std::vector<std::string> out;
		for ( const auto& v : values ) {
			auto expanded =
				expandWith( v, def ? def->tokens : Tokens::None, def && def->environment,
							def && def->tilde, keyword.c_str(), tokens );
			if ( expanded )
				out.push_back( std::move( *expanded ) );
		}
		return out;
	}

	SSHResolvedConfig build( const std::string& host, bool canonicalized ) {
		SSHResolvedConfig config;
		config.originalHost = mCtx.originalHost;
		config.hostName = host;
		config.canonicalized = canonicalized;
		config.user = value( "user", mCtx.localUser );
		int port = 22;
		if ( !String::fromString( port, value( "port", "22" ) ) || port <= 0 || port > 65535 ) {
			mWarnings.push_back( String::format( "Invalid port \"%s\"", value( "port", "" ) ) );
			port = 22;
		}
		config.port = static_cast<Uint16>( port );

		TokenValues tokens;
		tokens.host = config.hostName;
		tokens.original = config.originalHost;
		tokens.port = String::toString( config.port );
		tokens.user = config.user;
		tokens.alias = value( "hostkeyalias", config.originalHost );
		auto jump = mValues.find( "proxyjump" );
		if ( jump != mValues.end() && toLower( joinArgs( jump->second ) ) != "none" )
			tokens.jump = joinArgs( jump->second );

		// Identity files: raw values are kept like `ssh -G` prints them.
		auto identities = mValues.find( "identityfile" );
		if ( identities == mValues.end() || identities->second.empty() ) {
			mValues["identityfile"] = { "~/.ssh/id_rsa", "~/.ssh/id_ecdsa", "~/.ssh/id_ecdsa_sk",
										"~/.ssh/id_ed25519", "~/.ssh/id_ed25519_sk" };
		}
		config.identityFiles = expandList( "identityfile", mValues["identityfile"], tokens );
		if ( auto it = mValues.find( "certificatefile" ); it != mValues.end() )
			config.certificateFiles = expandList( "certificatefile", it->second, tokens );

		config.identitiesOnly = parseBool( value( "identitiesonly", "no" ), false );
		std::string agent = value( "identityagent", "" );
		if ( agent.empty() || agent == "SSH_AUTH_SOCK" ) {
			config.identityAgent = mCtx.getEnv ? mCtx.getEnv( "SSH_AUTH_SOCK" ) : std::nullopt;
#if EE_PLATFORM == EE_PLATFORM_WIN
			if ( !config.identityAgent )
				config.identityAgent = std::string( "\\\\.\\pipe\\openssh-ssh-agent" );
#endif
		} else if ( toLower( agent ) != "none" ) {
			auto expanded = expandList( "identityagent", { agent }, tokens );
			if ( !expanded.empty() )
				config.identityAgent = expanded.front();
		}

		if ( auto it = mValues.find( "userknownhostsfile" ); it != mValues.end() ) {
			if ( !( it->second.size() == 1 && toLower( it->second[0] ) == "none" ) )
				config.userKnownHostsFiles = expandList( "userknownhostsfile", it->second, tokens );
		} else {
			config.userKnownHostsFiles = expandList(
				"userknownhostsfile", { "~/.ssh/known_hosts", "~/.ssh/known_hosts2" }, tokens );
		}
		mValues["userknownhostsfile"] = config.userKnownHostsFiles;
		// Like `ssh -G`, report ControlPath expanded.
		if ( auto it = mValues.find( "controlpath" );
			 it != mValues.end() && toLower( it->second.front() ) != "none" )
			it->second = expandList( "controlpath", { it->second.front() }, tokens );
		if ( auto it = mValues.find( "globalknownhostsfile" ); it != mValues.end() ) {
			if ( !( it->second.size() == 1 && toLower( it->second[0] ) == "none" ) ) {
				config.globalKnownHostsFiles =
					expandList( "globalknownhostsfile", it->second, tokens );
			}
		} else {
#if EE_PLATFORM == EE_PLATFORM_WIN
			std::string programData =
				mCtx.getEnv ? mCtx.getEnv( "PROGRAMDATA" ).value_or( "C:/ProgramData" )
							: "C:/ProgramData";
			config.globalKnownHostsFiles = { programData + "/ssh/ssh_known_hosts",
											 programData + "/ssh/ssh_known_hosts2" };
#else
			config.globalKnownHostsFiles = { "/etc/ssh/ssh_known_hosts",
											 "/etc/ssh/ssh_known_hosts2" };
#endif
		}

		std::string strict = toLower( value( "stricthostkeychecking", "ask" ) );
		if ( strict == "yes" || strict == "true" )
			config.strictHostKeyChecking = SSHStrictHostKeyChecking::Yes;
		else if ( strict == "accept-new" )
			config.strictHostKeyChecking = SSHStrictHostKeyChecking::AcceptNew;
		else if ( strict == "no" || strict == "off" || strict == "false" )
			config.strictHostKeyChecking = SSHStrictHostKeyChecking::No;
		if ( auto it = mValues.find( "hostkeyalias" ); it != mValues.end() )
			config.hostKeyAlias = it->second.front();
		config.checkHostIP = parseBool( value( "checkhostip", "no" ), false );

		if ( !tokens.jump.empty() ) {
			auto expanded =
				expandWith( tokens.jump, Tokens::Proxy, false, false, "ProxyJump", tokens );
			if ( expanded ) {
				for ( const auto& spec : String::split( *expanded, ',' ) ) {
					auto parsed = SSHConfigEvaluator::parseJumpHost( spec );
					if ( parsed )
						config.proxyJump.push_back( *parsed );
					else {
						mWarnings.push_back(
							String::format( "Invalid ProxyJump entry \"%s\"", spec ) );
					}
				}
			}
		}
		if ( auto it = mValues.find( "proxycommand" ); it != mValues.end() ) {
			const std::string& command = it->second.front();
			if ( toLower( command ) != "none" ) {
				config.proxyCommand =
					expandWith( command, Tokens::Proxy, false, false, "ProxyCommand", tokens );
			}
		}

		std::string timeout = value( "connecttimeout", "none" );
		if ( toLower( timeout ) != "none" )
			config.connectTimeout = SSHConfigEvaluator::parseTime( timeout );
		if ( auto interval = SSHConfigEvaluator::parseTime( value( "serveraliveinterval", "0" ) );
			 interval && interval->asMicroseconds() > 0 )
			config.serverAliveInterval = interval;
		String::fromString( config.serverAliveCountMax, value( "serveralivecountmax", "3" ) );

		std::string preferred = value( "preferredauthentications",
									   "gssapi-with-mic,hostbased,publickey,keyboard-interactive,"
									   "password" );
		config.preferredAuthentications = String::split( preferred, ',' );
		config.pubkeyAuthentication = parseBool( value( "pubkeyauthentication", "yes" ), true );
		config.passwordAuthentication = parseBool( value( "passwordauthentication", "yes" ), true );
		config.kbdInteractiveAuthentication =
			parseBool( value( "kbdinteractiveauthentication", "yes" ), true );
		String::fromString( config.numberOfPasswordPrompts,
							value( "numberofpasswordprompts", "3" ) );
		config.batchMode = parseBool( value( "batchmode", "no" ), false );
		config.compression = parseBool( value( "compression", "no" ), false );

		auto optional = [&]( const char* keyword ) -> std::optional<std::string> {
			auto it = mValues.find( keyword );
			if ( it == mValues.end() )
				return std::nullopt;
			return joinArgs( it->second );
		};
		config.ciphers = optional( "ciphers" );
		config.macs = optional( "macs" );
		config.kexAlgorithms = optional( "kexalgorithms" );
		config.hostKeyAlgorithms = optional( "hostkeyalgorithms" );
		config.pubkeyAcceptedAlgorithms = optional( "pubkeyacceptedalgorithms" );
		config.tag = value( "tag", "" );

		mValues["hostname"] = { config.hostName };
		mValues["user"] = { config.user };
		mValues["port"] = { String::toString( config.port ) };
		config.options = std::move( mValues );
		config.warnings = std::move( mWarnings );
		mValues.clear();
		return config;
	}

	std::map<std::string, std::vector<std::string>> mValues;
	std::map<std::string, SSHConfigFile> mParsed;
	std::vector<std::string> mWarnings;
	std::string mError;
	std::string mHost;
	bool mFinalPass{ false };
	bool mWantFinal{ false };
};

SSHConfigEvaluator::SSHConfigEvaluator( SSHConfigContext context ) :
	mImpl( std::make_unique<Impl>( std::move( context ) ) ) {}

SSHConfigEvaluator::~SSHConfigEvaluator() = default;

std::variant<SSHResolvedConfig, std::string> SSHConfigEvaluator::evaluate() {
	return mImpl->evaluate();
}

std::vector<std::string>
SSHConfigEvaluator::applyAlgorithmSpec( const std::vector<std::string>& defaults,
										const std::string& spec ) {
	if ( spec.empty() )
		return defaults;
	char op = spec[0];
	std::string list = ( op == '+' || op == '-' || op == '^' ) ? spec.substr( 1 ) : spec;
	std::vector<std::string> items = String::split( list, ',' );
	std::vector<std::string> result;
	switch ( op ) {
		case '+':
			result = defaults;
			for ( const auto& item : items ) {
				if ( std::find( result.begin(), result.end(), item ) == result.end() )
					result.push_back( item );
			}
			return result;
		case '-':
			for ( const auto& def : defaults ) {
				if ( SSHPattern::matchList( def, list ) != SSHPattern::ListMatch::Match )
					result.push_back( def );
			}
			return result;
		case '^':
			result = items;
			for ( const auto& def : defaults ) {
				if ( std::find( result.begin(), result.end(), def ) == result.end() )
					result.push_back( def );
			}
			return result;
		default:
			// Plain lists may contain wildcards that select from the defaults.
			for ( const auto& item : items ) {
				if ( hasWildcard( item ) ) {
					for ( const auto& def : defaults ) {
						if ( SSHPattern::match( def, item ) &&
							 std::find( result.begin(), result.end(), def ) == result.end() )
							result.push_back( def );
					}
				} else if ( std::find( result.begin(), result.end(), item ) == result.end() ) {
					result.push_back( item );
				}
			}
			return result;
	}
}

std::optional<Time> SSHConfigEvaluator::parseTime( std::string_view value ) {
	std::string v = toLower( value );
	if ( v.empty() )
		return std::nullopt;
	if ( v == "none" )
		return std::nullopt;
	Int64 total = 0;
	size_t i = 0;
	while ( i < v.size() ) {
		if ( !std::isdigit( static_cast<unsigned char>( v[i] ) ) )
			return std::nullopt;
		Int64 number = 0;
		while ( i < v.size() && std::isdigit( static_cast<unsigned char>( v[i] ) ) )
			number = number * 10 + ( v[i++] - '0' );
		Int64 multiplier = 1;
		if ( i < v.size() ) {
			switch ( v[i] ) {
				case 's':
					multiplier = 1;
					break;
				case 'm':
					multiplier = 60;
					break;
				case 'h':
					multiplier = 3600;
					break;
				case 'd':
					multiplier = 86400;
					break;
				case 'w':
					multiplier = 604800;
					break;
				default:
					return std::nullopt;
			}
			++i;
		}
		total += number * multiplier;
	}
	return System::Seconds( static_cast<double>( total ) );
}

std::optional<SSHJumpHost> SSHConfigEvaluator::parseJumpHost( std::string_view spec ) {
	SSHJumpHost jump;
	std::string_view s( spec );
	if ( s.substr( 0, 6 ) == "ssh://" )
		s.remove_prefix( 6 );
	while ( !s.empty() && s.back() == '/' )
		s.remove_suffix( 1 );
	size_t at = s.rfind( '@' );
	if ( at != std::string_view::npos ) {
		jump.user = std::string( s.substr( 0, at ) );
		s.remove_prefix( at + 1 );
	}
	std::string_view portPart;
	if ( !s.empty() && s[0] == '[' ) {
		size_t close = s.find( ']' );
		if ( close == std::string_view::npos )
			return std::nullopt;
		jump.host = std::string( s.substr( 1, close - 1 ) );
		s.remove_prefix( close + 1 );
		if ( !s.empty() ) {
			if ( s[0] != ':' )
				return std::nullopt;
			portPart = s.substr( 1 );
		}
	} else {
		size_t colon = s.find( ':' );
		if ( colon != std::string_view::npos &&
			 s.find( ':', colon + 1 ) == std::string_view::npos ) {
			jump.host = std::string( s.substr( 0, colon ) );
			portPart = s.substr( colon + 1 );
		} else {
			jump.host = std::string( s );
		}
	}
	if ( jump.host.empty() )
		return std::nullopt;
	if ( !portPart.empty() ) {
		int port = 0;
		if ( !String::fromString( port, std::string( portPart ) ) || port <= 0 || port > 65535 )
			return std::nullopt;
		jump.port = static_cast<Uint16>( port );
	}
	return jump;
}

std::string SSHResolvedConfig::connectionKey() const {
	std::string key;
	auto add = [&key]( std::string_view part ) {
		key += String::toString( static_cast<Uint64>( part.size() ) );
		key += ':';
		key.append( part );
		key += ';';
	};
	add( hostName );
	add( String::toString( port ) );
	add( user );
	for ( const auto& jump : proxyJump ) {
		add( jump.user );
		add( jump.host );
		add( String::toString( jump.port ) );
	}
	add( proxyCommand.value_or( "" ) );
	add( identitiesOnly ? "1" : "0" );
	for ( const auto& identity : identityFiles )
		add( identity );
	for ( const auto& certificate : certificateFiles )
		add( certificate );
	add( identityAgent.value_or( "" ) );
	add( hostKeyAlias.value_or( "" ) );
	// Security policy: a connection verified or authenticated under a weaker policy must never
	// be handed to a destination that demands a stricter one.
	add( "known" );
	for ( const auto& file : userKnownHostsFiles )
		add( file );
	add( "global" );
	for ( const auto& file : globalKnownHostsFiles )
		add( file );
	add( String::toString( static_cast<int>( strictHostKeyChecking ) ) );
	add( checkHostIP ? "1" : "0" );
	for ( const auto* algorithms :
		  { &ciphers, &macs, &kexAlgorithms, &hostKeyAlgorithms, &pubkeyAcceptedAlgorithms } )
		add( algorithms->has_value() ? "+" + **algorithms : std::string() );
	add( "auth" );
	for ( const auto& method : preferredAuthentications )
		add( method );
	add( pubkeyAuthentication ? "1" : "0" );
	add( passwordAuthentication ? "1" : "0" );
	add( kbdInteractiveAuthentication ? "1" : "0" );
	add( String::toString( numberOfPasswordPrompts ) );
	add( batchMode ? "1" : "0" );
	add( compression ? "1" : "0" );
	return key;
}

// ---------------------------------------------------------------------------------------------
// System context
// ---------------------------------------------------------------------------------------------

namespace {

std::optional<std::string> systemGetEnv( const std::string& name ) {
	const char* value = std::getenv( name.c_str() );
	if ( !value )
		return std::nullopt;
	return std::string( value );
}

std::string systemLocalHostName() {
	char buffer[256] = { 0 };
	if ( gethostname( buffer, sizeof( buffer ) - 1 ) != 0 )
		return "localhost";
	return buffer;
}

#if EE_PLATFORM != EE_PLATFORM_WIN
bool systemLocalNetworkContains( const std::string& cidr ) {
	size_t slash = cidr.find( '/' );
	std::string address = cidr.substr( 0, slash );
	int bits = -1;
	if ( slash != std::string::npos && !String::fromString( bits, cidr.substr( slash + 1 ) ) )
		return false;
	unsigned char network[16];
	int family = AF_INET;
	if ( inet_pton( AF_INET, address.c_str(), network ) != 1 ) {
		if ( inet_pton( AF_INET6, address.c_str(), network ) != 1 )
			return false;
		family = AF_INET6;
	}
	int length = family == AF_INET ? 4 : 16;
	if ( bits < 0 )
		bits = length * 8;
	struct ifaddrs* addresses = nullptr;
	if ( getifaddrs( &addresses ) != 0 )
		return false;
	bool found = false;
	for ( auto* ifa = addresses; ifa && !found; ifa = ifa->ifa_next ) {
		if ( !ifa->ifa_addr || ifa->ifa_addr->sa_family != family )
			continue;
		const unsigned char* bytes =
			family == AF_INET ? reinterpret_cast<const unsigned char*>(
									&reinterpret_cast<sockaddr_in*>( ifa->ifa_addr )->sin_addr )
							  : reinterpret_cast<const unsigned char*>(
									&reinterpret_cast<sockaddr_in6*>( ifa->ifa_addr )->sin6_addr );
		bool match = true;
		for ( int bit = 0; bit < bits && match; ++bit ) {
			int byte = bit / 8, mask = 0x80 >> ( bit % 8 );
			match = ( bytes[byte] & mask ) == ( network[byte] & mask );
		}
		found = match;
	}
	freeifaddrs( addresses );
	return found;
}
#endif

std::optional<std::string> systemResolve( const std::string& fqdn ) {
	struct addrinfo hints = {};
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_CANONNAME;
	struct addrinfo* result = nullptr;
	if ( getaddrinfo( fqdn.c_str(), nullptr, &hints, &result ) != 0 || !result )
		return std::nullopt;
	std::string name = result->ai_canonname ? result->ai_canonname : fqdn;
	freeaddrinfo( result );
	return name;
}

bool systemExec( const std::string& command ) {
	System::Process process;
#if EE_PLATFORM == EE_PLATFORM_WIN
	std::string shell = "cmd";
	std::vector<std::string> args{ "/c", command };
#else
	std::string shell = systemGetEnv( "SHELL" ).value_or( "/bin/sh" );
	std::vector<std::string> args{ "-c", command };
#endif
	if ( !process.create( shell, args ) )
		return false;
	int exitCode = -1;
	process.join( &exitCode );
	return exitCode == 0;
}

} // namespace

SSHConfigContext SSHConfigContext::system( std::string originalHost ) {
	SSHConfigContext context;
	context.originalHost = std::move( originalHost );
	context.homeDirectory = System::Sys::getUserDirectory();
	System::FileSystem::dirRemoveSlashAtEnd( context.homeDirectory );
	context.localHostName = systemLocalHostName();
#if EE_PLATFORM == EE_PLATFORM_WIN
	context.localUser = systemGetEnv( "USERNAME" ).value_or( "user" );
	context.localUid = "0";
	std::string programData = systemGetEnv( "PROGRAMDATA" ).value_or( "C:/ProgramData" );
	context.sources.push_back(
		{ context.homeDirectory + "/.ssh/config", context.homeDirectory + "/.ssh" } );
	context.sources.push_back( { programData + "/ssh/ssh_config", programData + "/ssh" } );
#else
	if ( struct passwd* pw = getpwuid( getuid() ) )
		context.localUser = pw->pw_name;
	else
		context.localUser = systemGetEnv( "USER" ).value_or( "user" );
	context.localUid = String::toString( static_cast<Uint64>( getuid() ) );
	context.sources.push_back(
		{ context.homeDirectory + "/.ssh/config", context.homeDirectory + "/.ssh" } );
	if ( System::FileSystem::fileExists( "/etc/ssh/ssh_config" ) )
		context.sources.push_back( { "/etc/ssh/ssh_config", "/etc/ssh" } );
	else if ( System::FileSystem::fileExists( "/usr/etc/ssh/ssh_config" ) )
		context.sources.push_back( { "/usr/etc/ssh/ssh_config", "/etc/ssh" } );
	context.userHome = []( const std::string& user ) -> std::optional<std::string> {
		if ( struct passwd* pw = getpwnam( user.c_str() ) )
			return std::string( pw->pw_dir );
		return std::nullopt;
	};
	context.localNetworkContains = systemLocalNetworkContains;
#endif
	context.getEnv = systemGetEnv;
	context.readFile = []( const std::string& path ) -> std::optional<std::string> {
		std::string data;
		if ( !System::FileSystem::fileExists( path ) || System::FileSystem::isDirectory( path ) ||
			 !System::FileSystem::fileGet( path, data ) )
			return std::nullopt;
		return data;
	};
	context.listDirectory = []( const std::string& directory ) {
		return System::FileSystem::filesGetInPath( directory );
	};
	context.exec = systemExec;
	context.resolve = systemResolve;
	return context;
}

}} // namespace EE::VFS
