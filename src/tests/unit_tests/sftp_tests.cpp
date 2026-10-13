#include "utest.hpp"
#include <eepp/config.hpp>

#if defined( EE_VFS_SFTP ) && EE_PLATFORM != EE_PLATFORM_WIN

#include "vfs_test_utils.hpp"
#include <cstdlib>
#include <eepp/system/process.hpp>
#include <eepp/vfs/sftp/sftpprovider.hpp>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace vfstest;
using EE::System::Process;

namespace {

/** Throwaway OpenSSH server on a random loopback port, shared by the SFTP tests. */
class TestSSHServer {
  public:
	static TestSSHServer& instance() {
		static TestSSHServer server;
		return server;
	}

	bool available() const { return mAvailable; }

	const std::string& skipReason() const { return mSkipReason; }

	const std::string& dir() const { return mDir; }

	int port() const { return mPort; }

	std::string remoteRoot() const { return mDir + "remote"; }

	/** Builds a provider whose OpenSSH configuration is the test ssh_config. */
	std::shared_ptr<SFTPProvider> provider( SFTPProviderConfig config = {} ) const {
		std::string dir = mDir;
		config.configContextFactory = [dir]( const std::string& host ) {
			SSHConfigContext context = SSHConfigContext::system( host );
			context.sources = { { dir + "ssh_config", dir } };
			context.exec = nullptr;
			context.resolve = nullptr;
			return context;
		};
		if ( config.watchAgentCommand == "efsw-agent --stdio" )
			config.watchAgentCommand.clear();
		config.defaultConnectTimeout = System::Seconds( 10 );
		return std::make_shared<SFTPProvider>( std::move( config ) );
	}

	URI uri( const std::string& host, const std::string& relative = {} ) const {
		URI uri;
		uri.setScheme( "sftp" );
		uri.setHost( host );
		uri.setRawPath( remoteRoot() + ( relative.empty() ? "" : "/" + relative ) );
		return uri;
	}

	~TestSSHServer() {
		if ( mServer )
			mServer->kill();
		mServer.reset();
		if ( !mDir.empty() )
			FileSystem::dirRemoveAll( mDir );
	}

  private:
	TestSSHServer() { mAvailable = start(); }

	static bool run( const std::string& command, const std::vector<std::string>& args ) {
		Process process;
		if ( !process.create( command, args ) )
			return false;
		std::string out;
		process.readAllStdOut( out );
		int code = -1;
		process.join( &code );
		return code == 0;
	}

	static bool portOpen( int port ) {
		int sock = ::socket( AF_INET, SOCK_STREAM, 0 );
		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_port = htons( static_cast<uint16_t>( port ) );
		addr.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
		bool open = ::connect( sock, reinterpret_cast<sockaddr*>( &addr ), sizeof( addr ) ) == 0;
		::close( sock );
		return open;
	}

