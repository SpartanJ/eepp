#ifndef EE_VFS_SSH_SSHCONFIG_HPP
#define EE_VFS_SSH_SSHCONFIG_HPP

#include <eepp/config.hpp>
#include <eepp/system/time.hpp>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace EE { namespace VFS {

using EE::System::Time;

/** @brief OpenSSH-style pattern matching (ssh_config(5) "PATTERNS"). */
class SSHPattern {
  public:
	enum class ListMatch : Int8 {
		/** A negated pattern matched: the whole list is rejected. */
		Negated = -1,
		NoMatch = 0,
		Match = 1,
	};

	/** Matches str against a single pattern with '*' and '?' wildcards. */
	static bool match( std::string_view str, std::string_view pattern,
					   bool caseInsensitive = false );

	/** Matches str against a comma-separated pattern list that may contain negated ("!pat")
	 * entries. A negated match always wins; negations alone never produce a positive match. */
	static ListMatch matchList( std::string_view str, std::string_view patternList,
								bool caseInsensitive = false );
};

/** One "Keyword arguments" line of an ssh_config file. */
struct SSHConfigLine {
	/** Lowercased keyword. */
	std::string keyword;
	/** Arguments split with OpenSSH quoting rules. */
	std::vector<std::string> args;
	/** Unsplit remainder after the keyword (used by command directives such as ProxyCommand). */
	std::string rawArgs;
	Uint32 line{ 0 };
};

/** @brief Parsed, ordered representation of one ssh_config file.
 *
 * Parsing only tokenizes: directives are kept in file order so the evaluator can apply OpenSSH's
 * ordering, first-value-wins, Host/Match activation and Include semantics.
 */
class SSHConfigFile {
  public:
	static SSHConfigFile parse( std::string_view text, std::string path = {} );

	/** Splits a line into arguments like OpenSSH's argv_split(): double or single quotes group
	 * words, backslash escapes quotes, backslashes and (outside quotes) spaces, and an unquoted
	 * '#' at the start of an argument starts a comment. @return False on unterminated quotes. */
	static bool splitArguments( std::string_view line, std::vector<std::string>& args );

	std::string path;
	std::vector<SSHConfigLine> lines;
	std::vector<std::string> errors;
};

enum class SSHStrictHostKeyChecking : Uint8 {
	/** Unknown keys require user confirmation; changed keys are refused. */
	Ask,
	/** Unknown and changed keys are refused. */
	Yes,
	/** Unknown keys are accepted and recorded; changed keys are refused. */
	AcceptNew,
	/** Unknown keys are accepted and recorded; changed keys are refused (OpenSSH also refuses
	 * changed keys with "no", only relaxing some protections). */
	No,
};

struct SSHJumpHost {
	std::string user;
	std::string host;
	/** 0 when not specified (the jump host's own config / default applies). */
	Uint16 port{ 0 };

	bool operator==( const SSHJumpHost& ) const = default;
};

/** @brief Typed effective client configuration for one destination.
 *
 * Paths are fully expanded (tilde, tokens, environment). The raw evaluated option values are kept
 * in `options` for diagnostics and `ssh -G` style dumps.
 */
struct SSHResolvedConfig {
	std::string originalHost;
	/** Effective connection host (HostName applied, canonicalized when enabled). */
	std::string hostName;
	std::string user;
	Uint16 port{ 22 };

	/** Expanded identity file paths, in order. Defaults apply when none are configured. */
	std::vector<std::string> identityFiles;
	std::vector<std::string> certificateFiles;
	/** Agent socket/pipe path; nullopt when agent use is disabled ("IdentityAgent none"). */
	std::optional<std::string> identityAgent;
	bool identitiesOnly{ false };

	std::vector<std::string> userKnownHostsFiles;
	std::vector<std::string> globalKnownHostsFiles;
	SSHStrictHostKeyChecking strictHostKeyChecking{ SSHStrictHostKeyChecking::Ask };
	std::optional<std::string> hostKeyAlias;
	bool checkHostIP{ false };

	std::vector<SSHJumpHost> proxyJump;
	/** Expanded ProxyCommand, nullopt when unset or "none". */
	std::optional<std::string> proxyCommand;

	std::optional<Time> connectTimeout;
	std::optional<Time> serverAliveInterval;
	Uint32 serverAliveCountMax{ 3 };

	std::vector<std::string> preferredAuthentications;
	bool pubkeyAuthentication{ true };
	bool passwordAuthentication{ true };
	bool kbdInteractiveAuthentication{ true };
	Uint32 numberOfPasswordPrompts{ 3 };
	bool batchMode{ false };
	bool compression{ false };

