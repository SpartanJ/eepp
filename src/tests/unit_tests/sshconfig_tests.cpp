#include "utest.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/process.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/vfs/ssh/sshconfig.hpp>
#include <map>
#include <set>

using namespace EE;
using namespace EE::VFS;
using EE::System::FileSystem;
using EE::System::Sys;

namespace {

/** Deterministic context: in-memory files, fixed identity, no commands, no DNS. */
struct FakeSystem {
	std::map<std::string, std::string> files;
	std::map<std::string, std::string> env;
	std::map<std::string, std::string> dns;
	std::vector<std::string> executed;
	std::set<std::string> execSucceeds;
	std::set<std::string> networks;

	SSHConfigContext context( std::string host ) {
		SSHConfigContext ctx;
		ctx.originalHost = std::move( host );
		ctx.localUser = "tester";
		ctx.homeDirectory = "/home/tester";
		ctx.localHostName = "box.example.com";
		ctx.localUid = "1000";
		ctx.sources = { { "/home/tester/.ssh/config", "/home/tester/.ssh" },
						{ "/etc/ssh/ssh_config", "/etc/ssh" } };
		ctx.readFile = [this]( const std::string& path ) -> std::optional<std::string> {
			auto it = files.find( path );
			if ( it == files.end() )
				return std::nullopt;
			return it->second;
		};
		ctx.listDirectory = [this]( const std::string& dir ) {
			std::vector<std::string> names;
			std::string prefix = dir + "/";
			for ( const auto& file : files ) {
				if ( file.first.rfind( prefix, 0 ) == 0 &&
					 file.first.find( '/', prefix.size() ) == std::string::npos )
					names.push_back( file.first.substr( prefix.size() ) );
			}
			return names;
		};
		ctx.getEnv = [this]( const std::string& name ) -> std::optional<std::string> {
			auto it = env.find( name );
			if ( it == env.end() )
				return std::nullopt;
			return it->second;
		};
		ctx.userHome = []( const std::string& user ) -> std::optional<std::string> {
			if ( user == "other" )
				return std::string( "/home/other" );
			return std::nullopt;
		};
		ctx.exec = [this]( const std::string& command ) {
			executed.push_back( command );
			return execSucceeds.count( command ) > 0;
		};
		ctx.resolve = [this]( const std::string& name ) -> std::optional<std::string> {
			auto it = dns.find( name );
			if ( it == dns.end() )
				return std::nullopt;
			return it->second;
		};
		ctx.localNetworkContains = [this]( const std::string& cidr ) {
			return networks.count( cidr ) > 0;
		};
		return ctx;
	}

	SSHResolvedConfig resolve( std::string host, std::optional<std::string> user = {},
							   std::optional<Uint16> port = {} ) {
		auto ctx = context( std::move( host ) );
		ctx.userOverride = std::move( user );
		ctx.portOverride = port;
		auto result = SSHConfigEvaluator( std::move( ctx ) ).evaluate();
		if ( std::holds_alternative<std::string>( result ) ) {
			SSHResolvedConfig failed;
			failed.warnings.push_back( "ERROR: " + std::get<std::string>( result ) );
			return failed;
		}
		return std::get<SSHResolvedConfig>( result );
	}

	std::string error( std::string host ) {
		auto result = SSHConfigEvaluator( context( std::move( host ) ) ).evaluate();
		return std::holds_alternative<std::string>( result ) ? std::get<std::string>( result )
															 : std::string();
	}
};

const std::string UserConfig = "/home/tester/.ssh/config";
const std::string SystemConfig = "/etc/ssh/ssh_config";

} // namespace

