#include <algorithm>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cstring>
#include <eepp/core/small_vector.hpp>
#include <eepp/network/http.hpp>
#include <eepp/network/http/httpstreamchunked.hpp>
#include <eepp/network/socketselector.hpp>
#include <eepp/network/ssl/sslsocket.hpp>
#include <eepp/network/uri.hpp>
#include <eepp/system/compression.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostream.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreaminflate.hpp>
#include <eepp/system/iostreamstring.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/sys.hpp>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
#include <emscripten.h>
#endif

using namespace EE::Network::SSL;
using namespace EE::Network::Private;

namespace EE { namespace Network {

#define PACKET_BUFFER_SIZE ( 16384 )

static std::string sDefaultUserAgent = "Mozilla/5.0 (Linux; x86_64) eepp-network/1.0 "
									   "Chrome/146.0.0.0 AppleWebKit/537.36 Safari/537.36";

namespace {

using SteadyClock = std::chrono::steady_clock;

// These limits bound metadata, independently of the size of downloaded resource bodies.
constexpr std::size_t MaxHTTPHeaderBytes = 64 * 1024;
constexpr unsigned MaxInformationalResponses = 32;

bool isRedirectStatus( Http::Response::Status status ) {
	return status == Http::Response::MovedPermanently ||
		   status == Http::Response::MovedTemporarily || status == Http::Response::SeeOther ||
		   status == Http::Response::TemporaryRedirect ||
		   status == Http::Response::PermanentRedirect;
}

std::string originKey( const URI& uri ) {
	return String::toLower( uri.getScheme() ) + "://" + String::toLower( uri.getHost() ) + ":" +
		   String::toString( uri.getPort() );
}

std::string transportKey( const URI& uri, const URI& proxy ) {
	return originKey( uri ) + "\n" +
		   ( proxy.empty() ? "" : originKey( proxy ) + "\n" + proxy.getUserInfo() );
}

class DiscardStream : public IOStreamString {
  public:
	ios_size write( const char*, ios_size size ) { return size; }
};

bool hasToken( std::string_view value, std::string_view token ) {
	while ( !value.empty() ) {
		const auto comma = value.find( ',' );
		const auto part = String::trim( value.substr( 0, comma ) );
		if ( part.size() == token.size() &&
			 std::equal( part.begin(), part.end(), token.begin(),
						 []( unsigned char a, char b ) { return std::tolower( a ) == b; } ) )
			return true;
		if ( comma == std::string_view::npos )
			break;
		value.remove_prefix( comma + 1 );
	}
	return false;
}

bool parseContentLength( std::string_view value, Uint64& length ) {
	bool first = true;
	do {
		const auto comma = value.find( ',' );
		const auto part = String::trim( value.substr( 0, comma ) );
		Uint64 parsed = 0;
		auto result = std::from_chars( part.data(), part.data() + part.size(), parsed );
		if ( part.empty() || result.ec != std::errc() || result.ptr != part.data() + part.size() ||
			 ( !first && parsed != length ) )
			return false;
		length = parsed;
		first = false;
		if ( comma == std::string_view::npos )
			return true;
		value.remove_prefix( comma + 1 );
	} while ( !value.empty() );
	return false;
}

// Servers close idle connections whenever they choose (Apache's default keep-alive timeout is 5 s),
// so by the time a connection is reused its FIN, RST, or stray bytes are usually already readable.
// A nonblocking read finds them without waiting. Any data at all is invalid: no request is in
// flight on an idle connection. For TLS this also consumes post-handshake records such as session
// tickets, which leave a healthy connection reporting NotReady.
bool isIdleConnectionUsable( TcpSocket& socket ) {
	socket.setBlocking( false );
	char byte;
	std::size_t count = 0;
	const auto status = socket.receive( &byte, sizeof( byte ), count );
	return status == Socket::NotReady || ( status == Socket::Done && count == 0 );
}

} // namespace

struct Http::ConnectionPool {
	struct Connection {
		std::unique_ptr<TcpSocket> socket;
		SocketSelector selector;
		std::string key;
		SteadyClock::time_point lastUsed;
		bool busy{ true };
		bool validateCertificate{ true };
		bool validateHostname{ true };

		~Connection() {
			// In particular, an OpenSSL close_notify must not block browser session teardown.
			if ( socket )
				socket->setBlocking( false );
		}
	};

	struct Lease {
		ConnectionPool& pool;
		Connection* connection{ nullptr };
		bool reused{ false };
		bool reusable{ false };

		explicit Lease( ConnectionPool& pool ) : pool( pool ) {}

		~Lease() { release(); }

		void release() {
			if ( !connection )
				return;
			std::unique_ptr<Connection> discarded;
			{
				std::lock_guard<std::mutex> lock( pool.mutex );
				if ( reusable && !pool.closed.load( std::memory_order_acquire ) ) {
					connection->busy = false;
					connection->lastUsed = SteadyClock::now();
				} else {
					auto it = std::find_if(
						pool.connections.begin(), pool.connections.end(),
						[this]( const auto& item ) { return item.get() == connection; } );
					if ( it != pool.connections.end() ) {
						discarded = std::move( *it );
						pool.connections.erase( it );
					}
				}
				connection = nullptr;
			}
			// TLS destruction and callbacks must never execute under the pool mutex.
			pool.condition.notify_all();
		}
	};

	explicit ConnectionPool( ConnectionPoolOptions options = {} ) : options( options ) {
		this->options.maxConnections = std::max<std::size_t>( 1, options.maxConnections );
		this->options.maxConnectionsPerHost =
			std::max<std::size_t>( 1, options.maxConnectionsPerHost );
	}

	bool acquire( Lease& lease, const Http& http, const Request& request,
				  SteadyClock::time_point started, Time timeout ) {
		std::unique_lock<std::mutex> lock( mutex );
		while ( !closed.load( std::memory_order_acquire ) && !request.isCancelled() &&
				!http.mShuttingDown.load( std::memory_order_acquire ) ) {
			const auto now = SteadyClock::now();
			if ( timeout > Time::Zero &&
				 now - started >= std::chrono::microseconds( timeout.asMicroseconds() ) )
				return false;
			// Expiry and capacity eviction remove only idle sockets; active leases stay stable.
			for ( auto it = connections.begin(); it != connections.end(); ) {
				if ( !( *it )->busy &&
					 now - ( *it )->lastUsed >=
						 std::chrono::microseconds( options.idleTimeout.asMicroseconds() ) ) {
					auto discarded = std::move( *it );
					it = connections.erase( it );
					lock.unlock();
					discarded.reset();
					lock.lock();
					// Another acquirer may have changed the vector while it was unlocked.
					it = connections.begin();
				} else {
					++it;
				}
			}
			std::size_t hostCount = 0;
			Connection* oldest = nullptr;
			for ( auto& connection : connections ) {
				const bool sameHost = connection->key == http.mConnectionKey;
				if ( sameHost )
					++hostCount;
				if ( !connection->busy && sameHost &&
					 connection->validateCertificate == request.getValidateCertificate() &&
					 connection->validateHostname == request.getValidateHostname() ) {
					connection->busy = true;
					lease.connection = connection.get();
					lease.reused = true;
					return true;
				}
			}
			for ( auto& connection : connections ) {
				if ( !connection->busy &&
					 ( hostCount < options.maxConnectionsPerHost ||
					   connection->key == http.mConnectionKey ) &&
					 ( !oldest || connection->lastUsed < oldest->lastUsed ) )
					oldest = connection.get();
			}
			if ( oldest && ( connections.size() >= options.maxConnections ||
							 hostCount >= options.maxConnectionsPerHost ) ) {
				auto it =
					std::find_if( connections.begin(), connections.end(),
								  [oldest]( const auto& item ) { return item.get() == oldest; } );
				auto discarded = std::move( *it );
				connections.erase( it );
				lock.unlock();
				discarded.reset();
				lock.lock();
				continue;
			}
			if ( connections.size() < options.maxConnections &&
				 hostCount < options.maxConnectionsPerHost ) {
				auto connection = std::make_unique<Connection>();
				connection->key = http.mConnectionKey;
				connection->validateCertificate = request.getValidateCertificate();
				connection->validateHostname = request.getValidateHostname();
				lease.connection = connection.get();
				connections.emplace_back( std::move( connection ) );
				return true;
			}
			// Timed waits also observe per-request cancellation without running user callbacks
			// locked.
			condition.wait_for( lock, std::chrono::milliseconds( 10 ) );
		}
		return false;
	}

	void closeIdle( bool closePool = false ) {
		SmallVector<std::unique_ptr<Connection>, 8> discarded;
		{
			std::lock_guard<std::mutex> lock( mutex );
			if ( closePool )
				closed.store( true, std::memory_order_release );
			for ( auto it = connections.begin(); it != connections.end(); ) {
				if ( !( *it )->busy ) {
					discarded.emplace_back( std::move( *it ) );
					it = connections.erase( it );
				} else {
					++it;
				}
			}
		}
		condition.notify_all();
	}

