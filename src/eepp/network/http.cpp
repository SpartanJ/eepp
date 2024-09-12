#include <algorithm>
#include <cctype>
#include <eepp/network/http.hpp>
#include <eepp/network/http/httpstreamchunked.hpp>
#include <eepp/network/ssl/sslsocket.hpp>
#include <eepp/network/uri.hpp>
#include <eepp/system/compression.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostream.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreaminflate.hpp>
#include <eepp/system/iostreamstring.hpp>
#include <eepp/system/sys.hpp>
#include <iostream>
#include <iterator>
#include <limits>
#include <sstream>

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
#include <emscripten.h>
#endif

using namespace EE::Network::SSL;
using namespace EE::Network::Private;

namespace EE { namespace Network {

#define PACKET_BUFFER_SIZE ( 16384 )

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
	mCancel( false ),
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

const Http::Request::ProgressCallback& Http::Request::getProgressCallback() const {
	return mProgressCallback;
}

void Http::Request::cancel() {
	mCancel = true;
}

const bool& Http::Request::isCancelled() const {
	return mCancel;
}

std::string Http::Request::prepareTunnel( const Http& http ) {
	std::ostringstream out;

	setMethod( Connect );

	std::string method = methodToString( mMethod );

	out << method << " " << http.getHostName() << ":" << http.getPort() << " ";
	out << "HTTP/" << mMajorVersion << "." << mMinorVersion << "\r\n";

	setField( "Host", String::format( "%s:%d", http.getHostName().c_str(), http.getPort() ) );
	setField( "Proxy-Connection", "Keep-Alive" );
	setField( "User-Agent", "eepp-network" );

	for ( FieldTable::const_iterator i = mFields.begin(); i != mFields.end(); ++i )
		out << i->first << ": " << i->second << "\r\n";

	out << "\r\n";

	return out.str();
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
	if ( http.getProxy().empty() ) {
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
	char* http_proxy = getenv( "http_proxy" );
	URI proxy;

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

const char* Http::Response::getStatusDescription() const {
	switch ( mStatus ) {
		// 2xx: success
		case Ok:
			return "Successfull";
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
			return "The requested page has permanently moved to a new location";
		case MovedTemporarily:
			return "The requested page has temporarily moved to a new location";
		case NotModified:
			return "For conditionnal requests, means the requested page hasn't changed and doesn't "
				   "need to be refreshed";

		// 4xx: client error
		case BadRequest:
			return "The server couldn't understand the request (syntax error)";
		case Unauthorized:
			return "The requested page needs an authentification to be accessed";
		case Forbidden:
			return "The requested page cannot be accessed at all, even with authentification";
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
	std::istringstream in( data );

	// Extract the HTTP version from the first line
	std::string version;

	if ( in >> version ) {
		std::locale loc;
		if ( ( version.size() >= 8 ) && ( version[6] == '.' ) &&
			 ( String::toLower( version.substr( 0, 5 ) ) == "http/" ) &&
			 std::isdigit( version[5], loc ) && std::isdigit( version[7], loc ) ) {
			mMajorVersion = version[5] - '0';
			mMinorVersion = version[7] - '0';
		} else {
			// Invalid HTTP version
			mStatus = InvalidResponse;
			return;
		}
	}

	// Extract the status code from the first line
	int status = InvalidResponse;

	if ( in >> status ) {
		mStatus = static_cast<Status>( status );
	} else {
		// Invalid status code
		mStatus = InvalidResponse;
		return;
	}

	// Ignore the end of the first line
	in.ignore( std::numeric_limits<std::streamsize>::max(), '\n' );

	// Parse the other lines, which contain fields, one by one
	parseFields( in );

	mBody.clear();
}

void Http::Response::parseFields( std::istream& in ) {
	std::string line;
	while ( std::getline( in, line ) && ( line.size() > 2 ) ) {
		std::string::size_type pos = line.find( ": " );

		if ( pos != std::string::npos ) {
			// Extract the field name and its value
			std::string field = line.substr( 0, pos );
			std::string value = line.substr( pos + 2 );

			// Remove any trailing \r
			if ( !value.empty() && ( *value.rbegin() == '\r' ) )
				value.erase( value.size() - 1 );

			// Add the field
			mFields[String::toLower( field )] = value;
		}
	}
}

static Http::Pool sGlobalHttpPool = Http::Pool();

static std::shared_ptr<ThreadPool> sGlobalThreadPool = nullptr;

Http::Response Http::request( const URI& uri, Request::Method method, const Time& timeout,
							  const Http::Request::ProgressCallback& progressCallback,
							  const Http::Request::FieldTable& headers, const std::string& body,
							  const bool& validateCertificate, const URI& proxy ) {
	Http* http = sGlobalHttpPool.get( uri, proxy );
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

void Http::requestAsync( const Http::AsyncResponseCallback& cb, const URI& uri, const Time& timeout,
						 Request::Method method,
						 const Http::Request::ProgressCallback& progressCallback,
						 const Http::Request::FieldTable& headers, const std::string& body,
						 const bool& validateCertificate, const URI& proxy ) {
	Http* http = sGlobalHttpPool.get( uri, proxy );
	Request request( uri.getPathAndQuery(), method, body, validateCertificate, validateCertificate,
					 true, true );
	request.setProgressCallback( progressCallback );

	for ( const auto& field : headers )
		request.setField( field.first, field.second );

	http->sendAsyncRequest( cb, request, timeout );
}

void Http::getAsync( const Http::AsyncResponseCallback& cb, const URI& uri, const Time& timeout,
					 const Http::Request::ProgressCallback& progressCallback,
					 const Http::Request::FieldTable& headers, const std::string& body,
					 const bool& validateCertificate, const URI& proxy ) {
	requestAsync( cb, uri, timeout, Request::Method::Get, progressCallback, headers, body,
				  validateCertificate, proxy );
}

void Http::postAsync( const Http::AsyncResponseCallback& cb, const URI& uri, const Time& timeout,
					  const Http::Request::ProgressCallback& progressCallback,
					  const Http::Request::FieldTable& headers, const std::string& body,
					  const bool& validateCertificate, const URI& proxy ) {
	requestAsync( cb, uri, timeout, Request::Method::Post, progressCallback, headers, body,
				  validateCertificate, proxy );
}

Http::Http() : mConnection( NULL ), mHost(), mPort( 0 ), mIsSSL( false ), mHostSolved( false ) {}

Http::Http( const std::string& host, unsigned short port, bool useSSL, URI proxy ) :
	mConnection( NULL ),
	mHostName( host ),
	mPort( port ),
	mIsSSL( useSSL ),
	mHostSolved( false ),
	mProxy( proxy ) {
	setHost( host, port, useSSL, proxy );
}

Http::~Http() {
	// First we wait to finish any request pending
	for ( auto&& itt : mThreads ) {
		itt->wait();
	}

	for ( auto&& itt : mThreads ) {
		eeDelete( itt );
	}

	// Then we destroy the last open connection
	HttpConnection* connection = mConnection;

	eeSAFE_DELETE( connection );
}

void Http::setHost( const std::string& host, unsigned short port, bool useSSL, URI proxy ) {
	mProxy = proxy;

	bool sameHost( host == mHostName && port == mPort && useSSL == mIsSSL );

	// Check the protocol
	if ( String::toLower( host.substr( 0, 7 ) ) == "http://" ) {
		// HTTP protocol
		mHostName = host.substr( 7 );
		mPort = ( port != 0 ? port : 80 );
	} else if ( String::toLower( host.substr( 0, 8 ) ) == "https://" ) {
// HTTPS protocol
#if defined( EE_SSL_SUPPORT ) || EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
		mIsSSL = true;
		mHostName = host.substr( 8 );
		mPort = ( port != 0 ? port : 443 );
#else
		mHostName = "";
		mPort = 0;
#endif
	} else {
		// Undefined protocol - use HTTP, unless SSL is specified
		mHostName = host;
		mPort = ( port != 0 ? port : 80 );

#if defined( EE_SSL_SUPPORT ) || EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
		mPort = useSSL ? ( port != 0 ? port : 443 ) : mPort;
		mIsSSL = useSSL || mPort == 443;
#endif
	}

	// Remove any trailing '/' from the host name
	if ( !mHostName.empty() && ( *mHostName.rbegin() == '/' ) )
		mHostName.erase( mHostName.size() - 1 );

	if ( !mProxy.empty() ) {
		sameHost = false;
	}

	// If the new host is different to the last set host
	// and there's an open connection to the host, we close
	// the old connection to prepare a new one.
	if ( !sameHost && NULL != mConnection ) {
		HttpConnection* connection = mConnection;
		eeSAFE_DELETE( connection );
		mConnection = NULL;
	}
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
	if ( request.getProgressCallback() )
		return request.getProgressCallback()( http, request, response, status, totalBytes,
											  currentBytes );
	return true;
}

Http::Response Http::downloadRequest( const Http::Request& request, IOStream& writeTo,
									  Time timeout ) {
	// Solve the host IP only when the request starts.
	if ( !mHostSolved ) {
		if ( !mProxy.empty() ) {
			mHost = IpAddress( mProxy.getHost() );
		} else {
			mHost = IpAddress( mHostName );
		}
		mHostSolved = true;
	}

	if ( 0 == mHost.toInteger() ) {
		return Response();
	}

	if ( NULL == mConnection ) {
		HttpConnection* connection = eeNew( HttpConnection, () );
		TcpSocket* socket = NULL;

		// If the http client is proxied and the end host use SSL
		// We need to create an HTTP Tunnel against the proxy server
		if ( isProxied() && mIsSSL && SSLSocket::isSupported() ) {
			socket = SSLSocket::New( mHostName, request.getValidateCertificate(),
									 request.getValidateHostname() );

			connection->setSSL( true );
		} else {
			bool isSSL = !isProxied()
							 ? mIsSSL
							 : ( SSLSocket::isSupported() && mProxy.getScheme() == "https" );

			socket = isSSL ? SSLSocket::New( mHostName, request.getValidateCertificate(),
											 request.getValidateHostname() )
						   : TcpSocket::New();

			connection->setSSL( isSSL );
		}

		if ( timeout != Time::Zero ) {
			socket->setReceiveTimeout( timeout );
			socket->setSendTimeout( timeout );
		}

		connection->setSocket( socket );

		mConnection = connection;
	}

	// First make sure that the request is valid -- add missing mandatory fields
	Request toSend( prepareFields( request ) );

	// Prepare the response
	Response received;

	// If not connected, try to connect to the server
	if ( !mConnection->isConnected() ) {
		// We need to create an HTTP Tunnel?
		if ( isProxied() && mIsSSL && SSLSocket::isSupported() ) {
			SSLSocket* sslSocket = reinterpret_cast<SSLSocket*>( mConnection->getSocket() );

			// For an HTTP Tunnel first we need to connect to the proxy server ( without TLS )
			if ( sslSocket->tcpConnect( mHost, mProxy.getPort(), timeout ) != Socket::Done ) {
				return received;
			} else {
				mConnection->setConnected( true );
			}
		} else {
			if ( mConnection->getSocket()->connect(
					 mHost, mProxy.empty() ? mPort : mProxy.getPort(), timeout ) != Socket::Done ) {
				return received;
			} else {
				mConnection->setConnected( true );
			}
		}

		if ( mConnection->isConnected() &&
			 !sendProgress( *this, request, received, Request::Connected, 0, 0 ) ) {
			mConnection->disconnect();
			return received;
		}
	}

	// Connect the socket to the host
	if ( mConnection->isConnected() ) {
		// Create a HTTP Tunnel for SSL connections if not ready
		if ( isProxied() && mIsSSL && !mConnection->isTunneled() ) {
			// Create the HTTP Tunnel request
			Request tunnelRequest;
			std::string tunnelStr = tunnelRequest.prepareTunnel( *this );

			SSLSocket* sslSocket = reinterpret_cast<SSLSocket*>( mConnection->getSocket() );
			std::size_t sent;

			// Send the request
			if ( sslSocket->tcpSend( tunnelStr.c_str(), tunnelStr.size(), sent ) == Socket::Done ) {
				char buffer[PACKET_BUFFER_SIZE + 1];
				std::size_t readed = 0;

				// Get the proxy server response
				if ( sslSocket->tcpReceive( buffer, PACKET_BUFFER_SIZE, readed ) == Socket::Done ) {
					// Parse the HTTP Tunnel request response
					Response tunnelResponse;
					std::string header;
					header.append( buffer, readed );
					tunnelResponse.parse( header );

					if ( tunnelResponse.getStatus() == Response::Ok ) {
						// Stablish the SSL connection if the response is positive
						if ( sslSocket->sslConnect( mHost, mProxy.getPort(), timeout ) !=
							 Socket::Done ) {
							return received;
						}
					} else {
						return tunnelResponse;
					}
				} else {
					return received;
				}

				mConnection->setTunneled( true );
				mConnection->setKeepAlive( true );
			}
		}

		if ( request.isContinue() ) {
			std::size_t continueLength = writeTo.getSize();

			if ( continueLength > 0 ) {
				IOStreamString responseHeadBody;
				Request requestHead = request;
				requestHead.setContinue( false );
				requestHead.setMethod( Request::Head );
				Response responseHead = downloadRequest( requestHead, responseHeadBody );
				std::size_t contentLength = 0;

				if ( responseHead.hasField( "Accept-Ranges" ) &&
					 responseHead.hasField( "Content-Length" ) &&
					 String::fromString( contentLength,
										 responseHead.getField( "Content-Length" ) ) &&
					 contentLength > 0 && continueLength < contentLength ) {
					writeTo.seek( continueLength );
					Request newRequest( request );
					newRequest.setContinue( false );
					newRequest.setField( "Range", String::format( "bytes=%lu-%lu",
																  (unsigned long)continueLength,
																  (unsigned long)contentLength ) );
					return downloadRequest( newRequest, writeTo, timeout );
				}
			}
		}

		// Convert the request to string and send it through the connected socket
		std::string requestStr = toSend.prepare( *this );

		if ( request.isVerbose() ) {
			std::cout << "Request:" << std::endl;
			std::cout << requestStr << std::endl;
		}

		if ( !requestStr.empty() ) {
			Socket::Status status;

			// Send it through the socket
			if ( mConnection->getSocket()->send( requestStr.c_str(), requestStr.size() ) ==
				 Socket::Done ) {
				if ( !sendProgress( *this, request, received, Request::Sent, 0, 0 ) ) {
					request.mCancel = true;
				}

				// Wait for the server's response
				std::size_t currentTotalBytes = 0;
				std::size_t len = 0;
				std::size_t readed = 0;
				char* eol = NULL; // end of line
				char* bol = NULL; // beginning of line
				char buffer[PACKET_BUFFER_SIZE + 1];
				bool isnheader = false;
				bool chunked = false;
				bool compressed = false;
				std::size_t contentLength = 0;
				std::string headerBuffer;
				HttpStreamChunked* chunkedStream = NULL;
				IOStreamInflate* inflateStream = NULL;
				IOStream* bufferStream = NULL;

				while ( !request.isCancelled() &&
						( status = mConnection->getSocket()->receive( buffer, PACKET_BUFFER_SIZE,
																	  readed ) ) == Socket::Done ) {
					char* readBuffer = buffer;

					// If we didn't receive the header yet, we will try to find the end of the
					// header
					if ( !isnheader ) {
						// calculate combined length of unprocessed data and new data
						len += readed;

						// NULL terminate buffer for string functions
						readBuffer[len] = '\0';

						// process each line in buffer looking for header break
						bol = readBuffer;

						while ( !isnheader && ( eol = strchr( bol, '\n' ) ) != NULL ) {
							// update bol based upon the value of eol
							bol = eol + 1;

							// test if end of headers has been reached
							if ( 0 == strncmp( bol, "\r\n", 2 ) || 0 == strncmp( bol, "\n", 1 ) ) {
								// note that end of headers has been reached
								isnheader = true;

								// update the value of bol to reflect the beginning of the line
								// immediately after the headers
								if ( bol[0] != '\n' )
									bol += 1;

								bol += 1;

								// calculate the amount of data remaining in the buffer
								len = readed - ( bol - readBuffer );

								// Fill the header buffer
								headerBuffer.append( readBuffer, ( bol - readBuffer ) );

								if ( !headerBuffer.empty() ) {
									// Build the Response object from the received data
									received.parse( headerBuffer );

									// Check if the response is chunked
									chunked = received.getField( "transfer-encoding" ) == "chunked";

									// Check if the content is compressed
									std::string encoding( received.getField( "content-encoding" ) );
									compressed = encoding == "gzip" || encoding == "deflate";

									if ( compressed ) {
										Compression::Mode compressionMode =
											"gzip" == encoding ? Compression::MODE_GZIP
															   : Compression::MODE_DEFLATE;

										inflateStream =
											IOStreamInflate::New( writeTo, compressionMode );
									}

									if ( chunked ) {
										IOStream& writeToStream =
											compressed ? *inflateStream : writeTo;
										chunkedStream =
											eeNew( HttpStreamChunked, ( writeToStream ) );
									}

									bufferStream = chunked
													   ? chunkedStream
													   : ( compressed ? inflateStream : &writeTo );

									// Get the content length
									if ( !received.getField( "content-length" ).empty() ) {
										if ( !String::fromString(
												 contentLength,
												 received.getField( "content-length" ) ) )
											contentLength = 0;
									}

									if ( received.getField( "connection" ) == "closed" ) {
										mConnection->setConnected( false );
										mConnection->setTunneled( false );
									}

									// If a redirection is requested, and requests follows
									// redirections, send a new request to the redirection location.
									if ( ( received.getStatus() == Response::MovedPermanently ||
										   received.getStatus() == Response::MovedTemporarily ) &&
										 request.getFollowRedirect() ) {

										// Only continue redirecting if less than 10 redirections
										// were done
										if ( request.mRedirectionCount <
											 request.getMaxRedirects() ) {
											std::string location( received.getField( "location" ) );
											URI uri( location );

											// Close the connection
											if ( !mConnection->isKeepAlive() )
												mConnection->disconnect();

											eeSAFE_DELETE( chunkedStream );
											eeSAFE_DELETE( inflateStream );

											Http::Request newRequest( request );
											newRequest.setUri( uri.getPathAndQuery() );

											request.mRedirectionCount++;
											newRequest.mRedirectionCount =
												request.mRedirectionCount;

											// Same host, expects a path in the same domain
											if ( uri.getHost().empty() ||
												 uri.getHost() == getHost() ) {
												return downloadRequest( newRequest, writeTo,
																		timeout );
											} else {
												// New host, we need to solve the host
												Http http( uri.getHost(), uri.getPort(),
														   uri.getScheme() == "https" ? true
																					  : false );
												return http.downloadRequest( newRequest, writeTo,
																			 timeout );
											}
										}
									}

									if ( !sendProgress( *this, request, received,
														Request::HeaderReceived, contentLength,
														0 ) ) {
										request.mCancel = true;
									}

									// Move the readBuffer to the starting point
									// of the file buffer
									if ( len > 0 ) {
										readBuffer = bol;
										readed = len;
									} else {
										readed = 0;
									}

									headerBuffer.clear();
								}
							}
						}

						if ( !isnheader ) {
							headerBuffer.append( readBuffer, ( bol - readBuffer ) );
						}
					}

					if ( isnheader ) {
						currentTotalBytes += readed;

						if ( readed > 0 )
							bufferStream->write( readBuffer, readed );

						if ( !sendProgress( *this, request, received, Request::ContentReceived,
											contentLength, currentTotalBytes ) ) {
							request.mCancel = true;
							break;
						}

						// If the response is compressed and the stream ended means that we received
						// the message. So we can skip the socket receive call.
						if ( ( compressed && NULL != inflateStream && !inflateStream->isOpen() ) ||
							 ( contentLength > 0 && contentLength == currentTotalBytes ) ) {
							break;
						}
					}
				}

				if ( chunked && NULL != chunkedStream &&
					 !chunkedStream->getHeaderBuffer().empty() ) {
					headerBuffer.append( chunkedStream->getHeaderBuffer() );
				}

				if ( !headerBuffer.empty() ) {
					std::istringstream in( headerBuffer );
					received.parseFields( in );
				}

				if ( status == Socket::Status::Disconnected ) {
					mConnection->setConnected( false );
					mConnection->setTunneled( false );
				}

				eeSAFE_DELETE( chunkedStream );
				eeSAFE_DELETE( inflateStream );
			} else {
				mConnection->setConnected( false );
				mConnection->setTunneled( false );
			}
		}

		// Close the connection
		if ( !mConnection->isKeepAlive() )
			mConnection->disconnect();
	}

	return received;
}

Http::Response Http::downloadRequest( const Http::Request& request, std::string writePath,
									  Time timeout ) {
	IOStreamFile file( writePath, request.isContinue() ? "ab+" : "wb+" );
	return downloadRequest( request, file, timeout );
}

void Http::setThreadPool( std::shared_ptr<ThreadPool> pool ) {
	sGlobalThreadPool = pool;
}

Http::AsyncRequest::AsyncRequest( Http* http, const Http::AsyncResponseCallback& cb,
								  Http::Request request, Time timeout ) :
	mHttp( http ),
	mCb( cb ),
	mRequest( request ),
	mTimeout( timeout ),
	mRunning( true ),
	mStreamed( false ),
	mStreamOwned( false ),
	mStream( NULL ) {}

Http::AsyncRequest::AsyncRequest( Http* http, const Http::AsyncResponseCallback& cb,
								  Http::Request request, IOStream& writeTo, Time timeout ) :
	mHttp( http ),
	mCb( cb ),
	mRequest( request ),
	mTimeout( timeout ),
	mRunning( true ),
	mStreamed( true ),
	mStreamOwned( false ),
	mStream( &writeTo ) {}

Http::AsyncRequest::AsyncRequest( Http* http, const Http::AsyncResponseCallback& cb,
								  Http::Request request, std::string writePath, Time timeout ) :
	mHttp( http ),
	mCb( cb ),
	mRequest( request ),
	mTimeout( timeout ),
	mRunning( true ),
	mStreamed( true ),
	mStreamOwned( true ),
	mStream( IOStreamFile::New( writePath, "wb" ) ) {}

Http::AsyncRequest::~AsyncRequest() {
	if ( mStreamOwned )
		eeSAFE_DELETE( mStream );
}

void Http::AsyncRequest::run() {
	Http::Response response = mStreamed ? mHttp->downloadRequest( mRequest, *mStream, mTimeout )
										: mHttp->sendRequest( mRequest, mTimeout );

	mCb( *mHttp, mRequest, response );

	if ( mStreamed && mStreamOwned ) {
		eeSAFE_DELETE( mStream );
	}

	// The Async Request destroys the socket used to create the request
	HttpConnection* connection = mHttp->mConnection;
	eeSAFE_DELETE( connection );
	mHttp->mConnection = NULL;

	mRunning = false;
}

void Http::removeOldThreads() {
	std::vector<AsyncRequest*> remove;

	for ( AsyncRequest* ar : mThreads ) {
		if ( ar->mRunning )
			continue;
		// We need to be sure, since the state is set in the thread, this will not block the
		// thread anyway
		ar->wait();

		eeDelete( ar );

		remove.push_back( ar );
	}

	for ( auto rem : remove ) {
		auto found = std::find( mThreads.begin(), mThreads.end(), rem );
		if ( found != mThreads.end() )
			mThreads.erase( found );
	}
}

Http::Request Http::prepareFields( const Http::Request& request ) {
	Request toSend( request );

	if ( !toSend.hasField( "User-Agent" ) )
		toSend.setField( "User-Agent", "eepp-network" );

	if ( !toSend.hasField( "Accept" ) )
		toSend.setField( "Accept", "*/*" );

	if ( !toSend.hasField( "Host" ) )
		toSend.setField(
			"Host",
			mHostName + ( mPort != 80 && mPort != 443 ? ":" + String::toString( mPort ) : "" ) );

	if ( !toSend.hasField( "Content-Length" ) && toSend.mBody.size() > 0 ) {
		std::ostringstream out;
		out << toSend.mBody.size();
		toSend.setField( "Content-Length", out.str() );
	}

	if ( ( toSend.mMethod == Request::Post ) && !toSend.hasField( "Content-Type" ) )
		toSend.setField( "Content-Type", "application/x-www-form-urlencoded" );

	if ( ( toSend.mMajorVersion * 10 + toSend.mMinorVersion >= 11 ) &&
		 !toSend.hasField( "Connection" ) && ( !mConnection || !mConnection->isKeepAlive() ) ) {
		toSend.setField( "Connection", "close" );
	}

	if ( !mProxy.empty() ) {
		toSend.setField( "Accept", "*/*" );

		if ( mIsSSL ) {
			toSend.setField( "Proxy-connection", "keep-alive" );
		} else {
			toSend.setField( "Proxy-connection", "close" );
		}
	}

	if ( request.isCompressedResponse() )
		toSend.setField( "Accept-Encoding", "gzip, deflate" );

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

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
struct WGetAsyncRequest {
	Http* http;
	Http::Request request;
	Http::AsyncResponseCallback cb;
	IOStream* writeTo{ nullptr };
};

void emscripten_async_wget2_got_data( unsigned, void* vwget, void* buffer, unsigned bufferSize ) {
	WGetAsyncRequest* wget = reinterpret_cast<WGetAsyncRequest*>( vwget );
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
	delete wget;
}

void emscripten_async_wget2_got_file( unsigned int, void* vwget, const char* ) {
	WGetAsyncRequest* wget = reinterpret_cast<WGetAsyncRequest*>( vwget );
	Http::Response::Status status = Http::Response::Status::Ok;
	Http::Response response =
		Http::Response::createFakeResponse( Http::Response::FieldTable(), status, "" );
	wget->cb( *wget->http, wget->request, response );
	delete wget;
}

void emscripten_async_wget2_got_error_data( unsigned, void* vwget, int errorCode,
											const char* errorDescription ) {
	WGetAsyncRequest* wget = reinterpret_cast<WGetAsyncRequest*>( vwget );
	std::string responseBody;
	Http::Response::Status status = Http::Response::intAsStatus( errorCode );
	Http::Response response =
		Http::Response::createFakeResponse( Http::Response::FieldTable(), status, responseBody );
	wget->cb( *wget->http, wget->request, response );
	delete wget;
}

void emscripten_async_wget2_got_error_file( unsigned int, void* vwget, int errorCode ) {
	WGetAsyncRequest* wget = reinterpret_cast<WGetAsyncRequest*>( vwget );
	std::string responseBody;
	Http::Response::Status status = Http::Response::intAsStatus( errorCode );
	Http::Response response =
		Http::Response::createFakeResponse( Http::Response::FieldTable(), status, responseBody );
	wget->cb( *wget->http, wget->request, response );
	delete wget;
}
#endif

void Http::sendAsyncRequest( const Http::AsyncResponseCallback& cb, const Http::Request& request,
							 Time timeout ) {
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	WGetAsyncRequest* wget = new WGetAsyncRequest();
	wget->http = this;
	wget->cb = cb;
	wget->request = Http::Request( request );
	emscripten_async_wget2_data( ( getURI().toString() + request.getUri() ).c_str(),
								 Request::methodToString( request.getMethod() ).c_str(),
								 URI( request.getUri() ).getQuery().c_str(), wget, 1,
								 emscripten_async_wget2_got_data,
								 emscripten_async_wget2_got_error_data, NULL );
#else
	if ( sGlobalThreadPool ) {
		sGlobalThreadPool->run( [this, cb, request, timeout] {
			AsyncRequest asyncRequest( this, cb, request, timeout );
			asyncRequest.run();
		} );
		return;
	}
	AsyncRequest* thread = eeNew( AsyncRequest, ( this, cb, request, timeout ) );
	thread->launch();
	Lock l( mThreadsMutex );
	removeOldThreads();
	mThreads.push_back( thread );
#endif
}

void Http::downloadAsyncRequest( const Http::AsyncResponseCallback& cb,
								 const Http::Request& request, IOStream& writeTo, Time timeout ) {
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	WGetAsyncRequest* wget = new WGetAsyncRequest();
	wget->http = this;
	wget->cb = cb;
	wget->writeTo = &writeTo;
	wget->request = Http::Request( request );
	emscripten_async_wget2_data( ( getURI().toString() + request.getUri() ).c_str(),
								 Request::methodToString( request.getMethod() ).c_str(),
								 URI( request.getUri() ).getQuery().c_str(), wget, 1,
								 emscripten_async_wget2_got_data,
								 emscripten_async_wget2_got_error_data, NULL );
#else
	if ( sGlobalThreadPool ) {
		sGlobalThreadPool->run( [this, cb, request, &writeTo, timeout] {
			AsyncRequest asyncRequest( this, cb, request, writeTo, timeout );
			asyncRequest.run();
		} );
		return;
	}
	AsyncRequest* thread = eeNew( AsyncRequest, ( this, cb, request, writeTo, timeout ) );
	thread->launch();
	Lock l( mThreadsMutex );
	removeOldThreads();
	mThreads.push_back( thread );
#endif
}

void Http::downloadAsyncRequest( const Http::AsyncResponseCallback& cb,
								 const Http::Request& request, std::string writePath,
								 Time timeout ) {
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	WGetAsyncRequest* wget = new WGetAsyncRequest();
	wget->http = this;
	wget->cb = cb;
	wget->request = Http::Request( request );
	emscripten_async_wget2( ( getURI().toString() + request.getUri() ).c_str(), writePath.c_str(),
							Request::methodToString( request.getMethod() ).c_str(),
							URI( request.getUri() ).getQuery().c_str(), wget,
							emscripten_async_wget2_got_file, emscripten_async_wget2_got_error_file,
							NULL );
#else
	if ( sGlobalThreadPool ) {
		sGlobalThreadPool->run( [this, cb, request, writePath, timeout] {
			AsyncRequest asyncRequest( this, cb, request, writePath, timeout );
			asyncRequest.run();
		} );
		return;
	}
	AsyncRequest* thread = eeNew( AsyncRequest, ( this, cb, request, writePath, timeout ) );
	thread->launch();
	Lock l( mThreadsMutex );
	removeOldThreads();
	mThreads.push_back( thread );
#endif
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

Http::HttpConnection::HttpConnection() :
	mSocket( NULL ),
	mIsConnected( false ),
	mIsTunneled( false ),
	mIsSSL( false ),
	mIsKeepAlive( false ) {}

Http::HttpConnection::HttpConnection( TcpSocket* socket ) :
	mSocket( socket ), mIsConnected( false ), mIsTunneled( false ), mIsSSL( false ) {}

Http::HttpConnection::~HttpConnection() {
	eeSAFE_DELETE( mSocket );
}

void Http::HttpConnection::setSocket( TcpSocket* socket ) {
	mSocket = socket;
}

TcpSocket* Http::HttpConnection::getSocket() const {
	return mSocket;
}

void Http::HttpConnection::disconnect() {
	if ( NULL != mSocket )
		mSocket->disconnect();

	mIsConnected = false;
}

const bool& Http::HttpConnection::isConnected() const {
	return mIsConnected;
}

void Http::HttpConnection::setConnected( const bool& connected ) {
	mIsConnected = connected;
}

const bool& Http::HttpConnection::isTunneled() const {
	return mIsTunneled;
}

void Http::HttpConnection::setTunneled( const bool& tunneled ) {
	mIsTunneled = tunneled;
}

const bool& Http::HttpConnection::isSSL() const {
	return mIsSSL;
}

void Http::HttpConnection::setSSL( const bool& ssl ) {
	mIsSSL = ssl;
}

const bool& Http::HttpConnection::isKeepAlive() const {
	return mIsKeepAlive;
}

void Http::HttpConnection::setKeepAlive( const bool& isKeepAlive ) {
	mIsKeepAlive = isKeepAlive;
}

Http::Pool& Http::Pool::getGlobal() {
	return sGlobalHttpPool;
}

Http::Pool::Pool() {}

Http::Pool::~Pool() {
	clear();
}

void Http::Pool::clear() {
	Lock l( mMutex );
	for ( auto& connection : mHttps ) {
		Http* con = connection.second;

		eeSAFE_DELETE( con );
	}

	mHttps.clear();
}

std::string Http::Pool::getHostKey( const URI& host, const URI& proxy ) {
	return proxy.empty() ? host.getSchemeAndAuthority()
						 : String::format( "%s-%s", host.getSchemeAndAuthority().c_str(),
										   proxy.getSchemeAndAuthority().c_str() );
}

String::HashType Http::Pool::getHostHash( const URI& host, const URI& proxy ) {
	return String::hash( Http::Pool::getHostKey( host, proxy ) );
}

bool Http::Pool::exists( const URI& host, const URI& proxy ) {
	Lock l( mMutex );
	return mHttps.find( getHostHash( host, proxy ) ) != mHttps.end();
}

Http* Http::Pool::get( const URI& host, const URI& proxy ) {
	{
		Lock l( mMutex );
		auto hostInstance = mHttps.find( Http::Pool::getHostHash( host, proxy ) );

		if ( hostInstance != mHttps.end() ) {
			return hostInstance->second;
		}
	}

	Http* http = eeNew( Http, ( host.getHost(), host.getPort(), host.getScheme() == "https" ) );
	Lock l( mMutex );
	mHttps[getHostHash( host, proxy )] = http;
	return http;
}

static constexpr const char* TWO_HYPHENS = "--";
static constexpr const char* LINE_END = "\r\n";

Http::MultipartEntitiesBuilder::MultipartEntitiesBuilder() :
	MultipartEntitiesBuilder( "eepp-client-boundary-" +
							  String::toString( (Uint64)Sys::getSystemTime() ) ) {}

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