UTEST( SSHConfig, lexerFollowsOpenSSHQuoting ) {
	std::vector<std::string> args;
	EXPECT_TRUE( SSHConfigFile::splitArguments( R"(a "b c" 'd e' f\ g "h\"i" # comment)", args ) );
	EXPECT_EQ( 5u, args.size() );
	EXPECT_STDSTREQ( "a", args[0] );
	EXPECT_STDSTREQ( "b c", args[1] );
	EXPECT_STDSTREQ( "d e", args[2] );
	EXPECT_STDSTREQ( "f g", args[3] );
	EXPECT_STDSTREQ( "h\"i", args[4] );
	args.clear();
	EXPECT_TRUE( SSHConfigFile::splitArguments( R"(a#b c\d)", args ) );
	EXPECT_STDSTREQ( "a#b", args[0] );
	EXPECT_STDSTREQ( "c\\d", args[1] );
	args.clear();
	EXPECT_FALSE( SSHConfigFile::splitArguments( R"("unterminated)", args ) );

	auto file = SSHConfigFile::parse( "  # comment\n"
									  "HOSTNAME=example.org\n"
									  "User = black  \n"
									  "Port   2222\n"
									  "ProxyCommand ssh -W %h:%p \"gw host\"\n"
									  "\tIdentityFile \"/path with/spaces\"\n"
									  "Bad\n"
									  "Broken \"quote\n",
									  "test" );
	EXPECT_EQ( 5u, file.lines.size() );
	EXPECT_STDSTREQ( "hostname", file.lines[0].keyword );
	EXPECT_STDSTREQ( "example.org", file.lines[0].args[0] );
	EXPECT_STDSTREQ( "black", file.lines[1].args[0] );
	EXPECT_STDSTREQ( "2222", file.lines[2].args[0] );
	EXPECT_STDSTREQ( "ssh -W %h:%p \"gw host\"", file.lines[3].rawArgs );
	EXPECT_STDSTREQ( "/path with/spaces", file.lines[4].args[0] );
	EXPECT_EQ( 2u, file.errors.size() );
}

UTEST( SSHConfig, patterns ) {
	EXPECT_TRUE( SSHPattern::match( "host.co.uk", "*.co.uk" ) );
	EXPECT_TRUE( SSHPattern::match( "192.168.0.5", "192.168.0.?" ) );
	EXPECT_FALSE( SSHPattern::match( "192.168.0.10", "192.168.0.?" ) );
	EXPECT_TRUE( SSHPattern::match( "anything", "*" ) );
	EXPECT_TRUE( SSHPattern::match( "", "*" ) );
	EXPECT_TRUE( SSHPattern::match( "abcbcd", "a*bcd" ) );
	EXPECT_FALSE( SSHPattern::match( "Host", "host" ) );
	EXPECT_TRUE( SSHPattern::match( "Host", "host", true ) );
	using LM = SSHPattern::ListMatch;
	EXPECT_EQ( LM::Match, SSHPattern::matchList( "a.example.com", "*.example.com,other" ) );
	EXPECT_EQ( LM::Negated, SSHPattern::matchList( "x.dialup.example.com",
												   "!*.dialup.example.com,*.example.com" ) );
	// Negations alone never produce a positive match.
	EXPECT_EQ( LM::NoMatch, SSHPattern::matchList( "host3", "!host1,!host2" ) );
	EXPECT_EQ( LM::Match, SSHPattern::matchList( "", "" ) );
}

UTEST( SSHConfig, firstValueWinsAndSourcePrecedence ) {
	FakeSystem fake;
	fake.files[UserConfig] = "Host makyo\n"
							 "    HostName 10.0.0.42\n"
							 "    User black\n"
							 "    IdentityFile ~/.ssh/id_ed25519\n"
							 "    ProxyJump gateway\n"
							 "Host *\n"
							 "    User generic\n"
							 "    Port 2200\n";
	fake.files[SystemConfig] = "Host *\n"
							   "    Port 22\n"
							   "    ServerAliveInterval 30\n"
							   "    User system\n";
	auto config = fake.resolve( "makyo" );
	EXPECT_STDSTREQ( "10.0.0.42", config.hostName );
	EXPECT_STDSTREQ( "makyo", config.originalHost );
	EXPECT_STDSTREQ( "black", config.user );
	EXPECT_EQ( 2200, config.port );
	EXPECT_EQ( 1u, config.identityFiles.size() );
	EXPECT_STDSTREQ( "/home/tester/.ssh/id_ed25519", config.identityFiles[0] );
	EXPECT_EQ( 1u, config.proxyJump.size() );
	EXPECT_STDSTREQ( "gateway", config.proxyJump[0].host );
	EXPECT_TRUE( config.serverAliveInterval.has_value() );
	EXPECT_EQ( 30, (int)config.serverAliveInterval->asSeconds() );

	auto other = fake.resolve( "elsewhere" );
	EXPECT_STDSTREQ( "elsewhere", other.hostName );
	EXPECT_STDSTREQ( "generic", other.user );
	// Defaults.
	EXPECT_EQ( 5u, other.identityFiles.size() );
	EXPECT_STDSTREQ( "/home/tester/.ssh/id_rsa", other.identityFiles[0] );
	EXPECT_EQ( 2u, other.userKnownHostsFiles.size() );
	EXPECT_STDSTREQ( "/home/tester/.ssh/known_hosts", other.userKnownHostsFiles[0] );
	EXPECT_EQ( SSHStrictHostKeyChecking::Ask, other.strictHostKeyChecking );
	EXPECT_EQ( 3u, other.serverAliveCountMax );
	EXPECT_FALSE( other.connectTimeout.has_value() );

	// URI-supplied user/port behave like command-line options and win over every file.
	auto overridden = fake.resolve( "makyo", std::string( "root" ), Uint16( 2022 ) );
	EXPECT_STDSTREQ( "root", overridden.user );
	EXPECT_EQ( 2022, overridden.port );

	// No config at all: local user, port 22.
	FakeSystem empty;
	auto bare = empty.resolve( "Host.Example" );
	EXPECT_STDSTREQ( "host.example", bare.hostName );
	EXPECT_STDSTREQ( "tester", bare.user );
	EXPECT_EQ( 22, bare.port );
}

UTEST( SSHConfig, hostNegationAndMultiplePatterns ) {
	FakeSystem fake;
	fake.files[UserConfig] = "Host *.corp !bastion.corp\n"
							 "    User corp\n"
							 "Host bastion.corp other\n"
							 "    User b\n";
	EXPECT_STDSTREQ( "corp", fake.resolve( "a.corp" ).user );
	EXPECT_STDSTREQ( "b", fake.resolve( "bastion.corp" ).user );
	EXPECT_STDSTREQ( "b", fake.resolve( "other" ).user );
	EXPECT_STDSTREQ( "tester", fake.resolve( "nothing" ).user );
}

UTEST( SSHConfig, appendAndSpecialMerges ) {
	FakeSystem fake;
	fake.env["KEYS"] = "/keys";
	fake.files[UserConfig] = "Host target\n"
							 "    IdentityFile ~/.ssh/a\n"
							 "    IdentityFile ${KEYS}/%r@%h:%p\n"
							 "    SendEnv LANG LC_* GIT_*\n"
							 "    ProxyCommand nc %h %p\n"
							 "    ProxyJump ignored\n"
							 "Host *\n"
							 "    IdentityFile ~other/id\n"
							 "    IdentityFile ${MISSING}/x\n"
							 "    SendEnv -LC_* EXTRA\n"
							 "    User u\n";
	auto config = fake.resolve( "target" );
	EXPECT_EQ( 3u, config.identityFiles.size() );
	EXPECT_STDSTREQ( "/home/tester/.ssh/a", config.identityFiles[0] );
	EXPECT_STDSTREQ( "/keys/u@target:22", config.identityFiles[1] );
	EXPECT_STDSTREQ( "/home/other/id", config.identityFiles[2] );
	EXPECT_FALSE( config.warnings.empty() ); // ${MISSING}
	auto sendEnv = config.options["sendenv"];
	EXPECT_EQ( 3u, sendEnv.size() );
	EXPECT_STDSTREQ( "LANG", sendEnv[0] );
	EXPECT_STDSTREQ( "GIT_*", sendEnv[1] );
	EXPECT_STDSTREQ( "EXTRA", sendEnv[2] );
	// ProxyCommand came first, so ProxyJump is ignored.
	EXPECT_TRUE( config.proxyJump.empty() );
	EXPECT_TRUE( config.proxyCommand.has_value() );
	EXPECT_STDSTREQ( "nc target 22", *config.proxyCommand );
}

UTEST( SSHConfig, includesAreConditionalOrderedAndCycleSafe ) {
	FakeSystem fake;
	fake.files[UserConfig] = "Include conf.d/*.conf\n"
							 "Host inc\n"
							 "    Include hosts/inc\n"
							 "Host notinc\n"
							 "    Include hosts/never\n"
							 "Host *\n"
							 "    Port 1000\n";
	fake.files["/home/tester/.ssh/conf.d/20-b.conf"] = "Host b*\n    User fromB\n";
	fake.files["/home/tester/.ssh/conf.d/10-a.conf"] = "Host *\n    User fromA\n    Port 10\n";
	fake.files["/home/tester/.ssh/conf.d/.hidden.conf"] = "User hidden\n";
	fake.files["/home/tester/.ssh/conf.d/x.txt"] = "User txt\n";
	fake.files["/home/tester/.ssh/hosts/inc"] = "HostName included.example\n"
												"Include /home/tester/.ssh/hosts/inc\n";
	fake.files["/home/tester/.ssh/hosts/never"] = "HostName never\n";
	// 10-a.conf sorts before 20-b.conf, so its unconditional values win.
	auto b = fake.resolve( "bee" );
	EXPECT_STDSTREQ( "fromA", b.user );
	EXPECT_EQ( 10, b.port );
	auto inc = fake.resolve( "inc" );
	EXPECT_STDSTREQ( "included.example", inc.hostName );
	bool cycleWarned = false;
	for ( const auto& w : inc.warnings )
		cycleWarned = cycleWarned || w.find( "cycle" ) != std::string::npos;
	EXPECT_TRUE( cycleWarned );
	// Conditional includes only apply to their own Host block.
	EXPECT_STDSTREQ( "never", fake.resolve( "notinc" ).hostName );
	EXPECT_STDSTREQ( "bee", b.hostName );

	// Deep include chains are rejected.
	FakeSystem deep;
	deep.files[UserConfig] = "Include /d/0\n";
	for ( int i = 0; i < 20; ++i )
		deep.files["/d/" + String::toString( i )] =
			"Include /d/" + String::toString( i + 1 ) + "\n";
	EXPECT_FALSE( deep.error( "x" ).empty() );
}

UTEST( SSHConfig, matchCriteria ) {
	FakeSystem fake;
	fake.execSucceeds.insert( "test-ok target" );
	fake.networks.insert( "10.0.0.0/8" );
	fake.files[UserConfig] = "Host alias\n"
							 "    HostName 10.1.2.3\n"
							 "Match host 10.* originalhost alias\n"
							 "    User matchedhost\n"
							 "Match exec \"test-fail %h\" host target\n"
							 "    Port 1\n"
							 "Match host target exec \"test-ok %h\"\n"
							 "    Port 2\n"
							 "Match localnetwork 192.168.0.0/16,10.0.0.0/8 !user nobody\n"
							 "    IdentitiesOnly yes\n"
							 "Match command sftp sessiontype subsystem\n"
							 "    Compression yes\n"
							 "Match version OpenSSH_*\n"
							 "    BatchMode yes\n"
							 "Match localuser tester\n"
							 "    ConnectTimeout 1m30s\n"
							 "Host *\n"
							 "    Tag work\n"
							 "Match tagged work\n"
							 "    CheckHostIP yes\n"
							 "Match all\n"
							 "    ServerAliveCountMax 9\n";
	auto alias = fake.resolve( "alias" );
	EXPECT_STDSTREQ( "matchedhost", alias.user );
	EXPECT_STDSTREQ( "10.1.2.3", alias.hostName );

	fake.executed.clear();
	auto target = fake.resolve( "target" );
	EXPECT_EQ( 2, target.port );
	// exec commands use tokens; the failing one ran, but no exec runs once a line already failed.
	EXPECT_EQ( 2u, fake.executed.size() );
	EXPECT_STDSTREQ( "test-fail target", fake.executed[0] );
	EXPECT_TRUE( target.identitiesOnly );
	EXPECT_TRUE( target.compression );
	EXPECT_TRUE( target.batchMode );
	EXPECT_EQ( 90, (int)target.connectTimeout->asSeconds() );
	EXPECT_TRUE( target.checkHostIP );
	EXPECT_EQ( 9u, target.serverAliveCountMax );

	fake.executed.clear();
	fake.resolve( "unrelated" );
	// "host target" fails before exec on the third line, so only the first exec runs.
	EXPECT_EQ( 1u, fake.executed.size() );

	FakeSystem bad;
	bad.files[UserConfig] = "Match all host x\n    User y\n";
	EXPECT_FALSE( bad.error( "x" ).empty() );
	bad.files[UserConfig] = "Match bogus x\n    User y\n";
	EXPECT_FALSE( bad.error( "x" ).empty() );
}

UTEST( SSHConfig, finalPassMatchesExpandedHostName ) {
	FakeSystem fake;
	fake.files[UserConfig] = "Match final host real.example.com\n"
							 "    User finaluser\n"
							 "Host real.example.com\n"
							 "    Port 2201\n"
							 "Host short\n"
							 "    HostName real.example.com\n"
							 "Match !final\n"
							 "    IdentityFile ~/.ssh/first-pass-only\n"
							 "Match canonical\n"
							 "    IdentityFile ~/.ssh/final-pass\n";
	auto config = fake.resolve( "short" );
	EXPECT_STDSTREQ( "real.example.com", config.hostName );
	EXPECT_STDSTREQ( "finaluser", config.user );
	// In the final pass Host lines match against the expanded HostName.
	EXPECT_EQ( 2201, config.port );
	EXPECT_EQ( 2u, config.identityFiles.size() );
	EXPECT_STDSTREQ( "/home/tester/.ssh/first-pass-only", config.identityFiles[0] );
	EXPECT_STDSTREQ( "/home/tester/.ssh/final-pass", config.identityFiles[1] );
}

UTEST( SSHConfig, hostnameCanonicalization ) {
	FakeSystem fake;
	fake.dns["web.b.example.com."] = "web.b.example.com";
	fake.dns["db.a.example.com."] = "db-cname.c.example.com.";
	fake.dns["app.a.example.com."] = "app-cname.blocked.com";
	fake.files[UserConfig] = "CanonicalizeHostname yes\n"
							 "CanonicalDomains a.example.com b.example.com\n"
							 "CanonicalizePermittedCNAMEs *.a.example.com:*.c.example.com\n"
							 "Match canonical host web.b.example.com\n"
							 "    User canonical\n"
							 "Host *.example.com\n"
							 "    Port 2000\n";
	auto web = fake.resolve( "web" );
	EXPECT_TRUE( web.canonicalized );
	EXPECT_STDSTREQ( "web.b.example.com", web.hostName );
	EXPECT_STDSTREQ( "canonical", web.user );
	EXPECT_EQ( 2000, web.port );
	// Permitted CNAME is followed, a non-permitted one is not.
	EXPECT_STDSTREQ( "db-cname.c.example.com", fake.resolve( "db" ).hostName );
	EXPECT_STDSTREQ( "app.a.example.com", fake.resolve( "app" ).hostName );
	// Too many dots: left to the system resolver.
	auto dotted = fake.resolve( "x.y.z" );
	EXPECT_FALSE( dotted.canonicalized );
	EXPECT_STDSTREQ( "x.y.z", dotted.hostName );
	// Unresolvable with fallback: unchanged; without fallback: hard error.
	EXPECT_STDSTREQ( "nowhere", fake.resolve( "nowhere" ).hostName );
	fake.files[UserConfig] = "CanonicalizeHostname yes\n"
							 "CanonicalizeFallbackLocal no\n"
							 "CanonicalDomains a.example.com\n";
	EXPECT_FALSE( fake.error( "nowhere" ).empty() );
	// Proxied connections are only canonicalized with "always".
	fake.files[UserConfig] = "CanonicalizeHostname yes\n"
							 "CanonicalDomains b.example.com\n"
							 "ProxyJump gw\n";
	EXPECT_STDSTREQ( "web", fake.resolve( "web" ).hostName );
	fake.files[UserConfig] = "CanonicalizeHostname always\n"
							 "CanonicalDomains b.example.com\n"
							 "ProxyJump gw\n";
	EXPECT_STDSTREQ( "web.b.example.com", fake.resolve( "web" ).hostName );
}

UTEST( SSHConfig, tokens ) {
	FakeSystem fake;
	fake.files[UserConfig] = "Host t\n"
							 "    HostName %h.example.net\n"
							 "    User bob\n"
							 "    Port 2022\n"
							 "    ProxyJump jump1,ops@jump2:2200\n"
							 "    HostKeyAlias aliaskey\n"
							 "    ControlPath ~/.ssh/cm-%C\n"
							 "    IdentityFile %d/%u/%L/%l/%i/%k/%j/%n/%%\n"
							 "    UserKnownHostsFile /kh/%h /kh2/%r\n"
							 "    IdentityAgent ~/agent.sock\n"
							 "Host bad\n"
							 "    IdentityFile /x/%Z\n"
							 "    HostName %p\n";
	auto config = fake.resolve( "t" );
	EXPECT_STDSTREQ( "t.example.net", config.hostName );
	EXPECT_STDSTREQ( "/home/tester/tester/box/box.example.com/1000/aliaskey/"
					 "jump1,ops@jump2:2200/t/%",
					 config.identityFiles[0] );
	EXPECT_STDSTREQ( "/kh/t.example.net", config.userKnownHostsFiles[0] );
	EXPECT_STDSTREQ( "/kh2/bob", config.userKnownHostsFiles[1] );
	EXPECT_STDSTREQ( "/home/tester/agent.sock", *config.identityAgent );
	std::string expectedHash = "box.example.comt.example.net2022bobjump1,ops@jump2:2200";
	EXPECT_EQ( 1u, config.options["controlpath"].size() );
	EXPECT_EQ( std::string( "/home/tester/.ssh/cm-" ).size() + 40,
			   config.options["controlpath"][0].size() );
	EXPECT_EQ( 2u, config.proxyJump.size() );
	EXPECT_STDSTREQ( "ops", config.proxyJump[1].user );
	EXPECT_STDSTREQ( "jump2", config.proxyJump[1].host );
	EXPECT_EQ( 2200, config.proxyJump[1].port );

	auto bad = fake.resolve( "bad" );
	// Unknown tokens drop the value (with a warning); HostName only accepts %h.
	bool unknownToken = false;
	for ( const auto& w : bad.warnings )
		unknownToken = unknownToken || w.find( "%Z" ) != std::string::npos;
	EXPECT_TRUE( unknownToken );
	EXPECT_STDSTREQ( "bad", bad.hostName );
}

UTEST( SSHConfig, helpers ) {
	auto jump = SSHConfigEvaluator::parseJumpHost( "ssh://user@[2001:db8::1]:2222" );
	EXPECT_TRUE( jump.has_value() );
	EXPECT_STDSTREQ( "2001:db8::1", jump->host );
	EXPECT_EQ( 2222, jump->port );
	EXPECT_STDSTREQ( "user", jump->user );
	auto bare = SSHConfigEvaluator::parseJumpHost( "2001:db8::2" );
	EXPECT_STDSTREQ( "2001:db8::2", bare->host );
	EXPECT_EQ( 0, bare->port );
	EXPECT_FALSE( SSHConfigEvaluator::parseJumpHost( "host:99999" ).has_value() );

	EXPECT_EQ( 0, (int)SSHConfigEvaluator::parseTime( "0" )->asSeconds() );
	EXPECT_EQ( 5430, (int)SSHConfigEvaluator::parseTime( "1h30m30s" )->asSeconds() );
	EXPECT_FALSE( SSHConfigEvaluator::parseTime( "none" ).has_value() );
	EXPECT_FALSE( SSHConfigEvaluator::parseTime( "5x" ).has_value() );

	std::vector<std::string> defaults{ "a", "b-sha1", "c" };
	auto plus = SSHConfigEvaluator::applyAlgorithmSpec( defaults, "+d,a" );
	EXPECT_EQ( 4u, plus.size() );
	auto minus = SSHConfigEvaluator::applyAlgorithmSpec( defaults, "-*sha1" );
	EXPECT_EQ( 2u, minus.size() );
	auto caret = SSHConfigEvaluator::applyAlgorithmSpec( defaults, "^c,z" );
	EXPECT_STDSTREQ( "c", caret[0] );
	EXPECT_STDSTREQ( "z", caret[1] );
	EXPECT_EQ( 4u, caret.size() );
	auto replace = SSHConfigEvaluator::applyAlgorithmSpec( defaults, "x,b*" );
	EXPECT_EQ( 2u, replace.size() );
	EXPECT_STDSTREQ( "b-sha1", replace[1] );
}

UTEST( SSHConfig, connectionKeyIgnoresPathsAndDistinguishesProfiles ) {
	FakeSystem fake;
	fake.files[UserConfig] = "Host makyo\n    User black\n";
	auto a = fake.resolve( "makyo" );
	auto b = fake.resolve( "makyo" );
	EXPECT_STDSTREQ( a.connectionKey(), b.connectionKey() );
	auto c = fake.resolve( "makyo", std::string( "root" ) );
	EXPECT_TRUE( a.connectionKey() != c.connectionKey() );
}

#if EE_PLATFORM != EE_PLATFORM_WIN
namespace {

std::map<std::string, std::vector<std::string>> sshDashG( const std::string& file,
														  const std::string& host ) {
	std::map<std::string, std::vector<std::string>> values;
	System::Process process;
	if ( !process.create( "ssh", std::vector<std::string>{ "-G", "-F", file, host } ) )
		return values;
	std::string out;
	process.readAllStdOut( out );
	int code = -1;
	process.join( &code );
	if ( code != 0 )
		return values;
	for ( const auto& line : String::split( out, '\n' ) ) {
		size_t space = line.find( ' ' );
		if ( space == std::string::npos )
			continue;
		std::string key = line.substr( 0, space );
		std::string value = line.substr( space + 1 );
		// Multi-valued directives print one value per line (identityfile) or space separated
		// (userknownhostsfile).
		if ( key == "userknownhostsfile" || key == "sendenv" ) {
			for ( auto& part : String::split( value, ' ' ) )
				values[key].push_back( part );
		} else {
			values[key].push_back( value );
		}
	}
	return values;
}

} // namespace

/** Differential test against the real OpenSSH client on the subset of options the evaluator
 * claims to implement. Skipped when `ssh` is not installed. */
UTEST( SSHConfig, differentialAgainstSshDashG ) {
	if ( Sys::which( "ssh" ).empty() ) {
		printf( "  ssh not found, differential test skipped\n" );
		return;
	}
	std::string dir = Sys::getTempPath();
	FileSystem::dirAddSlashAtEnd( dir );
	dir += "eepp-sshconfig-" + String::toString( Sys::getProcessID() );
	FileSystem::makeDir( dir + "/conf.d", true );

	struct Fixture {
		std::string config;
		std::vector<std::string> hosts;
	};
	std::vector<Fixture> fixtures = {
		{ "Host makyo\n"
		  "    HostName 10.0.0.%h\n"
		  "    User black\n"
		  "    IdentityFile ~/.ssh/id_%r_%h\n"
		  "    IdentityFile /x/%p-%n\n"
		  "    ProxyJump gateway\n"
		  "    Port 2222\n"
		  "Host *\n"
		  "    User other\n"
		  "    IdentityFile /generic\n"
		  "    UserKnownHostsFile ~/.ssh/kh1 /abs/kh2-%h\n"
		  "    ControlPath ~/.ssh/cm-%C\n",
		  { "makyo", "other.host" } },
		{ "Host !bastion *.corp\n"
		  "    User corpuser\n"
		  "Host bastion\n"
		  "    User b\n"
		  "    ProxyCommand nc %h %p\n"
		  "    ProxyJump never\n"
		  "Host *\n"
		  "    Port 2200\n"
		  "    IdentityFile ~/.ssh/id_test\n"
		  "    IdentitiesOnly yes\n"
		  "    ConnectTimeout 7\n",
		  { "a.corp", "bastion", "x" } },
		{ "Match host 10.* user root\n"
		  "    Port 1001\n"
		  "Host alias\n"
		  "    HostName 10.1.2.3\n"
		  "    User root\n"
		  "Match originalhost alias\n"
		  "    IdentityFile ~/.ssh/a\n"
		  "Match final host 10.1.2.3\n"
		  "    ProxyJump j1,j2:2022\n"
		  "Host 10.1.2.3\n"
		  "    UserKnownHostsFile /final-pass-host\n"
		  "Match all\n"
		  "    IdentityFile ~/.ssh/all\n"
		  "    ConnectTimeout 5\n",
		  { "alias", "10.9.9.9", "plain" } },
		{ "Include " + dir +
			  "/conf.d/*.conf\n"
			  "Host *\n"
			  "    IdentityFile ~/.ssh/base\n"
			  "    User = \"quoted user\"\n",
		  { "inc", "beta" } },
		{ "Host *\n"
		  "    Tag work\n"
		  "Match tagged work\n"
		  "    User tagged\n"
		  "    SendEnv LANG LC_*\n"
		  "    SendEnv -LC_* EXTRA\n"
		  "    IdentityFile ~/.ssh/t\n",
		  { "t" } },
	};
	FileSystem::fileWrite( dir + "/conf.d/10-a.conf",
						   std::string_view( "Host inc\n    HostName inc.example\n"
											 "    Port 3000\n" ) );
	FileSystem::fileWrite( dir + "/conf.d/20-b.conf",
						   std::string_view( "Host b*\n    User fromB\n    Port 4000\n" ) );

	const std::vector<std::string> keys = {
		"hostname",			  "user",			"port",			"identityfile",
		"userknownhostsfile", "proxyjump",		"proxycommand", "controlpath",
		"identitiesonly",	  "connecttimeout", "sendenv" };
	int compared = 0;
	for ( size_t f = 0; f < fixtures.size(); ++f ) {
		std::string file = dir + "/fixture" + String::toString( static_cast<Uint64>( f ) );
		FileSystem::fileWrite( file, std::string_view( fixtures[f].config ) );
		for ( const auto& host : fixtures[f].hosts ) {
			auto expected = sshDashG( file, host );
			if ( expected.empty() ) {
				printf( "  ssh -G failed for fixture %zu host %s\n", f, host.c_str() );
				continue;
			}
			auto ctx = SSHConfigContext::system( host );
			ctx.sources = { { file, dir } };
			ctx.exec = nullptr;
			ctx.resolve = nullptr;
			auto result = SSHConfigEvaluator( std::move( ctx ) ).evaluate();
			EXPECT_TRUE( std::holds_alternative<SSHResolvedConfig>( result ) );
			if ( !std::holds_alternative<SSHResolvedConfig>( result ) )
				continue;
			auto& config = std::get<SSHResolvedConfig>( result );
			for ( const auto& key : keys ) {
				auto ours =
					config.options.count( key ) ? config.options[key] : std::vector<std::string>();
				auto theirs = expected.count( key ) ? expected[key] : std::vector<std::string>();
				if ( key == "identitiesonly" && ours.empty() )
					ours = { "no" };
				if ( key == "connecttimeout" && ours.empty() )
					ours = { "none" };
				if ( key == "identityfile" && config.options.count( "identityfile" ) &&
					 theirs.size() > 0 && ours.size() == 5 && ours[0] == "~/.ssh/id_rsa" )
					continue; // OpenSSH's default identity list differs between versions.
				std::string a = String::join( ours, '|' );
				std::string b = String::join( theirs, '|' );
				if ( a != b )
					printf( "  mismatch fixture %zu host %s key %s: ours=[%s] ssh=[%s]\n", f,
							host.c_str(), key.c_str(), a.c_str(), b.c_str() );
				EXPECT_STDSTREQ( b, a );
				++compared;
			}
		}
	}
	EXPECT_GT( compared, 50 );
	FileSystem::dirRemoveAll( dir );
}
#endif