	bool start() {
		std::string sshd = Sys::which( "sshd" );
		if ( sshd.empty() && FileSystem::fileExists( "/usr/sbin/sshd" ) )
			sshd = "/usr/sbin/sshd";
		std::string keygen = Sys::which( "ssh-keygen" );
		if ( sshd.empty() || keygen.empty() ) {
			mSkipReason = "sshd/ssh-keygen not found";
			return false;
		}
		mDir = Sys::getTempPath();
		FileSystem::dirAddSlashAtEnd( mDir );
		mDir += "eepp-sftp-test-" + String::toString( Sys::getProcessID() ) + "-" +
				String::toString( Sys::getTicks() ) + "/";
		FileSystem::makeDir( remoteRoot(), true );
		for ( const char* key : { "host_ecdsa", "id_ecdsa", "other_ecdsa" } ) {
			if ( !run( keygen, { "-q", "-t", "ecdsa", "-N", "", "-f", mDir + key } ) ) {
				mSkipReason = "ssh-keygen failed";
				return false;
			}
		}
		if ( !run( keygen, { "-q", "-t", "ecdsa", "-N", "secret", "-f", mDir + "id_secret" } ) ||
			 !run( keygen, { "-q", "-t", "ed25519", "-N", "", "-f", mDir + "host_ed25519" } ) ||
			 !run( keygen, { "-q", "-t", "ed25519", "-N", "", "-f", mDir + "id_ed25519" } ) ||
			 !run( keygen, { "-q", "-t", "ed25519", "-N", "", "-f", mDir + "id_ed25519_nopub" } ) ||
			 !run( keygen, { "-q", "-t", "ed25519", "-N", "edsecret", "-f",
							 mDir + "id_ed25519_secret" } ) ) {
			mSkipReason = "ssh-keygen failed";
			return false;
		}
		std::string authorized, pub;
		for ( const char* key :
			  { "id_ecdsa", "id_secret", "id_ed25519", "id_ed25519_nopub", "id_ed25519_secret" } ) {
			FileSystem::fileGet( mDir + key + ".pub", pub );
			authorized += pub;
			::chmod( ( mDir + key ).c_str(), 0600 );
		}
		// Without a ".pub" file the public key is derived from the OpenSSH private key.
		FileSystem::fileRemove( mDir + "id_ed25519_nopub.pub" );
		FileSystem::fileWrite( mDir + "authorized_keys", std::string_view( authorized ) );

		std::string user = SSHConfigContext::system( "x" ).localUser;
		for ( int attempt = 0; attempt < 5 && !mServer; ++attempt ) {
			mPort =
				20000 + static_cast<int>(
							( Sys::getProcessID() * 7 + attempt * 131 + Sys::getTicks() ) % 20000 );
			if ( portOpen( mPort ) )
				continue;
			std::string config = "Port " + String::toString( mPort ) +
								 "\n"
								 "ListenAddress 127.0.0.1\n"
								 "HostKey " +
								 mDir +
								 "host_ecdsa\n"
								 "HostKey " +
								 mDir +
								 "host_ed25519\n"
								 // Every connection negotiates curve25519-sha256.
								 "KexAlgorithms curve25519-sha256,curve25519-sha256@libssh.org\n"
								 "PidFile none\n"
								 "AuthorizedKeysFile " +
								 mDir +
								 "authorized_keys\n"
								 "StrictModes no\n"
								 "UsePAM no\n"
								 "PermitRootLogin yes\n"
								 "PasswordAuthentication yes\n"
								 "KbdInteractiveAuthentication no\n"
								 "AllowTcpForwarding yes\n"
								 "Subsystem sftp internal-sftp\n"
								 "LogLevel ERROR\n"
								 // Tests fail authentication on purpose; OpenSSH >= 9.8 would
								 // otherwise penalize (refuse) the loopback source afterwards.
								 "PerSourcePenalties no\n";
			FileSystem::fileWrite( mDir + "sshd_config", std::string_view( config ) );
			auto server = std::make_unique<Process>();
			if ( !server->create( sshd, { "-D", "-e", "-f", mDir + "sshd_config" } ) )
				continue;
			for ( int i = 0; i < 100 && server->isAlive() && !portOpen( mPort ); ++i )
				std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
			if ( portOpen( mPort ) )
				mServer = std::move( server );
			else
				server->kill();
		}
		if ( !mServer ) {
			mSkipReason = "could not start sshd";
			return false;
		}

		std::string port = String::toString( mPort );
		std::string common = "    HostName 127.0.0.1\n"
							 "    Port " +
							 port +
							 "\n"
							 "    User " +
							 user +
							 "\n"
							 "    IdentitiesOnly yes\n"
							 "    IdentityAgent none\n"
							 "    GlobalKnownHostsFile /dev/null\n"
							 "    PreferredAuthentications publickey,password\n";
		// IdentityFile accumulates across every matching block, so the default key comes from a
		// negated block that excludes the passphrase test host.
		std::string sshConfig =
			"Host strict\n    StrictHostKeyChecking yes\n    UserKnownHostsFile " + mDir +
			"kh_strict\n" + common + "Host ask\n    StrictHostKeyChecking ask\n" +
			"    UserKnownHostsFile " + mDir + "kh_ask\n" + common +
			"Host mismatch\n    HostKeyAlias mismatch-alias\n    UserKnownHostsFile " + mDir +
			"kh_mismatch\n" + common + "Host secret\n    IdentityFile " + mDir + "id_secret\n" +
			common + "Host pw\n    PreferredAuthentications password\n" +
			"    NumberOfPasswordPrompts 2\n" + common + "Host jumped\n    ProxyJump testhost\n" +
			common + "Host proxied\n    ProxyCommand nc 127.0.0.1 " + port + "\n" + common +
			"Host edkey\n    IdentityFile " + mDir + "id_ed25519\n" + common +
			"Host ednopub\n    IdentityFile " + mDir + "id_ed25519_nopub\n" + common +
			"Host edsecret\n    IdentityFile " + mDir + "id_ed25519_secret\n" + common +
			// Only an Ed25519 key is known, and the ECDSA key the server also offers is
			// preferred by default: connecting needs the known-type ordering.
			"Host edhost\n    StrictHostKeyChecking yes\n    UserKnownHostsFile " + mDir +
			"kh_ed25519\n" + common +
			"Host * !secret !edkey !ednopub !edsecret\n    IdentityFile " + mDir + "id_ecdsa\n" +
			"Host *\n    StrictHostKeyChecking accept-new\n    UserKnownHostsFile " + mDir +
			"known_hosts\n" + common;
		FileSystem::fileWrite( mDir + "ssh_config", std::string_view( sshConfig ) );
		// A known_hosts entry with the wrong key for the mismatch alias.
		std::string other;
		FileSystem::fileGet( mDir + "other_ecdsa.pub", other );
		auto fields = String::split( other, ' ' );
		FileSystem::fileWrite(
			mDir + "kh_mismatch",
			std::string_view( "mismatch-alias " + fields[0] + " " + fields[1] + "\n" ) );
		std::string hostEd;
		FileSystem::fileGet( mDir + "host_ed25519.pub", hostEd );
		fields = String::split( hostEd, ' ' );
		FileSystem::fileWrite(
			mDir + "kh_ed25519",
			std::string_view( "[127.0.0.1]:" + port + " " + fields[0] + " " + fields[1] + "\n" ) );
		return true;
	}