	std::mutex mutex;
	std::condition_variable condition;
	SmallVector<std::unique_ptr<Connection>, 8> connections;
	ConnectionPoolOptions options;
	std::atomic<bool> closed{ false };
};

void Http::setDefaultUserAgent( const std::string& userAgent ) {
	sDefaultUserAgent = userAgent;
}

std::string Http::getDefaultUserAgent() {
	return sDefaultUserAgent;
}

std::string Http::Request::statusToString( Http::Request::Status status ) {
	switch ( status ) {
		case Connected:
			return "Connected";
		case Sent:
			return "Sent";
		case HeaderReceived:
			return "HeaderReceived";
		case ContentReceived:
			return "ContentReceived";
		case Redirect:
			return "Redirect";
	}
	return "";
}

Http::Request::Method Http::Request::methodFromString( std::string methodString ) {
	String::toLowerInPlace( methodString );
	if ( "get" == methodString )
		return Method::Get;
	else if ( "head" == methodString )
		return Method::Head;
	else if ( "post" == methodString )
		return Method::Post;
	else if ( "put" == methodString )
		return Method::Put;
	else if ( "delete" == methodString )
		return Method::Delete;
	else if ( "options" == methodString )
		return Method::Options;
	else if ( "patch" == methodString )
		return Method::Patch;
	else if ( "connect" == methodString )
		return Method::Connect;
	else
		return Method::Get;
}

std::string Http::Request::methodToString( const Http::Request::Method& method ) {
	switch ( method ) {
		default:
		case Get:
			return "GET";
		case Head:
			return "HEAD";
		case Post:
			return "POST";
		case Put:
			return "PUT";
		case Delete:
			return "DELETE";
		case Options:
			return "OPTIONS";
		case Patch:
			return "PATCH";
		case Connect:
			return "CONNECT";
	}
}

Http::Request::Request( const std::string& uri, Method method, const std::string& body,
						bool validateCertificate, bool validateHostname, bool followRedirect,
						bool compressedResponse ) :
	mValidateCertificate( validateCertificate ),
	mValidateHostname( validateHostname ),
	mFollowRedirect( followRedirect ),
	mCompressedResponse( compressedResponse ),
	mContinue( false ),
	mCancel( std::make_shared<std::atomic<bool>>( false ) ),
	mMaxRedirections( 10 ),
	mRedirectionCount( 0 ) {
	setMethod( method );
	setUri( uri );
	setHttpVersion( 1, 1 );
	setBody( body );
}

void Http::Request::setField( const std::string& field, const std::string& value ) {
	mFields[String::toLower( field )] = value;
}

void Http::Request::setHeader( const std::string& field, const std::string& value ) {
	setField( field, value );
}

void Http::Request::setMethod( Http::Request::Method method ) {
	mMethod = method;
}

void Http::Request::setUri( const std::string& uri ) {
	mUri = uri;

	// Make sure it starts with a '/'
	if ( mUri.empty() || ( mUri[0] != '/' ) )
		mUri.insert( 0, "/" );
}

void Http::Request::setHttpVersion( unsigned int major, unsigned int minor ) {
	mMajorVersion = major;
	mMinorVersion = minor;
}

void Http::Request::setBody( const std::string& body ) {
	mBody = body;
}

void Http::Request::setBody( std::string&& body ) {
	mBody = std::move( body );
}

void Http::Request::appendBody( const std::string& body ) {
	mBody.append( body );
}

void Http::Request::appendBody( std::string&& body ) {
	mBody.append( std::move( body ) );
}

const std::string& Http::Request::getUri() const {
	return mUri;
}

const Http::Request::Method& Http::Request::getMethod() const {
	return mMethod;
}

const bool& Http::Request::getValidateCertificate() const {
	return mValidateCertificate;
}

void Http::Request::setValidateCertificate( bool enable ) {
	mValidateCertificate = enable;
}

const bool& Http::Request::getValidateHostname() const {
	return mValidateHostname;
}

void Http::Request::setValidateHostname( bool enable ) {
	mValidateHostname = enable;
}

const bool& Http::Request::getFollowRedirect() const {
	return mFollowRedirect;
}

void Http::Request::setFollowRedirect( bool follow ) {
	mFollowRedirect = follow;
}

const unsigned int& Http::Request::getMaxRedirects() const {
	return mMaxRedirections;
}

void Http::Request::setMaxRedirects( unsigned int maxRedirects ) {
	mMaxRedirections = maxRedirects;
}

void Http::Request::setProgressCallback( const Http::Request::ProgressCallback& progressCallback ) {
	mProgressCallback = progressCallback;
}

void Http::Request::setCancelCallback( const Http::Request::CancelCallback& cancelCallback ) {
	mCancelCallback = cancelCallback;
}

const Http::Request::ProgressCallback& Http::Request::getProgressCallback() const {
	return mProgressCallback;
}

const Http::Request::CancelCallback& Http::Request::getCancelCallback() const {
	return mCancelCallback;
}

void Http::Request::cancel( bool resetCancelCallback ) {
	mCancel->store( true, std::memory_order_release );
	if ( resetCancelCallback ) {
		setProgressCallback( {} );
		setCancelCallback( {} );
	}
}

bool Http::Request::isCancelled() const {
	return mCancel->load( std::memory_order_acquire );
}

std::string Http::Request::prepareTunnel( const Http& http ) {
	std::ostringstream out;

	setMethod( Connect );

	std::string method = methodToString( mMethod );

	out << method << " " << http.getHostName() << ":" << http.getPort() << " ";
	out << "HTTP/" << mMajorVersion << "." << mMinorVersion << "\r\n";

	setField( "Host", String::format( "%s:%d", http.getHostName().c_str(), http.getPort() ) );
	setField( "Proxy-Connection", "Keep-Alive" );
	setField( "User-Agent", sDefaultUserAgent );

	for ( FieldTable::const_iterator i = mFields.begin(); i != mFields.end(); ++i )
		out << i->first << ": " << i->second << "\r\n";

	out << "\r\n";

	return out.str();
}

void Http::onCancel( const Http::Request& request ) {
	if ( request.getCancelCallback() )
		request.getCancelCallback()( *this, request );
}

bool Http::Request::isVerbose() const {
	return mVerbose;
}

void Http::Request::setVerbose( bool verbose ) {
	mVerbose = verbose;
}

void Http::Request::setContinue( const bool& resume ) {
	mContinue = resume;
}

const bool& Http::Request::isContinue() const {
	return mContinue;
}

const bool& Http::Request::isCompressedResponse() const {
	return mCompressedResponse;
}

void Http::Request::setCompressedResponse( const bool& compressedResponse ) {
	mCompressedResponse = compressedResponse;
}

std::string Http::Request::prepare( const Http& http ) const {
	std::ostringstream out;

	// Convert the method to its string representation
	std::string method = methodToString( mMethod );

	// Write the first line containing the request type
	if ( http.getProxy().empty() || http.isSSL() ) {
		out << method << " " << mUri << " ";
	} else {
		URI uri = http.getURI();
		uri.setPathEtc( mUri );
		out << method << " " << uri.toString() << " ";
	}

	out << "HTTP/" << mMajorVersion << "." << mMinorVersion << "\r\n";

	// Write fields
	for ( FieldTable::const_iterator i = mFields.begin(); i != mFields.end(); ++i ) {
		out << i->first << ": " << i->second << "\r\n";
	}

	// Use an extra \r\n to separate the header from the body
	out << "\r\n";

	// Add the body
	out << mBody;

	return out.str();
}

bool Http::Request::hasField( const std::string& field ) const {
	return mFields.find( String::toLower( field ) ) != mFields.end();
}

const std::string& Http::Request::getField( const std::string& field ) const {
	FieldTable::const_iterator it = mFields.find( String::toLower( field ) );
	if ( it != mFields.end() ) {
		return it->second;
	} else {
		static const std::string empty = "";
		return empty;
	}
}

URI Http::getEnvProxyURI() {
	const char* http_proxy = getenv( "http_proxy" );
	URI proxy;

	if ( NULL == http_proxy )
		http_proxy = getenv( "HTTP_PROXY" );

	if ( NULL != http_proxy ) {
		std::string httpProxy;
		httpProxy = std::string( http_proxy );
		if ( !httpProxy.empty() && httpProxy.find( "://" ) == std::string::npos )
			httpProxy = "http://" + httpProxy;
		proxy = URI( httpProxy );
	}
	return proxy;
}

const char* Http::Response::statusToString( const Http::Response::Status& status ) {
	switch ( status ) {
		// 2xx: success
		case Ok:
			return "OK";
		case Created:
			return "Created";
		case Accepted:
			return "Accepted";
		case NoContent:
			return "No Content";
		case ResetContent:
			return "Reset Content";
		case PartialContent:
			return "Partial Content";

		// 3xx: redirection
		case MultipleChoices:
			return "Multiple Choices";
		case MovedPermanently:
			return "Moved Permanently";
		case MovedTemporarily:
			return "Moved Temporarily";
		case NotModified:
			return "Not Modified";
		case TemporaryRedirect:
			return "Temporary Redirect";
		case PermanentRedirect:
			return "Permanent Redirect";

		// 4xx: client error
		case BadRequest:
			return "BadRequest";
		case Unauthorized:
			return "Unauthorized";
		case Forbidden:
			return "Forbidden";
		case NotFound:
			return "Not Found";
		case RangeNotSatisfiable:
			return "Range Not Satisfiable";

		// 5xx: server error
		case InternalServerError:
			return "Internal Server Error";
		case NotImplemented:
			return "Not Implemented";
		case BadGateway:
			return "Bad Gateway";
		case ServiceNotAvailable:
			return "Service Not Available";
		case GatewayTimeout:
			return "Gateway Timeout";
		case VersionNotSupported:
			return "Version Not Supported";

		// 10xx: Custom codes
		case InvalidResponse:
			return "Invalid Response";
		case ConnectionFailed:
			return "Connection Failed";
		default:
			return "";
	}
}

Http::Response::Status Http::Response::intAsStatus( const int& value ) {
	switch ( value ) {
		case Ok:
		case Created:
		case Accepted:
		case NoContent:
		case ResetContent:
		case PartialContent:
		case MultipleChoices:
		case MovedPermanently:
		case MovedTemporarily:
		case TemporaryRedirect:
		case PermanentRedirect:
		case NotModified:
		case BadRequest:
		case Unauthorized:
		case Forbidden:
		case NotFound:
		case RangeNotSatisfiable:
		case InternalServerError:
		case NotImplemented:
		case BadGateway:
		case ServiceNotAvailable:
		case GatewayTimeout:
		case VersionNotSupported:
		case InvalidResponse:
		case ConnectionFailed:
			return (Status)value;
		default:
			return InternalServerError;
	}
}

Http::Response Http::Response::createFakeResponse( const Http::Response::FieldTable& fields,
												   Http::Response::Status& status,
												   const std::string& body,
												   unsigned int majorVersion,
												   unsigned int minorVersion ) {
	Response response;
	response.mStatus = status;
	response.mBody = body;
	response.mFields = fields;
	response.mMajorVersion = majorVersion;
	response.mMinorVersion = minorVersion;
	return response;
}

Http::Response::Response() : mStatus( ConnectionFailed ), mMajorVersion( 0 ), mMinorVersion( 0 ) {}

const Http::Response::FieldTable& Http::Response::getHeaders() const {
	return mFields;
}

Http::Response::FieldTable Http::Response::getHeaders() {
	return mFields;
}

const std::string& Http::Response::getField( const std::string& field ) const {
	FieldTable::const_iterator it = mFields.find( String::toLower( field ) );
	if ( it != mFields.end() ) {
		return it->second;
	} else {
		static const std::string empty = "";
		return empty;
	}
}

bool Http::Response::hasField( const std::string& field ) const {
	return mFields.find( String::toLower( field ) ) != mFields.end();
}

Http::Response::Status Http::Response::getStatus() const {
	return mStatus;
}

bool Http::Response::isOK() const {
	return mStatus >= 200 && mStatus < 300;
}

const char* Http::Response::getStatusDescription() const {
	switch ( mStatus ) {
		// 2xx: success
		case Ok:
			return "Successful";
		case Created:
			return "The resource has successfully been created";
		case Accepted:
			return "The request has been accepted, but will be processed later by the server";
		case NoContent:
			return "The server didn't send any data in return";
		case ResetContent:
			return "The server informs the client that it should clear the view (form) that caused "
				   "the request to be sent";
		case PartialContent:
			return "The server has sent a part of the resource, as a response to a partial GET "
				   "request";

		// 3xx: redirection
		case MultipleChoices:
			return "The requested page can be accessed from several locations";
		case MovedPermanently:
		case PermanentRedirect:
			return "The requested page has permanently moved to a new location";
		case MovedTemporarily:
		case TemporaryRedirect:
			return "The requested page has temporarily moved to a new location";
		case NotModified:
			return "For conditional requests, means the requested page hasn't changed and doesn't "
				   "need to be refreshed";

		// 4xx: client error
		case BadRequest:
			return "The server couldn't understand the request (syntax error)";
		case Unauthorized:
			return "The requested page needs an authentication to be accessed";
		case Forbidden:
			return "The requested page cannot be accessed at all, even with authentication";
		case NotFound:
			return "The requested page doesn't exist";
		case RangeNotSatisfiable:
			return "The server can't satisfy the partial GET request (with a \"Range\" header "
				   "field)";

		// 5xx: server error
		case InternalServerError:
			return "The server encountered an unexpected error";
		case NotImplemented:
			return "The server doesn't implement a requested feature";
		case BadGateway:
			return "The gateway server has received an error from the source server";
		case ServiceNotAvailable:
			return "The server is temporarily unavailable (overloaded, in maintenance, ...)";
		case GatewayTimeout:
			return "The gateway server couldn't receive a response from the source server";
		case VersionNotSupported:
			return "The server doesn't support the requested HTTP version";

		// 10xx: Custom codes
		case InvalidResponse:
			return "Response is not a valid HTTP one";
		case ConnectionFailed:
			return "Connection with server failed";
		default:
			return "Unknown response status";
	}
}

unsigned int Http::Response::getMajorHttpVersion() const {
	return mMajorVersion;
}

unsigned int Http::Response::getMinorHttpVersion() const {
	return mMinorVersion;
}

const std::string& Http::Response::getBody() const {
	return mBody;
}

void Http::Response::parse( const std::string& data ) {
	mFields.clear();
	mBody.clear();
	mStatus = InvalidResponse;
	mMajorVersion = mMinorVersion = 0;
	std::istringstream in( data );
	std::string version;
	int status = 0;
	if ( !( in >> version >> status ) || version.size() != 8 || version.substr( 0, 5 ) != "HTTP/" ||
		 version[5] != '1' || version[6] != '.' || ( version[7] != '0' && version[7] != '1' ) ||
		 status < 100 || status > 599 )
		return;
	mMajorVersion = 1;
	mMinorVersion = version[7] - '0';
	in.ignore( std::numeric_limits<std::streamsize>::max(), '\n' );
	if ( parseFields( in ) )
		mStatus = static_cast<Status>( status );
}

bool Http::Response::parseFields( std::istream& in ) {
	std::string line;
	while ( std::getline( in, line ) ) {
		if ( !line.empty() && line.back() == '\r' )
			line.pop_back();
		if ( line.empty() )
			return true;
		const auto colon = line.find( ':' );
		if ( colon == std::string::npos || colon == 0 )
			return false;
		for ( size_t i = 0; i < colon; ++i ) {
			const unsigned char ch = line[i];
			if ( !std::isalnum( ch ) &&
				 std::string_view( "!#$%&'*+-.^_`|~" ).find( ch ) == std::string_view::npos )
				return false;
		}
		auto value = String::trim( std::string_view( line ).substr( colon + 1 ) );
		for ( const unsigned char ch : value ) {
			if ( ( ch < 32 && ch != '\t' ) || ch == 127 )
				return false;
		}
		std::string field = String::toLower( line.substr( 0, colon ) );
		auto it = mFields.find( field );
		if ( it != mFields.end() ) {
			// RFC 9110 5.3: repeated fields combine into one comma-separated list, except
			// Set-Cookie, whose values may themselves contain commas (Expires dates). Keep each
			// Set-Cookie on its own line; field values cannot contain line breaks.
			it->second += field == "set-cookie" ? '\n' : ',';
			it->second.append( value );
		} else {
			mFields[std::move( field )] = std::string( value );
		}
	}
	return true;
}

static Http::Pool sGlobalHttpPool = Http::Pool();

static std::shared_ptr<ThreadPool> sGlobalThreadPool = nullptr;
static std::mutex sGlobalThreadPoolMutex;

static std::shared_ptr<ThreadPool> getGlobalThreadPool() {
	std::lock_guard<std::mutex> lock( sGlobalThreadPoolMutex );
	return sGlobalThreadPool;
}

Http::Response Http::request( const URI& uri, Request::Method method, const Time& timeout,
							  const Http::Request::ProgressCallback& progressCallback,
							  const Http::Request::FieldTable& headers, const std::string& body,
							  const bool& validateCertificate, const URI& proxy ) {
	auto http = sGlobalHttpPool.get( uri, proxy );
	Request request( uri.getPathAndQuery(), method, body, validateCertificate, validateCertificate,
					 true, true );
	request.setProgressCallback( progressCallback );

	for ( const auto& field : headers )
		request.setField( field.first, field.second );

	return http->sendRequest( request, timeout );
}

Http::Response Http::get( const URI& uri, const Time& timeout,
						  const Http::Request::ProgressCallback& progressCallback,
						  const Http::Request::FieldTable& headers, const std::string& body,
						  const bool& validateCertificate, const URI& proxy ) {
	return request( uri, Request::Method::Get, timeout, progressCallback, headers, body,
					validateCertificate, proxy );
}

Http::Response Http::post( const URI& uri, const Time& timeout,
						   const Http::Request::ProgressCallback& progressCallback,
						   const Http::Request::FieldTable& headers, const std::string& body,
						   const bool& validateCertificate, const URI& proxy ) {
	return request( uri, Request::Method::Post, timeout, progressCallback, headers, body,
					validateCertificate, proxy );
}

Uint64 Http::requestAsync( const Http::AsyncResponseCallback& cb, const URI& uri,
						   const Time& timeout, Request::Method method,
						   const Http::Request::ProgressCallback& progressCallback,
						   const Http::Request::FieldTable& headers, const std::string& body,
						   const bool& validateCertificate, const URI& proxy,
						   bool followRedirect ) {
	if ( Log::existsSingleton() && Log::instance()->getLogLevelThreshold() == LogLevel::Debug ) {
		Log::debug( "Http::requestAsync: %s \"%s\"", Request::methodToString( method ),
					uri.toString() );
	}

	auto http = sGlobalHttpPool.get( uri, proxy );
	Request request( uri.getPathAndQuery(), method, body, validateCertificate, validateCertificate,
					 true, true );
	request.setProgressCallback( progressCallback );
	request.setFollowRedirect( followRedirect );

	for ( const auto& field : headers )
		request.setField( field.first, field.second );

	return http->sendAsyncRequest( cb, request, timeout );
}

Uint64 Http::getAsync( const Http::AsyncResponseCallback& cb, const URI& uri, const Time& timeout,
					   const Http::Request::ProgressCallback& progressCallback,
					   const Http::Request::FieldTable& headers, const std::string& body,
					   const bool& validateCertificate, const URI& proxy, bool followRedirect ) {
	return requestAsync( cb, uri, timeout, Request::Method::Get, progressCallback, headers, body,
						 validateCertificate, proxy, followRedirect );
}

Uint64 Http::postAsync( const Http::AsyncResponseCallback& cb, const URI& uri, const Time& timeout,
						const Http::Request::ProgressCallback& progressCallback,
						const Http::Request::FieldTable& headers, const std::string& body,
						const bool& validateCertificate, const URI& proxy, bool followRedirect ) {
	return requestAsync( cb, uri, timeout, Request::Method::Post, progressCallback, headers, body,
						 validateCertificate, proxy, followRedirect );
}

Http::Http() : Http( "" ) {}

Http::Http( const std::string& host, unsigned short port, bool useSSL, URI proxy ) :
	mConnections( std::make_shared<ConnectionPool>() ), mPort( 0 ), mIsSSL( false ) {
	setHost( host, port, useSSL, std::move( proxy ) );
}

Http::Http( std::shared_ptr<ConnectionPool> connections, const URI& host, const URI& proxy ) :
	mConnections( std::move( connections ) ), mPort( 0 ), mIsSSL( false ) {
	setHost( host, proxy );
}

Http::~Http() {
	shutdown();
}

void Http::setHost( const std::string& host, unsigned short port, bool useSSL, URI proxy ) {
	std::string name = host;
	if ( String::toLower( name.substr( 0, 8 ) ) == "https://" ) {
		name.erase( 0, 8 );
		useSSL = true;
	} else if ( String::toLower( name.substr( 0, 7 ) ) == "http://" ) {
		name.erase( 0, 7 );
		useSSL = false;
	}
	if ( !name.empty() && name.back() == '/' )
		name.pop_back();
	mHostName = String::toLower( name );
	mIsSSL = useSSL;
	mPort = port ? port : ( useSSL ? 443 : 80 );
	mProxy = std::move( proxy );
	mHost = IpAddress();
	mHostSolved = false;
	mConnectionKey = transportKey( getURI(), mProxy );
}

void Http::setHost( const URI& uri, URI proxy ) {
	setHost( uri.getHost(), uri.getPort(), uri.getScheme() == "https", std::move( proxy ) );
}

Http::Response Http::sendRequest( const Http::Request& request, Time timeout ) {
	IOStreamString stream;
	Response response = downloadRequest( request, stream, timeout );
	response.mBody = std::move( stream.getStream() );
	return response;
}

static bool sendProgress( const Http& http, const Http::Request& request,
						  const Http::Response& response, const Http::Request::Status& status,
						  const std::size_t& totalBytes, const std::size_t& currentBytes ) {
	if ( request.isCancelled() )
		return false;
	if ( request.getProgressCallback() )
		return request.getProgressCallback()( http, request, response, status, totalBytes,
											  currentBytes );
	return true;
}

Http::Response Http::downloadRequest( const Http::Request& request, IOStream& writeTo,
									  Time timeout ) {
	const auto started = SteadyClock::now();
	auto remaining = [&]() {
		if ( timeout <= Time::Zero )
			return Time::Zero;
		const auto elapsed =
			std::chrono::duration_cast<std::chrono::microseconds>( SteadyClock::now() - started )
				.count();
		return Microseconds( std::max<Int64>( 1, timeout.asMicroseconds() - elapsed ) );
	};
	// Ordinary requests need only the prepared wire-header copy below. Own another Request
	// only when resuming or redirecting actually changes it.
	std::optional<Request> modifiedRequest;
	const Request* currentRequest = &request;
	if ( request.isContinue() && writeTo.getSize() > 0 ) {
		Request head( request );
		head.setContinue( false );
		head.setMethod( Request::Head );
		IOStreamString headBody;
		auto response = downloadRequest( head, headBody, remaining() );
		Uint64 length = 0;
		const auto offset = writeTo.getSize();
		modifiedRequest.emplace( request );
		currentRequest = &*modifiedRequest;
		if ( response.isOK() && hasToken( response.getField( "accept-ranges" ), "bytes" ) &&
			 parseContentLength( response.getField( "content-length" ), length ) &&
			 static_cast<Uint64>( offset ) < length ) {
			writeTo.seek( offset );
			modifiedRequest->setField( "Range", "bytes=" + String::toString( offset ) + "-" );
		}
		modifiedRequest->setContinue( false );
	}

	Http* target = this;
	std::unique_ptr<Http> redirected;
	for ( unsigned redirects = 0;; ++redirects ) {
		const Request& current = *currentRequest;
		auto connections = target->mConnections;
		auto cancelled = [&]() {
			if ( connections->closed.load( std::memory_order_acquire ) ||
				 target->mShuttingDown.load( std::memory_order_acquire ) )
				current.mCancel->store( true, std::memory_order_release );
			return current.isCancelled();
		};
		auto expired = [&]() {
			return timeout > Time::Zero &&
				   SteadyClock::now() - started >=
					   std::chrono::microseconds( timeout.asMicroseconds() );
		};
		auto progress = [&]( const Response& response, Request::Status status, Uint64 total,
							 Uint64 count ) {
			if ( !sendProgress( *target, current, response, status, total, count ) )
				current.mCancel->store( true, std::memory_order_release );
			return !cancelled();
		};

		// Idle connections the server already closed are discarded before use, for every method.
		auto acquireUsable = [&]( ConnectionPool::Lease& lease ) {
			while ( connections->acquire( lease, *target, current, started, timeout ) ) {
				if ( !lease.reused || isIdleConnectionUsable( *lease.connection->socket ) )
					return true;
				lease.release(); // Not reusable, so the dead connection is discarded.
				lease.reused = false;
			}
			return false;
		};

		Response response;
		// The server can still close a connection between that check and our request. Retry once,
		// only for safe requests and only before any response bytes have arrived; never replay a
		// form submission.
		for ( unsigned attempt = 0; attempt < 2; ++attempt ) {
			ConnectionPool::Lease lease( *connections );
			if ( cancelled() || expired() || !acquireUsable( lease ) )
				break;
			bool responseBytes = false;
			bool retryable = false;
			auto perform = [&]() -> Response {
				Response received;
				auto invalid = [&]() {
					received.mStatus = Response::InvalidResponse;
					return received;
				};
				auto& connection = *lease.connection;
				const bool tunnel = target->isProxied() && target->mIsSSL;
				if ( !connection.socket ) {
					if ( tunnel && target->mProxy.getScheme() == "https" )
						return received; // Nested TLS to an HTTPS CONNECT proxy is unsupported.
					const bool ssl = target->mIsSSL || ( target->isProxied() &&
														 target->mProxy.getScheme() == "https" );
					if ( ssl && !SSLSocket::isSupported() )
						return received;
					connection.socket.reset( ssl ? SSLSocket::New( tunnel || !target->isProxied()
																	   ? target->mHostName
																	   : target->mProxy.getHost(),
																   current.getValidateCertificate(),
																   current.getValidateHostname() )
												 : TcpSocket::New() );
					IpAddress address;
					{
						std::lock_guard<std::mutex> lock( target->mHostMutex );
						if ( !target->mHostSolved ) {
							target->mHost =
								IpAddress( target->isProxied() ? target->mProxy.getHost()
															   : target->mHostName );
							target->mHostSolved = target->mHost.toInteger() != 0;
						}
						address = target->mHost;
					}
					if ( expired() || cancelled() || address.toInteger() == 0 )
						return received;
					const auto port =
						target->isProxied() ? target->mProxy.getPort() : target->mPort;
					// Connect TCP first: timeout options have no effect before a socket handle
					// exists.
					if ( connection.socket->TcpSocket::connect( address, port, remaining() ) !=
						 Socket::Done ) {
						std::lock_guard<std::mutex> lock( target->mHostMutex );
						target->mHostSolved = false;
						return received;
					}
					connection.socket->setSendTimeout( remaining() );
					connection.selector.add( *connection.socket );
					if ( tunnel ) {
						auto* socket = static_cast<SSLSocket*>( connection.socket.get() );
						Request connect;
						if ( current.hasField( "Proxy-Authorization" ) ) {
							connect.setField( "Proxy-Authorization",
											  current.getField( "Proxy-Authorization" ) );
						}
						const std::string wire = connect.prepareTunnel( *target );
						std::size_t sent = 0;
						if ( socket->tcpSend( wire.data(), wire.size(), sent ) != Socket::Done )
							return received;
						socket->setBlocking( false );
						std::string header;
						char buffer[1024];
						while ( header.find( "\r\n\r\n" ) == std::string::npos ) {
							if ( cancelled() || expired() || header.size() > MaxHTTPHeaderBytes )
								return received;
							std::size_t count = 0;
							const auto status =
								socket->tcpReceive( buffer, sizeof( buffer ), count );
							if ( status == Socket::NotReady ) {
								connection.selector.wait(
									timeout > Time::Zero
										? std::min( remaining(), Milliseconds( 100 ) )
										: Milliseconds( 100 ) );
								continue;
							}
							if ( status != Socket::Done )
								return received;
							header.append( buffer, count );
						}
						Response connected;
						connected.parse( header );
						if ( !connected.isOK() || header.find( "\r\n\r\n" ) + 4 != header.size() )
							return received;
					}
					if ( ssl ) {
						auto* socket = static_cast<SSLSocket*>( connection.socket.get() );
						socket->setBlocking( true );
						connection.socket->setReceiveTimeout( remaining() );
						if ( socket->sslConnect( address, port, remaining() ) != Socket::Done )
							return received;
					}
				}
				auto& socket = *connection.socket;
				socket.setBlocking( true );
				socket.setSendTimeout( remaining() );
				if ( !progress( received, Request::Connected, 0, 0 ) || expired() )
					return received;
				const Request toSend( target->prepareFields( current ) );
				const std::string wire = toSend.prepare( *target );
				if ( current.isVerbose() )
					std::cout << "Request:\n" << wire << std::endl;
				if ( socket.send( wire.data(), wire.size() ) != Socket::Done ) {
					retryable = true;
					return received;
				}
				if ( !progress( received, Request::Sent, 0, 0 ) )
					return received;
				// Read readiness and cancellation use the selector instead of SO_RCVTIMEO:
				// Windows reports receive timeouts as a disconnection, unlike POSIX EAGAIN.
				socket.setBlocking( false );

				std::string pending;
				char buffer[PACKET_BUFFER_SIZE];
				Socket::Status lastStatus = Socket::Done;
				auto readMore = [&]() {
					while ( !cancelled() && !expired() ) {
						std::size_t count = 0;
						lastStatus = socket.receive( buffer, sizeof( buffer ), count );
						if ( lastStatus == Socket::NotReady ||
							 ( lastStatus == Socket::Done && count == 0 ) ) {
							connection.selector.wait(
								timeout > Time::Zero ? std::min( remaining(), Milliseconds( 100 ) )
													 : Milliseconds( 100 ) );
							continue;
						}
						if ( lastStatus != Socket::Done )
							return false;
						responseBytes = true;
						pending.append( buffer, count );
						return true;
					}
					return false;
				};
				unsigned informationals = 0;
				for ( ;; ) {
					std::size_t end = pending.find( "\r\n\r\n" );
					std::size_t delimiter = 4;
					if ( end == std::string::npos ) {
						end = pending.find( "\n\n" );
						delimiter = 2;
					}
					if ( end == std::string::npos ) {
						if ( pending.size() > MaxHTTPHeaderBytes )
							return invalid();
						if ( !readMore() ) {
							retryable = !responseBytes && lastStatus == Socket::Disconnected;
							return responseBytes ? invalid() : received;
						}
						continue;
					}
					end += delimiter;
					if ( end > MaxHTTPHeaderBytes )
						return invalid();
					received.parse( pending.substr( 0, end ) );
					pending.erase( 0, end );
					if ( received.getStatus() == Response::InvalidResponse )
						return received;
					if ( received.getStatus() >= 200 )
						break;
					if ( received.getStatus() == 101 ||
						 ++informationals > MaxInformationalResponses )
						return invalid();
				}

				const auto status = received.getStatus();
				if ( current.getMethod() == Request::Connect && received.isOK() )
					return invalid(); // Raw upgraded/tunnel streams are not exposed by this API.
				const bool noBody = current.getMethod() == Request::Head ||
									status == Response::NoContent ||
									status == Response::NotModified;
				const bool hasLength = received.hasField( "content-length" );
				Uint64 length = 0;
				if ( hasLength &&
					 !parseContentLength( received.getField( "content-length" ), length ) )
					return invalid();
				const std::string transfer =
					String::toLower( received.getField( "transfer-encoding" ) );
				const bool chunked = !noBody && transfer == "chunked";
				if ( !noBody && !transfer.empty() && ( !chunked || hasLength ) )
					return invalid();
				bool persistent = !hasToken( toSend.getField( "connection" ), "close" ) &&
								  !hasToken( received.getField( "connection" ), "close" ) &&
								  ( received.mMinorVersion == 1 ||
									hasToken( received.getField( "connection" ), "keep-alive" ) ) &&
								  ( toSend.mMinorVersion == 1 ||
									hasToken( toSend.getField( "connection" ), "keep-alive" ) );
				if ( target->isProxied() && !tunnel &&
					 hasToken( received.getField( "proxy-connection" ), "close" ) )
					persistent = false;
				const bool redirect =
					current.getFollowRedirect() && redirects < current.getMaxRedirects() &&
					isRedirectStatus( status ) && !received.getField( "location" ).empty();
				if ( !progress( received, Request::HeaderReceived, length, 0 ) )
					return received;

				// Redirect bodies are discarded, but fully framed before another request can use
				// this socket. Their content coding need not be decoded for navigation.
				DiscardStream discard;
				IOStream& destination = redirect ? static_cast<IOStream&>( discard ) : writeTo;
				std::optional<IOStreamInflate> inflate;
				if ( !noBody && !redirect ) {
					const auto encoding =
						String::toLower( received.getField( "content-encoding" ) );
					if ( encoding == "gzip" || encoding == "deflate" || encoding == "br" ) {
						inflate.emplace( destination,
										 encoding == "gzip"
											 ? Compression::MODE_GZIP
											 : ( encoding == "br" ? Compression::MODE_BROTLI
																  : Compression::MODE_DEFLATE ) );
					}
				}
				IOStream& decoded = inflate ? static_cast<IOStream&>( *inflate ) : destination;
				std::optional<HttpStreamChunked> chunks;
				if ( chunked )
					chunks.emplace( decoded );
				IOStream& body = chunks ? static_cast<IOStream&>( *chunks ) : decoded;
				Uint64 count = 0;
				bool complete = false;
				for ( ;; ) {
					if ( cancelled() || expired() )
						return invalid();
					if ( !pending.empty() ) {
						// Bytes beyond the framed message cannot belong to it (no pipelining). Like
						// browsers, keep the message and never reuse the connection, instead of
						// failing a response that is complete.
						std::size_t size = pending.size();
						if ( noBody )
							size = 0;
						else if ( !chunked && hasLength && size > length - count )
							size = static_cast<std::size_t>( length - count );
						if ( size != pending.size() )
							persistent = false;
						const ios_size written = size > 0 ? body.write( pending.data(), size ) : 0;
						if ( chunks && chunks->isComplete() && written >= 0 &&
							 static_cast<std::size_t>( written ) < size ) {
							persistent = false;
						} else if ( written != static_cast<ios_size>( size ) ) {
							return invalid();
						}
						count += written;
						pending.clear();
						if ( written > 0 &&
							 !progress( received, Request::ContentReceived, length, count ) )
							return invalid();
					}
					complete =
						noBody || ( chunks ? chunks->isComplete() : hasLength && count == length );
					if ( complete )
						break;
					if ( !readMore() ) {
						complete = !cancelled() && !expired() &&
								   lastStatus == Socket::Disconnected && !chunked && !hasLength;
						persistent = false;
						break;
					}
				}
				// An empty body has no compressed stream to complete, e.g. Content-Length: 0 with
				// Content-Encoding: gzip.
				if ( !complete || ( inflate && inflate->hasInput() &&
									( inflate->hasError() || !inflate->isComplete() ) ) )
					return invalid();
				if ( chunks && !chunks->getHeaderBuffer().empty() ) {
					Response trailers;
					std::istringstream in( chunks->getHeaderBuffer() );
					if ( !trailers.parseFields( in ) )
						return invalid();
					for ( auto& field : trailers.mFields ) {
						if ( field.first == "content-length" ||
							 field.first == "transfer-encoding" || field.first == "connection" ||
							 field.first == "content-encoding" || field.first == "host" )
							return invalid();
						received.mFields.insert_or_assign( field.first, std::move( field.second ) );
					}
				}
				lease.reusable = persistent && !cancelled();
				return received;
			};
			response = perform();
			if ( !retryable || !lease.reused || responseBytes || cancelled() || expired() ||
				 ( current.getMethod() != Request::Get && current.getMethod() != Request::Head &&
				   current.getMethod() != Request::Options ) )
				break;
		}
		if ( cancelled() ) {
			onCancel( current );
			return response;
		}
		if ( !current.getFollowRedirect() || redirects >= current.getMaxRedirects() ||
			 !isRedirectStatus( response.getStatus() ) || response.getField( "location" ).empty() )
			return response;
		if ( !progress( response, Request::Redirect, 0, 0 ) ) {
			onCancel( current );
			return response;
		}
		URI next = target->getURI();
		next.setPathEtc( current.getUri() );
		next.resolve( response.getField( "location" ) );
		if ( next.getScheme() != "http" && next.getScheme() != "https" ) {
			response.mStatus = Response::InvalidResponse;
			return response;
		}
		const bool sameOrigin = originKey( next ) == originKey( target->getURI() );
		Request nextRequest( current );
		nextRequest.mFields.erase( "host" );
		if ( !sameOrigin ) {
			nextRequest.mFields.erase( "authorization" );
			nextRequest.mFields.erase( "cookie" );
		} else if ( response.hasField( "set-cookie" ) ) {
			// Each Set-Cookie line sets one cookie, replacing any previous value of that name.
			std::string cookies = current.getField( "cookie" );
			String::readBySeparator(
				response.getField( "set-cookie" ),
				[&]( std::string_view setCookie ) {
					std::string_view pair =
						String::trim( setCookie.substr( 0, setCookie.find( ';' ) ) );
					const auto equals = pair.find( '=' );
					if ( equals == std::string_view::npos || equals == 0 )
						return;
					const auto name = pair.substr( 0, equals );
					std::string merged( pair );
					String::readBySeparator(
						cookies,
						[&]( std::string_view part ) {
							part = String::trim( part );
							if ( !part.empty() && part.substr( 0, part.find( '=' ) ) != name ) {
								merged += "; ";
								merged.append( part );
							}
						},
						';' );
					cookies = std::move( merged );
				},
				'\n' );
			if ( !cookies.empty() )
				nextRequest.setField( "Cookie", cookies );
		}
		const auto method =
			Request::getRedirectMethodFromStatus( current.getMethod(), response.getStatus() );
		if ( method != current.getMethod() ) {
			nextRequest.setBody( "" );
			nextRequest.mFields.erase( "content-length" );
			nextRequest.mFields.erase( "content-type" );
		}
		nextRequest.setMethod( method );
		nextRequest.setUri( next.getPathAndQuery() );
		modifiedRequest.emplace( std::move( nextRequest ) );
		currentRequest = &*modifiedRequest;
		if ( !sameOrigin ) {
			auto nextHttp = std::unique_ptr<Http>( new Http( connections, next, target->mProxy ) );
			redirected = std::move( nextHttp );
			target = redirected.get();
		}
	}
}

Http::Request::Method
Http::Request::getRedirectMethodFromStatus( Method requestMethod,
											Response::Status responseStatus ) {
	// 1. 307 and 308 ALWAYS preserve the original method.
	if ( responseStatus == Http::Response::PermanentRedirect ||
		 responseStatus == Http::Response::TemporaryRedirect ) {
		return requestMethod;
	}
	// 2. 303 See Other ALWAYS converts to GET (unless it
	// was a HEAD).
	if ( responseStatus == Http::Response::SeeOther ) {
		return requestMethod == Http::Request::Method::Head ? Http::Request::Method::Head
															: Http::Request::Method::Get;
	}
	// 3. 301 and 302 historically convert POST to GET, but
	// preserve others.
	return requestMethod == Http::Request::Method::Post ? Http::Request::Method::Get
														: requestMethod;
}

Http::Response Http::downloadRequest( const Http::Request& request, std::string writePath,
									  Time timeout ) {
	IOStreamFile file( writePath, request.isContinue() ? "ab+" : "wb+" );
	return downloadRequest( request, file, timeout );
}

void Http::setThreadPool( std::shared_ptr<ThreadPool> pool ) {
	std::lock_guard<std::mutex> lock( sGlobalThreadPoolMutex );
	sGlobalThreadPool = pool;
}

std::atomic<Uint64> Http::AsyncRequest::IdCounter = 1;
thread_local Http::AsyncRequest* Http::AsyncRequest::sCurrent = nullptr;

Http::AsyncRequest::AsyncRequest( Uint64 id, Http* http, const Http::AsyncResponseCallback& cb,
								  Http::Request request, Time timeout, bool fromLocalPool ) :
	mId( id ),
	mHttp( http ),
	mOwner( fromLocalPool ? http->weak_from_this().lock() : nullptr ),
	mCb( cb ),
	mRequest( std::move( request ) ),
	mTimeout( timeout ),
	mStreamed( false ),
	mStreamOwned( false ),
	mFromLocalPool( fromLocalPool ),
	mStream( NULL ) {}

Http::AsyncRequest::AsyncRequest( Uint64 id, Http* http, const Http::AsyncResponseCallback& cb,
								  Http::Request request, IOStream& writeTo, Time timeout,
								  bool fromLocalPool ) :
	mId( id ),
	mHttp( http ),
	mOwner( fromLocalPool ? http->weak_from_this().lock() : nullptr ),
	mCb( cb ),
	mRequest( std::move( request ) ),
	mTimeout( timeout ),
	mStreamed( true ),
	mStreamOwned( false ),
	mFromLocalPool( fromLocalPool ),
	mStream( &writeTo ) {}

Http::AsyncRequest::AsyncRequest( Uint64 id, Http* http, const Http::AsyncResponseCallback& cb,
								  Http::Request request, std::string writePath, Time timeout,
								  bool fromLocalPool ) :
	mId( id ),
	mHttp( http ),
	mOwner( fromLocalPool ? http->weak_from_this().lock() : nullptr ),
	mCb( cb ),
	mRequest( std::move( request ) ),
	mTimeout( timeout ),
	mStreamed( true ),
	mStreamOwned( true ),
	mFromLocalPool( fromLocalPool ),
	mStream( IOStreamFile::New( writePath, "wb" ) ) {}

Http::AsyncRequest::~AsyncRequest() {
	if ( mStreamOwned )
		eeSAFE_DELETE( mStream );
}

void Http::AsyncRequest::cancel( bool resetCancelCallback ) {
	mRequest.cancel( resetCancelCallback );
}

void Http::AsyncRequest::run() {
	AsyncRequest* previousRequest = sCurrent;
	sCurrent = this;

	Http::Response response = mStreamed ? mHttp->downloadRequest( mRequest, *mStream, mTimeout )
										: mHttp->sendRequest( mRequest, mTimeout );

	if ( !mRequest.isCancelled() )
		mCb( *mHttp, mRequest, response );

	if ( mStreamed && mStreamOwned ) {
		eeSAFE_DELETE( mStream );
	}

	if ( mFromLocalPool && mHttp->removeAsyncRequest( this ) ) {
		auto me = this;
		sCurrent = previousRequest;
		eeSAFE_DELETE( me );
		return;
	}

	sCurrent = previousRequest;
}

Http::AsyncRequest* Http::AsyncRequest::current() {
	return sCurrent;
}

Http::SharedRequestOperation::SharedRequestOperation( Http& http,
													  std::shared_ptr<AsyncRequest> request ) :
	mHttp( http ), mOwner( http.weak_from_this().lock() ), mRequest( std::move( request ) ) {}

Http::SharedRequestOperation::~SharedRequestOperation() {
	mHttp.completeSharedRequest( mRequest );
}

void Http::SharedRequestOperation::run() {
	mRequest->run();
}

bool Http::removeAsyncRequest( AsyncRequest* req ) {
	{
		std::lock_guard<std::mutex> lock( mRequestsMutex );
		mCurRequests.erase( req->id() );
	}

	Lock l( mThreadsMutex );
	if ( mShuttingDown.load( std::memory_order_acquire ) )
		return false;
	auto found = std::find( mThreads.begin(), mThreads.end(), req );
	if ( found == mThreads.end() )
		return false;
	mThreads.erase( found );
	return true;
}

bool Http::registerSharedRequest( const std::shared_ptr<AsyncRequest>& request ) {
	std::lock_guard<std::mutex> lock( mRequestsMutex );
	if ( mShuttingDown.load( std::memory_order_acquire ) )
		return false;
	mCurRequests[request->id()] = request.get();
	mSharedRequests[request->id()] = request;
	return true;
}

void Http::completeSharedRequest( const std::shared_ptr<AsyncRequest>& request ) {
	{
		std::lock_guard<std::mutex> lock( mRequestsMutex );
		mCurRequests.erase( request->id() );
		mSharedRequests.erase( request->id() );
	}
	mRequestsComplete.notify_all();
}

bool Http::scheduleSharedRequest( const std::shared_ptr<ThreadPool>& threadPool,
								  const std::shared_ptr<AsyncRequest>& request ) {
	if ( !registerSharedRequest( request ) ) {
		request->cancel();
		return false;
	}

	// The executor is externally owned and may discard queued work. The operation reports
	// completion when the queued function releases it, whether or not run() was called.
	auto operation = std::make_shared<SharedRequestOperation>( *this, request );
	threadPool->run( [operation] { operation->run(); } );
	return true;
}

void Http::shutdown( bool waitForSharedRequests ) {
	std::vector<std::shared_ptr<AsyncRequest>> sharedRequests;
	{
		std::lock_guard<std::mutex> lock( mRequestsMutex );
		mShuttingDown.store( true, std::memory_order_release );
		sharedRequests.reserve( mSharedRequests.size() );
		for ( const auto& [_, request] : mSharedRequests )
			sharedRequests.emplace_back( request );
	}

	for ( const auto& request : sharedRequests )
		request->cancel();

	std::vector<AsyncRequest*> threads;
	{
		Lock l( mThreadsMutex );
		threads.swap( mThreads );
	}
	for ( auto* thread : threads ) {
		thread->cancel();
		thread->wait();
		{
			std::lock_guard<std::mutex> lock( mRequestsMutex );
			mCurRequests.erase( thread->id() );
		}
		eeDelete( thread );
	}
	if ( !waitForSharedRequests )
		return;

	AsyncRequest* currentRequest = AsyncRequest::current();
	std::unique_lock<std::mutex> lock( mRequestsMutex );
	mRequestsComplete.wait( lock, [this, currentRequest] {
		if ( currentRequest && currentRequest->mHttp == this && !currentRequest->mFromLocalPool ) {
			return mSharedRequests.empty() ||
				   ( mSharedRequests.size() == 1 &&
					 mSharedRequests.begin()->second.get() == currentRequest );
		}
		return mSharedRequests.empty();
	} );
}

Http::Request Http::prepareFields( const Http::Request& request ) {
	Request toSend( request );

	if ( !toSend.hasField( "User-Agent" ) )
		toSend.setField( "User-Agent", sDefaultUserAgent );

	if ( !toSend.hasField( "Accept" ) )
		toSend.setField( "Accept", "*/*" );

	if ( !toSend.hasField( "Host" ) ) {
		toSend.setField(
			"Host",
			mHostName + ( mPort != ( mIsSSL ? 443 : 80 ) ? ":" + String::toString( mPort ) : "" ) );
	}

	if ( !toSend.hasField( "Content-Length" ) && toSend.mBody.size() > 0 ) {
		std::ostringstream out;
		out << toSend.mBody.size();
		toSend.setField( "Content-Length", out.str() );
	}

	if ( ( toSend.mMethod == Request::Post ) && !toSend.hasField( "Content-Type" ) )
		toSend.setField( "Content-Type", "application/x-www-form-urlencoded" );

	if ( mIsSSL && isProxied() ) {
		// Proxy credentials belong to CONNECT, never to the origin inside its TLS tunnel.
		toSend.mFields.erase( "proxy-authorization" );
		toSend.mFields.erase( "proxy-connection" );
	}

	// HTTP/1.1 persistence is the default. Respect an explicit Connection: close request.
	if ( request.isCompressedResponse() )
		toSend.setField( "Accept-Encoding", "gzip, deflate, br" );

	return toSend;
}

void Http::setProxy( const URI& uri ) {
	setHost( mHostName, mPort, mIsSSL, uri );
}

const URI& Http::getProxy() const {
	return mProxy;
}

bool Http::isProxied() const {
	return !mProxy.empty();
}

bool Http::setCancelRequest( Uint64 reqId, bool resetCancelCallback ) {
	std::lock_guard<std::mutex> lock( mRequestsMutex );
	auto found = mCurRequests.find( reqId );
	if ( found != mCurRequests.end() ) {
		found->second->cancel( resetCancelCallback );
		return true;
	}
	return false;
}

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
struct WGetAsyncRequest {
	Http* http;
	// Browser fetches, like native async operations, must retain a shared client after a
	// document partition closes. The closed flag suppresses its late completion.
	std::shared_ptr<Http> owner;
	const std::atomic<bool>* closed;
	Http::Request request;
	Http::AsyncResponseCallback cb;
	IOStream* writeTo{ nullptr };