	/** Algorithm preference specifications as configured (with OpenSSH +/-/^ modifiers). Apply
	 * with SSHConfigEvaluator::applyAlgorithmSpec() against the backend's supported list. */
	std::optional<std::string> ciphers;
	std::optional<std::string> macs;
	std::optional<std::string> kexAlgorithms;
	std::optional<std::string> hostKeyAlgorithms;
	std::optional<std::string> pubkeyAcceptedAlgorithms;

	std::string tag;
	/** True when hostname canonicalization rewrote the host. */
	bool canonicalized{ false };

	/** Every evaluated directive (lowercase keyword -> values), first-wins or accumulated. */
	std::map<std::string, std::vector<std::string>> options;
	/** Non-fatal problems (unknown directives, failed expansions, unreadable includes). */
	std::vector<std::string> warnings;

	/** @return A key identifying meaningfully different SSH sessions (host, port, user, proxy
	 * chain, authentication profile, host key alias). Never contains a filesystem path of the
	 * remote side. */
	std::string connectionKey() const;
};

/** Environment of an evaluation. Everything that touches the system is injectable so tests stay
 * deterministic and never execute commands or resolve names. */
struct SSHConfigContext {
	/** Host as given by the user (URI host). */
	std::string originalHost;
	/** Command-line style overrides (from the URI): they win over every config file. */
	std::optional<std::string> userOverride;
	std::optional<Uint16> portOverride;

	std::string localUser;
	std::string homeDirectory;
	/** Full local host name (%l); %L is derived from it. */
	std::string localHostName;
	std::string localUid;
	/** Values for Match command / sessiontype. SFTP uses "sftp" and "subsystem". */
	std::string command{ "sftp" };
	std::string sessionType{ "subsystem" };
	/** Matched by "Match version". */
	std::string version{ "OpenSSH_10.0" };

	/** Configuration files in precedence order (normally user, then system). Relative Include
	 * paths resolve against `includeBase` of the including file's source. */
	struct Source {
		std::string path;
		std::string includeBase;
	};
	std::vector<Source> sources;

	std::function<std::optional<std::string>( const std::string& name )> getEnv;
	std::function<std::optional<std::string>( const std::string& path )> readFile;
	/** Lists directory entry names (for Include globs). */
	std::function<std::vector<std::string>( const std::string& directory )> listDirectory;
	/** Home directory of another user (for "~user/"), nullopt if unknown. */
	std::function<std::optional<std::string>( const std::string& user )> userHome;
	/** Runs a "Match exec" command; nullptr disables exec (never matches, with a warning). */
	std::function<bool( const std::string& command )> exec;
	/** Resolves a fully qualified name for canonicalization. Returns the canonical name (CNAME
	 * target, or the name itself) or nullopt if it does not resolve. */
	std::function<std::optional<std::string>( const std::string& fqdn )> resolve;
	/** Returns true if any local interface address is inside the CIDR network. */
	std::function<bool( const std::string& cidr )> localNetworkContains;

	/** @return A context backed by the real system: current user, real files, environment,
	 * process execution for Match exec, DNS for canonicalization, ~/.ssh/config and the system
	 * ssh_config as sources. */
	static SSHConfigContext system( std::string originalHost );
};

/** @brief Evaluates ssh_config files for a destination with OpenSSH semantics.
 *
 * Implements first-value-wins with table-driven per-directive merge rules, Host and Match
 * activation (including negation and all client criteria), conditional Include with globs and
 * cycle/depth protection, hostname canonicalization and the canonical/final re-parse pass, and
 * context-aware token and environment expansion.
 */
class SSHConfigEvaluator {
  public:
	explicit SSHConfigEvaluator( SSHConfigContext context );

	~SSHConfigEvaluator();

	/** @return The resolved configuration, or an error message for fatal problems (for example
	 * canonicalization failure with CanonicalizeFallbackLocal no). */
	std::variant<SSHResolvedConfig, std::string> evaluate();

	/** Applies an OpenSSH algorithm specification to a default list: "+a,b" appends, "-a*"
	 * removes matching entries, "^a,b" prepends, anything else replaces. */
	static std::vector<std::string> applyAlgorithmSpec( const std::vector<std::string>& defaults,
														const std::string& spec );

	/** Parses OpenSSH time formats ("30", "1m30s", "2h", "none"). */
	static std::optional<Time> parseTime( std::string_view value );

	/** Parses "[user@]host[:port]" or "ssh://[user@]host[:port]" jump specifications. */
	static std::optional<SSHJumpHost> parseJumpHost( std::string_view spec );

  private:
	class Impl;
	std::unique_ptr<Impl> mImpl;
};

}} // namespace EE::VFS

#endif