	std::string mDir;
	std::string mSkipReason;
	std::unique_ptr<Process> mServer;
	int mPort{ 0 };
	bool mAvailable{ false };
};

/** Stops the test (with the error printed) when a VFS result failed. */
#define REQUIRE_OK( result )                                                           \
	do {                                                                               \
		if ( !( result ) ) {                                                           \
			printf( "  %s failed: %s\n", #result, result.error().toString().c_str() ); \
			EXPECT_TRUE( false );                                                      \
			return;                                                                    \
		}                                                                              \
	} while ( 0 )

#define REQUIRE_SSHD()                                                                  \
	auto& server = TestSSHServer::instance();                                           \
	if ( !server.available() ) {                                                        \
		printf( "  SFTP integration test skipped: %s\n", server.skipReason().c_str() ); \
		return;                                                                         \
	}

class FixedVerifier : public SSHHostKeyVerifier {
  public:
	explicit FixedVerifier( SSHHostKeyDecision decision ) : mDecision( decision ) {}

	Task<SSHHostKeyDecision> verify( const SSHHostKeyInfo& info ) override {
		++calls;
		lastInfo = info;
		return makeReadyTask( mDecision );
	}

	std::atomic<int> calls{ 0 };
	SSHHostKeyInfo lastInfo;

  private:
	SSHHostKeyDecision mDecision;
};

class ScriptedAuthenticator : public SSHAuthenticationHandler {
  public:
	explicit ScriptedAuthenticator( std::vector<std::optional<std::string>> answers ) :
		mAnswers( std::move( answers ) ) {}

	Task<std::optional<std::vector<std::string>>>
	request( const SSHAuthenticationRequest& request ) override {
		std::lock_guard<std::mutex> lock( mMutex );
		requests.push_back( request );
		std::optional<std::vector<std::string>> answer;
		if ( mNext < mAnswers.size() && mAnswers[mNext] )
			answer = std::vector<std::string>{ *mAnswers[mNext] };
		++mNext;
		// Answer asynchronously from another thread, like a UI would.
		TaskSource<std::optional<std::vector<std::string>>> source;
		auto task = source.task();
		std::thread( [source = std::move( source ), answer]() mutable {
			std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
			source.resolve( answer );
		} ).detach();
		return task;
	}

	std::vector<SSHAuthenticationRequest> requests;

  private:
	std::mutex mMutex;
	std::vector<std::optional<std::string>> mAnswers;
	size_t mNext{ 0 };
};

struct SFTPFixture {
	VirtualFileSystem vfs;
	std::shared_ptr<SFTPProvider> sftp;

	explicit SFTPFixture( SFTPProviderConfig config = {} ) {
		sftp = TestSSHServer::instance().provider( std::move( config ) );
		vfs.registerProvider( sftp );
		vfs.watchManager().registerBackend( "sftp", sftp->createWatchBackend( vfs ) );
	}

	~SFTPFixture() {
		vfs.watchManager().unregisterBackend( "sftp" );
		vfs.unregisterProvider( "sftp" );
	}
};

std::string uniqueName( const char* prefix ) {
	static std::atomic<int> counter{ 0 };
	return std::string( prefix ) + String::toString( ++counter );
}

} // namespace

UTEST( SFTP, conformanceOverRealServer ) {
	REQUIRE_SSHD();
	SFTPFixture fx;
	std::string name = uniqueName( "conformance" );
	FileSystem::makeDir( server.remoteRoot() + "/" + name );
	providerConformance( utest_result, fx.vfs, server.uri( "testhost", name ) );
	// accept-new recorded the host key.
	std::string known;
	FileSystem::fileGet( server.dir() + "known_hosts", known );
	EXPECT_TRUE( known.find( "[127.0.0.1]:" + String::toString( server.port() ) ) !=
				 std::string::npos );
	// All operations shared one SSH connection.
	EXPECT_EQ( 1u, fx.sftp->connectionCount() );
	auto caps = fx.vfs.capabilities( server.uri( "testhost" ) );
	EXPECT_TRUE( caps.has( VFSCapability::AtomicReplace ) );
	EXPECT_TRUE( caps.has( VFSCapability::Fsync ) );
	EXPECT_TRUE( caps.has( VFSCapability::FreeSpace ) );
	auto space = waitFor( fx.vfs.spaceInfo( server.uri( "testhost" ) ) );
	REQUIRE_OK( space );
	EXPECT_TRUE( space.value().capacity > 0 );
}

UTEST( SFTP, metadataOwnershipAndSymlinks ) {
	REQUIRE_SSHD();
	SFTPFixture fx;
	std::string name = uniqueName( "meta" );
	std::string local = server.remoteRoot() + "/" + name;
	FileSystem::makeDir( local + "/dir", true );
	FileSystem::fileWrite( local + "/file.txt", std::string_view( "12345" ) );
	EXPECT_EQ( 0, ::symlink( "dir", ( local + "/link" ).c_str() ) );
	auto listed = waitFor( fx.vfs.listAll( server.uri( "testhost", name ) ) );
	REQUIRE_OK( listed );
	EXPECT_EQ( 3u, listed.value().size() );
	for ( const auto& entry : listed.value() ) {
		EXPECT_FALSE( entry.owner.empty() );
		if ( entry.name == "file.txt" ) {
			EXPECT_EQ( 5u, *entry.size() );
			EXPECT_TRUE( entry.modifiedTime().has_value() );
		}
		if ( entry.name == "link" ) {
			EXPECT_EQ( FileType::Symlink, entry.type );
			EXPECT_EQ( FileType::Directory, entry.targetType );
		}
	}
	auto lstat =
		waitFor( fx.vfs.stat( server.uri( "testhost", name + "/link" ), StatOptions{ false } ) );
	EXPECT_STDSTREQ( "dir", lstat.value().symlinkTarget );
	auto canonical = waitFor( fx.vfs.canonicalize( server.uri( "testhost", name + "/link" ) ) );
	REQUIRE_OK( canonical );
	EXPECT_STDSTREQ( "dir", canonical.value().getLastPathSegment() );
	EXPECT_STDSTREQ( "testhost", canonical.value().getHost() );
}

UTEST( SFTP, atomicWritesDetectConflicts ) {
	REQUIRE_SSHD();
	SFTPFixture fx;
	std::string name = uniqueName( "atomic" );
	FileSystem::makeDir( server.remoteRoot() + "/" + name );
	URI file = server.uri( "testhost", name + "/doc.txt" );
	auto saved = waitFor( fx.vfs.writeAll( file, std::string_view( "v1" ) ) );
	REQUIRE_OK( saved );
	WriteOptions options;
	options.expectedRevision = saved.value().revision;
	// Change size so the second-resolution SFTP revision detects it reliably.
	FileSystem::fileWrite( server.remoteRoot() + "/" + name + "/doc.txt",
						   std::string_view( "changed externally" ) );
	auto conflict = waitFor( fx.vfs.writeAll( file, std::string_view( "v2" ), options ) );
	EXPECT_TRUE( conflict.hasError() );
	if ( conflict )
		return;
	EXPECT_EQ( VFSErrorCode::ChangedExternally, codeOf( conflict ) );
	auto listed = waitFor( fx.vfs.listAll( server.uri( "testhost", name ) ) );
	REQUIRE_OK( listed );
	EXPECT_EQ( 1u, listed.value().size() ); // no temporaries left behind
	EXPECT_STDSTREQ( "changed externally", readString( fx.vfs, file ) );
}

UTEST( SFTP, transfersAcrossProviders ) {
	REQUIRE_SSHD();
	SFTPFixture fx;
	TempDir local;
	FileSystem::makeDir( local.path + "tree/sub", true );
	FileSystem::fileWrite( local.path + "tree/a.txt", std::string_view( "aaa" ) );
	std::string big( 3 * 1024 * 1024 + 17, 'z' );
	FileSystem::fileWrite( local.path + "tree/sub/big.bin", std::string_view( big ) );
	std::string name = uniqueName( "transfer" );
	URI remote = server.uri( "testhost", name );
	auto up = waitFor( fx.vfs.copy( URI::fromFSPath( local.path + "tree" ), remote ).completion() );
	EXPECT_TRUE( up.hasValue() );
	EXPECT_EQ( 2u, up.value().filesTransferred );
	auto down =
		waitFor( fx.vfs.copy( remote, URI::fromFSPath( local.path + "back" ) ).completion() );
	EXPECT_TRUE( down.hasValue() );
	std::string roundTrip;
	FileSystem::fileGet( local.path + "back/sub/big.bin", roundTrip );
	EXPECT_TRUE( roundTrip == big );
	// Same-server move is a rename.
	auto moved =
		waitFor( fx.vfs.move( remote, server.uri( "testhost", name + "-moved" ) ).completion() );
	REQUIRE_OK( moved );
	EXPECT_TRUE( moved.value().renamed );
	EXPECT_TRUE( FileSystem::fileExists( server.remoteRoot() + "/" + name + "-moved/a.txt" ) );
}

UTEST( SFTP, hostKeyPolicies ) {
	REQUIRE_SSHD();
	{
		SFTPFixture fx;
		auto strict = waitFor( fx.vfs.stat( server.uri( "strict" ) ) );
		EXPECT_EQ( VFSErrorCode::HostKeyRejected, codeOf( strict ) );
	}
	{
		// A changed key is refused without even asking.
		auto verifier = std::make_shared<FixedVerifier>( SSHHostKeyDecision::AcceptAndSave );
		SFTPProviderConfig config;
		config.hostKeyVerifier = verifier;
		SFTPFixture fx( config );
		auto mismatch = waitFor( fx.vfs.stat( server.uri( "mismatch" ) ) );
		EXPECT_EQ( VFSErrorCode::HostKeyRejected, codeOf( mismatch ) );
		EXPECT_EQ( 0, verifier->calls.load() );
	}
	{
		auto verifier = std::make_shared<FixedVerifier>( SSHHostKeyDecision::Reject );
		SFTPProviderConfig config;
		config.hostKeyVerifier = verifier;
		SFTPFixture fx( config );
		auto rejected = waitFor( fx.vfs.stat( server.uri( "ask" ) ) );
		EXPECT_EQ( VFSErrorCode::HostKeyRejected, codeOf( rejected ) );
		EXPECT_EQ( 1, verifier->calls.load() );
		EXPECT_TRUE( verifier->lastInfo.fingerprint.rfind( "SHA256:", 0 ) == 0 );
	}
	{
		auto verifier = std::make_shared<FixedVerifier>( SSHHostKeyDecision::AcceptAndSave );
		SFTPProviderConfig config;
		config.hostKeyVerifier = verifier;
		SFTPFixture fx( config );
		EXPECT_TRUE( waitFor( fx.vfs.stat( server.uri( "ask" ) ) ).hasValue() );
		EXPECT_EQ( 1, verifier->calls.load() );
		// Saved: a new provider (fresh connection) does not ask again.
		auto verifier2 = std::make_shared<FixedVerifier>( SSHHostKeyDecision::Reject );
		SFTPProviderConfig config2;
		config2.hostKeyVerifier = verifier2;
		SFTPFixture fx2( config2 );
		EXPECT_TRUE( waitFor( fx2.vfs.stat( server.uri( "ask" ) ) ).hasValue() );
		EXPECT_EQ( 0, verifier2->calls.load() );
	}
}

UTEST( SFTP, authenticationPrompts ) {
	REQUIRE_SSHD();
	{
		auto auth =
			std::make_shared<ScriptedAuthenticator>( std::vector<std::optional<std::string>>{
				std::string( "wrong" ), std::string( "secret" ) } );
		SFTPProviderConfig config;
		config.authenticationHandler = auth;
		SFTPFixture fx( config );
		EXPECT_TRUE( waitFor( fx.vfs.stat( server.uri( "secret" ) ) ).hasValue() );
		EXPECT_EQ( 2u, auth->requests.size() );
		if ( auth->requests.size() < 2 )
			return;
		EXPECT_EQ( SSHAuthenticationRequest::Kind::Passphrase, auth->requests[0].kind );
		EXPECT_TRUE( auth->requests[0].identityFile.find( "id_secret" ) != std::string::npos );
		EXPECT_EQ( 2u, auth->requests[1].attempt );
	}
	{
		// Declined passphrase and wrong passwords: authentication fails cleanly.
		auto auth = std::make_shared<ScriptedAuthenticator>(
			std::vector<std::optional<std::string>>{ std::nullopt } );
		SFTPProviderConfig config;
		config.authenticationHandler = auth;
		SFTPFixture fx( config );
		auto failed = waitFor( fx.vfs.stat( server.uri( "secret" ) ) );
		EXPECT_EQ( VFSErrorCode::AuthenticationFailed, codeOf( failed ) );
	}
	{
		auto auth =
			std::make_shared<ScriptedAuthenticator>( std::vector<std::optional<std::string>>{
				std::string( "nope1" ), std::string( "nope2" ) } );
		SFTPProviderConfig config;
		config.authenticationHandler = auth;
		SFTPFixture fx( config );
		auto failed = waitFor( fx.vfs.stat( server.uri( "pw" ) ) );
		EXPECT_EQ( VFSErrorCode::AuthenticationFailed, codeOf( failed ) );
		// NumberOfPasswordPrompts 2.
		EXPECT_EQ( 2u, auth->requests.size() );
		if ( !auth->requests.empty() )
			EXPECT_EQ( SSHAuthenticationRequest::Kind::Password, auth->requests[0].kind );
	}
}

UTEST( SFTP, ed25519KeysHostKeysAndKex ) {
	REQUIRE_SSHD();
	{
		SFTPFixture fx;
		// Identity file with and without its ".pub" file (the server only accepts
		// curve25519-sha256 key exchange, so every connection also covers it).
		auto withPub = waitFor( fx.vfs.stat( server.uri( "edkey" ) ) );
		REQUIRE_OK( withPub );
		auto noPub = waitFor( fx.vfs.stat( server.uri( "ednopub" ) ) );
		REQUIRE_OK( noPub );
		// Ed25519 host key verified against a known_hosts file that only has that type.
		auto host = waitFor( fx.vfs.stat( server.uri( "edhost" ) ) );
		REQUIRE_OK( host );
	}
	{
		// Encrypted Ed25519 key: a wrong passphrase is asked again.
		auto auth =
			std::make_shared<ScriptedAuthenticator>( std::vector<std::optional<std::string>>{
				std::string( "wrong" ), std::string( "edsecret" ) } );
		SFTPProviderConfig config;
		config.authenticationHandler = auth;
		SFTPFixture fx( config );
		auto stat = waitFor( fx.vfs.stat( server.uri( "edsecret" ) ) );
		REQUIRE_OK( stat );
		EXPECT_EQ( 2u, auth->requests.size() );
		if ( !auth->requests.empty() )
			EXPECT_EQ( SSHAuthenticationRequest::Kind::Passphrase, auth->requests[0].kind );
	}
}

UTEST( SFTP, proxyJumpAndProxyCommand ) {
	REQUIRE_SSHD();
	SFTPFixture fx;
	std::string name = uniqueName( "proxy" );
	FileSystem::makeDir( server.remoteRoot() + "/" + name );
	FileSystem::fileWrite( server.remoteRoot() + "/" + name + "/x.txt",
						   std::string_view( "through the jump" ) );
	EXPECT_STDSTREQ( "through the jump",
					 readString( fx.vfs, server.uri( "jumped", name + "/x.txt" ) ) );
	EXPECT_STDSTREQ( "through the jump",
					 readString( fx.vfs, server.uri( "proxied", name + "/x.txt" ) ) );
}

UTEST( SFTP, connectionReuseReconnectAndErrors ) {
	REQUIRE_SSHD();
	SFTPFixture fx;
	EXPECT_TRUE( waitFor( fx.vfs.stat( server.uri( "testhost" ) ) ).hasValue() );
	EXPECT_TRUE( waitFor( fx.vfs.listAll( server.uri( "testhost" ) ) ).hasValue() );
	EXPECT_EQ( 1u, fx.sftp->connectionCount() );
	fx.sftp->disconnectAll();
	EXPECT_EQ( 0u, fx.sftp->connectionCount() );
	EXPECT_TRUE( waitFor( fx.vfs.stat( server.uri( "testhost" ) ) ).hasValue() );
	EXPECT_EQ( 1u, fx.sftp->connectionCount() );

	auto missing = waitFor( fx.vfs.stat( server.uri( "testhost", "does-not-exist" ) ) );
	EXPECT_EQ( VFSErrorCode::NotFound, codeOf( missing ) );
	auto withPassword = waitFor( fx.vfs.stat( URI( "sftp://user:pw@testhost/x" ) ) );
	EXPECT_EQ( VFSErrorCode::InvalidURI, codeOf( withPassword ) );
	CancellationSource cancel;
	cancel.cancel();
	auto cancelled = waitFor( fx.vfs.stat( server.uri( "testhost" ), {}, cancel.token() ) );
	EXPECT_EQ( VFSErrorCode::Cancelled, codeOf( cancelled ) );
	auto config = waitFor( fx.sftp->resolveConfig( server.uri( "jumped" ) ) );
	REQUIRE_OK( config );
	EXPECT_EQ( 1u, config.value().proxyJump.size() );
}

UTEST( SFTP, remoteWatchThroughAgent ) {
	REQUIRE_SSHD();
	std::string agent = Sys::getProcessPath();
	FileSystem::dirAddSlashAtEnd( agent );
	agent += "../efsw-agent";
	if ( !FileSystem::fileExists( agent ) ) {
		printf( "  efsw-agent not built, remote watch test skipped\n" );
		return;
	}
	SFTPProviderConfig config;
	config.watchAgentCommand = agent + " --stdio";
	SFTPFixture fx( config );
	std::string name = uniqueName( "watch" );
	std::string local = server.remoteRoot() + "/" + name;
	FileSystem::makeDir( local );
	auto opened = waitFor( fx.vfs.watch( server.uri( "testhost", name ), WatchOptions{ true } ) );
	REQUIRE_OK( opened );
	WatchHandle watch( std::move( opened ).value() );
	EXPECT_EQ( WatchStatus::Active, watch.status() );
	TestLoop loop;
	EventCollector events;
	watch.onEvents( loop, events.callback() );
	FileSystem::fileWrite( local + "/remote.txt", std::string_view( "x" ) );
	EXPECT_TRUE(
		loop.runUntil( [&] { return events.has( WatchEventType::Added, "remote.txt" ); } ) );
	FileSystem::makeDir( local + "/sub" );
	std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
	FileSystem::fileWrite( local + "/sub/deep.txt", std::string_view( "x" ) );
	EXPECT_TRUE(
		loop.runUntil( [&] { return events.has( WatchEventType::Added, "sub/deep.txt" ); } ) );

	// Killing the agent: the watch reconnects, re-registers and asks for a rescan. SFTP keeps
	// working meanwhile.
	Process killer;
	killer.create( "pkill", { "-f", agent + " --stdio" } );
	int code;
	killer.join( &code );
	EXPECT_TRUE(
		loop.runUntil( [&] { return events.has( WatchEventType::RescanRequired, "" ); }, 15000 ) );
	EXPECT_TRUE( waitFor( fx.vfs.stat( server.uri( "testhost", name ) ) ).hasValue() );
	EXPECT_TRUE( loop.runUntil( [&] { return watch.status() == WatchStatus::Active; }, 15000 ) );
	FileSystem::fileWrite( local + "/after.txt", std::string_view( "x" ) );
	EXPECT_TRUE(
		loop.runUntil( [&] { return events.has( WatchEventType::Added, "after.txt" ); } ) );
}

UTEST( SFTP, remoteWatchFallsBackToPolling ) {
	REQUIRE_SSHD();
	SFTPProviderConfig config;
	config.watchAgentCommand = "efsw-agent-that-does-not-exist --stdio";
	SFTPFixture fx( config );
	std::string name = uniqueName( "pollwatch" );
	std::string local = server.remoteRoot() + "/" + name;
	FileSystem::makeDir( local );
	WatchOptions options;
	options.pollingInterval = System::Milliseconds( 100 );
	auto opened = waitFor( fx.vfs.watch( server.uri( "testhost", name ), options ) );
	REQUIRE_OK( opened );
	WatchHandle watch( std::move( opened ).value() );
	EXPECT_EQ( WatchStatus::Polling, watch.status() );
	TestLoop loop;
	EventCollector events;
	watch.onEvents( loop, events.callback() );
	FileSystem::fileWrite( local + "/polled.txt", std::string_view( "x" ) );
	EXPECT_TRUE(
		loop.runUntil( [&] { return events.has( WatchEventType::Added, "polled.txt" ); } ) );
}

#endif