	bool isCancelled() const {
		return request.isCancelled() || closed->load( std::memory_order_acquire );
	}
};

void emscripten_async_wget2_got_data( unsigned, void* vwget, void* buffer, unsigned bufferSize ) {
	auto wget = std::unique_ptr<WGetAsyncRequest>( static_cast<WGetAsyncRequest*>( vwget ) );
	if ( wget->isCancelled() )
		return;
	Http::Response::Status status = Http::Response::Status::Ok;
	if ( wget->writeTo ) {
		wget->writeTo->write( (const char*)buffer, bufferSize );
		Http::Response response =
			Http::Response::createFakeResponse( Http::Response::FieldTable(), status, "" );
		wget->cb( *wget->http, wget->request, response );
	} else {
		std::string responseBody;
		responseBody.insert( 0, (const char*)buffer, bufferSize );
		Http::Response response = Http::Response::createFakeResponse( Http::Response::FieldTable(),
																	  status, responseBody );
		wget->cb( *wget->http, wget->request, response );
	}
}

void emscripten_async_wget2_got_file( unsigned int, void* vwget, const char* ) {
	auto wget = std::unique_ptr<WGetAsyncRequest>( static_cast<WGetAsyncRequest*>( vwget ) );
	if ( wget->isCancelled() )
		return;
	Http::Response::Status status = Http::Response::Status::Ok;
	Http::Response response =
		Http::Response::createFakeResponse( Http::Response::FieldTable(), status, "" );
	wget->cb( *wget->http, wget->request, response );
}

void emscripten_async_wget2_got_error_data( unsigned, void* vwget, int errorCode,
											const char* errorDescription ) {
	auto wget = std::unique_ptr<WGetAsyncRequest>( static_cast<WGetAsyncRequest*>( vwget ) );
	if ( wget->isCancelled() )
		return;
	std::string responseBody;
	Http::Response::Status status = Http::Response::intAsStatus( errorCode );
	Http::Response response =
		Http::Response::createFakeResponse( Http::Response::FieldTable(), status, responseBody );
	wget->cb( *wget->http, wget->request, response );
}

void emscripten_async_wget2_got_error_file( unsigned int, void* vwget, int errorCode ) {
	auto wget = std::unique_ptr<WGetAsyncRequest>( static_cast<WGetAsyncRequest*>( vwget ) );
	if ( wget->isCancelled() )
		return;
	std::string responseBody;
	Http::Response::Status status = Http::Response::intAsStatus( errorCode );
	Http::Response response =
		Http::Response::createFakeResponse( Http::Response::FieldTable(), status, responseBody );
	wget->cb( *wget->http, wget->request, response );
}
#endif

Uint64 Http::sendAsyncRequest( const Http::AsyncResponseCallback& cb, const Http::Request& request,
							   Time timeout ) {
	Uint64 id = Http::AsyncRequest::IdCounter.fetch_add( 1, std::memory_order_relaxed );
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	WGetAsyncRequest* wget = new WGetAsyncRequest();
	wget->http = this;
	wget->owner = weak_from_this().lock();
	wget->closed = &mConnections->closed;
	wget->cb = cb;
	wget->request = Http::Request( request );
	emscripten_async_wget2_data( ( getURI().toString() + request.getUri() ).c_str(),
								 Request::methodToString( request.getMethod() ).c_str(),
								 URI( request.getUri() ).getQuery().c_str(), wget, 1,
								 emscripten_async_wget2_got_data,
								 emscripten_async_wget2_got_error_data, NULL );
	return id;
#else
	auto threadPool = getGlobalThreadPool();
	if ( threadPool ) {
		auto asyncRequest = std::make_shared<AsyncRequest>( id, this, cb, request, timeout, false );
		scheduleSharedRequest( threadPool, asyncRequest );
		return id;
	}
	AsyncRequest* thread = eeNew( AsyncRequest, ( id, this, cb, request, timeout, true ) );
	{
		Lock threadsLock( mThreadsMutex );
		{
			std::lock_guard<std::mutex> requestsLock( mRequestsMutex );
			if ( mShuttingDown.load( std::memory_order_acquire ) ) {
				eeDelete( thread );
				return id;
			}
			mCurRequests[id] = thread;
		}
		mThreads.push_back( thread );
		thread->launch();
	}
	return id;
#endif
}

Uint64 Http::downloadAsyncRequest( const Http::AsyncResponseCallback& cb,
								   const Http::Request& request, IOStream& writeTo, Time timeout ) {
	Uint64 id = Http::AsyncRequest::IdCounter.fetch_add( 1, std::memory_order_relaxed );
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	WGetAsyncRequest* wget = new WGetAsyncRequest();
	wget->http = this;
	wget->owner = weak_from_this().lock();
	wget->closed = &mConnections->closed;
	wget->cb = cb;
	wget->writeTo = &writeTo;
	wget->request = Http::Request( request );
	emscripten_async_wget2_data( ( getURI().toString() + request.getUri() ).c_str(),
								 Request::methodToString( request.getMethod() ).c_str(),
								 URI( request.getUri() ).getQuery().c_str(), wget, 1,
								 emscripten_async_wget2_got_data,
								 emscripten_async_wget2_got_error_data, NULL );
	return id;
#else
	auto threadPool = getGlobalThreadPool();
	if ( threadPool ) {
		auto asyncRequest =
			std::make_shared<AsyncRequest>( id, this, cb, request, writeTo, timeout, false );
		scheduleSharedRequest( threadPool, asyncRequest );
		return id;
	}
	AsyncRequest* thread = eeNew( AsyncRequest, ( id, this, cb, request, writeTo, timeout, true ) );
	{
		Lock threadsLock( mThreadsMutex );
		{
			std::lock_guard<std::mutex> requestsLock( mRequestsMutex );
			if ( mShuttingDown.load( std::memory_order_acquire ) ) {
				eeDelete( thread );
				return id;
			}
			mCurRequests[id] = thread;
		}
		mThreads.push_back( thread );
		thread->launch();
	}
	return id;
#endif
}

Uint64 Http::downloadAsyncRequest( const Http::AsyncResponseCallback& cb,
								   const Http::Request& request, std::string writePath,
								   Time timeout ) {
	Uint64 id = Http::AsyncRequest::IdCounter.fetch_add( 1, std::memory_order_relaxed );
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	WGetAsyncRequest* wget = new WGetAsyncRequest();
	wget->http = this;
	wget->owner = weak_from_this().lock();
	wget->closed = &mConnections->closed;
	wget->cb = cb;
	wget->request = Http::Request( request );
	emscripten_async_wget2( ( getURI().toString() + request.getUri() ).c_str(), writePath.c_str(),
							Request::methodToString( request.getMethod() ).c_str(),
							URI( request.getUri() ).getQuery().c_str(), wget,
							emscripten_async_wget2_got_file, emscripten_async_wget2_got_error_file,
							NULL );
#else
	auto threadPool = getGlobalThreadPool();
	if ( threadPool ) {
		auto asyncRequest =
			std::make_shared<AsyncRequest>( id, this, cb, request, writePath, timeout, false );
		scheduleSharedRequest( threadPool, asyncRequest );
		return id;
	}
	AsyncRequest* thread =
		eeNew( AsyncRequest, ( id, this, cb, request, writePath, timeout, true ) );
	{
		Lock threadsLock( mThreadsMutex );
		{
			std::lock_guard<std::mutex> requestsLock( mRequestsMutex );
			if ( mShuttingDown.load( std::memory_order_acquire ) ) {
				eeDelete( thread );
				return id;
			}
			mCurRequests[id] = thread;
		}
		mThreads.push_back( thread );
		thread->launch();
	}
#endif
	return id;
}

const IpAddress& Http::getHost() const {
	return mHost;
}

const std::string& Http::getHostName() const {
	return mHostName;
}

const unsigned short& Http::getPort() const {
	return mPort;
}

const bool& Http::isSSL() const {
	return mIsSSL;
}

URI Http::getURI() const {
	return URI(
		String::format( "%s://%s:%d", mIsSSL ? "https" : "http", mHostName.c_str(), mPort ) );
}

Http::Pool& Http::Pool::getGlobal() {
	return sGlobalHttpPool;
}

Http::Pool::Pool() : Pool( ConnectionPoolOptions{} ) {}

Http::Pool::Pool( const ConnectionPoolOptions& options ) :
	mConnections( std::make_shared<ConnectionPool>( options ) ) {}

Http::Pool::~Pool() {
	if ( !mClosed )
		clear();
}

void Http::Pool::clear() {
	decltype( mHttps ) clients;
	std::shared_ptr<ConnectionPool> connections;
	{
		Lock lock( mMutex );
		if ( mClosed )
			return;
		clients.swap( mHttps );
		connections = std::move( mConnections );
		mConnections = std::make_shared<ConnectionPool>( connections->options );
	}
	connections->closeIdle( true );
	// Callbacks may re-enter the client pool. Never join while holding its mutex, and never
	// wait on shared-executor work queued behind the callback doing the clearing.
	AsyncRequest* current = AsyncRequest::current();
	const bool wait = !current || current->fromLocalPool();
	for ( const auto& client : clients )
		client.second->shutdown( wait );
}

void Http::Pool::close() {
	decltype( mHttps ) clients;
	std::shared_ptr<ConnectionPool> connections;
	{
		Lock lock( mMutex );
		mClosed = true;
		clients.swap( mHttps );
		connections = mConnections;
	}
	// Async operations retain their clients in both executor modes. Closing their transport
	// makes queued/running work observe cancellation without joining any worker on the UI thread.
	connections->closeIdle( true );
}

void Http::Pool::closeIdleConnections() {
	std::shared_ptr<ConnectionPool> connections;
	{
		Lock lock( mMutex );
		connections = mConnections;
	}
	connections->closeIdle();
}

std::string Http::Pool::getHostKey( const URI& host, const URI& proxy ) {
	return transportKey( host, proxy );
}

bool Http::Pool::exists( const URI& host, const URI& proxy ) {
	Lock lock( mMutex );
	return mHttps.find( getHostKey( host, proxy ) ) != mHttps.end();
}

std::shared_ptr<Http> Http::Pool::get( const URI& host, const URI& proxy ) {
	const std::string key = getHostKey( host, proxy );
	Lock lock( mMutex );
	auto found = mHttps.find( key );
	if ( found != mHttps.end() )
		return found->second;
	// The complete key participates in equality; hash collisions cannot alias origins/proxies.
	// Constructing under the lock also prevents concurrent first requests creating two clients.
	auto client = std::shared_ptr<Http>( new Http( mConnections, host, proxy ) );
	if ( !mClosed )
		mHttps.emplace( key, client );
	return client;
}

static constexpr const char* TWO_HYPHENS = "--";
static constexpr const char* LINE_END = "\r\n";

Http::MultipartEntitiesBuilder::MultipartEntitiesBuilder() :
	MultipartEntitiesBuilder( "eepp-client-boundary-" +
							  String::toString( Sys::getUnixTimestamp() ) ) {}

Http::MultipartEntitiesBuilder::MultipartEntitiesBuilder( const std::string& boundary ) :
	mBoundary( boundary ) {}

std::string Http::MultipartEntitiesBuilder::getContentType() {
	return "multipart/form-data;boundary=" + getBoundary();
}

const std::string& Http::MultipartEntitiesBuilder::getBoundary() const {
	return mBoundary;
}

void Http::MultipartEntitiesBuilder::addParameter( const std::string& name,
												   const std::string& value ) {
	mParams[name] = value;
}

void Http::MultipartEntitiesBuilder::addFile( const std::string& parameterName,
											  const std::string& fileName, IOStream* stream ) {
	auto pair = std::make_pair( fileName, stream );

	mStreamParams[parameterName] = pair;
}

void Http::MultipartEntitiesBuilder::addFile( const std::string& parameterName,
											  const std::string& filePath ) {
	mFileParams[parameterName] = filePath;
}

std::string Http::MultipartEntitiesBuilder::build() {
	std::ostringstream ostream;

	for ( auto& file : mStreamParams ) {
		buildFilePart( ostream, file.second.second, file.first, file.second.first, "" );
	}

	for ( auto& file : mFileParams ) {
		IOStreamFile f( file.second );
		buildFilePart( ostream, &f, file.first, FileSystem::fileNameFromPath( file.second ), "" );
	}

	for ( auto& text : mParams ) {
		buildTextPart( ostream, text.first, text.second );
	}

	ostream << TWO_HYPHENS << getBoundary() << TWO_HYPHENS << LINE_END;

	return ostream.str();
}

void Http::MultipartEntitiesBuilder::buildFilePart( std::ostream& ostream, IOStream* stream,
													const std::string& fieldName,
													const std::string& fileName,
													const std::string& contentType ) {
	size_t initialPos = stream->tell();
	stream->seek( 0 );
	int bytesAvailable = stream->getSize();
	int maxBufferSize = 1024 * 1024;
	int bufferSize = eemin( bytesAvailable, maxBufferSize );
	TScopedBuffer<char> buffer( bufferSize );

	ostream << TWO_HYPHENS << getBoundary() << LINE_END;
	ostream << "Content-Disposition: form-data; name=\"" << fieldName << "\"; filename=\""
			<< fileName << "\"" << LINE_END;
	ostream << "Content-Transfer-Encoding: binary" << LINE_END;
	ostream << "Content-Length: " << bytesAvailable << LINE_END;
	if ( !contentType.empty() ) {
		ostream << "Content-Type: " << contentType << LINE_END;
	}
	ostream << LINE_END;

	// read file and write it into form...
	int bytesRead = stream->read( buffer.get(), bufferSize );

	while ( bytesRead > 0 ) {
		ostream.write( buffer.get(), bufferSize );
		bytesAvailable -= bytesRead;
		bufferSize = eemin( bytesAvailable, maxBufferSize );
		bytesRead = stream->read( buffer.get(), bufferSize );
	}

	ostream << LINE_END;
	stream->seek( initialPos );
}

void Http::MultipartEntitiesBuilder::buildTextPart( std::ostream& ostream,
													const std::string& parameterName,
													const std::string& parameterValue ) {
	ostream << TWO_HYPHENS << getBoundary() << LINE_END;
	ostream << "Content-Disposition: form-data; name=\"" << parameterName << "\"" << LINE_END;
	ostream << "Content-Type: text/plain; charset=UTF-8" << LINE_END;
	ostream << LINE_END;
	ostream << parameterValue;
	ostream << LINE_END;
}

}} // namespace EE::Network
