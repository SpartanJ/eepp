#ifndef EE_NETWORKCHTTP_HPP
#define EE_NETWORKCHTTP_HPP

#include <eepp/network/base.hpp>
#include <eepp/network/ipaddress.hpp>
#include <eepp/network/tcpsocket.hpp>
#include <eepp/core/noncopyable.hpp>
#include <eepp/system/time.hpp>
#include <eepp/system/threadlocalptr.hpp>
#include <eepp/system/thread.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/network/uri.hpp>
#include <map>
#include <string>
#include <list>

namespace EE { namespace System {
class IOStream;
}}

using namespace EE::System;

namespace EE { namespace Network {

/** @brief A HTTP client */
class EE_API Http : NonCopyable {
	public :
		/** @brief Define a HTTP request */
		class EE_API Request {
			public :
			/** @brief Enumerate the available HTTP methods for a request */
			enum Method {
				Get,     ///< The GET method requests a representation of the specified resource. Requests using GET should only retrieve data.
				Head,    ///< Request a page's header only
				Post,    ///< The POST method is used to submit an entity to the specified resource, often causing a change in state or side effects on the server.
				Put,     ///< The PUT method replaces all current representations of the target resource with the request payload.
				Delete,  ///< The DELETE method deletes the specified resource.
				Options, ///< The OPTIONS method is used to describe the communication options for the target resource.
				Patch,   ///< The PATCH method is used to apply partial modifications to a resource.
				Connect  ///< The CONNECT method starts two-way communications with the requested resource. It can be used to open a tunnel.
			};

			/** @return Method from a method name string. */
			static Method methodFromString( std::string methodString );

			/** @return The method string from a method */
			static std::string methodToString( const Method& method );

			/** @brief Default constructor
			**  This constructor creates a GET request, with the root
			**  URI ("/") and an empty body.
			**  @param uri	Target URI
			**  @param method Method to use for the request
			**  @param body   Content of the request's body
			**  @param validateCertificate Enables certificate validation for https request
			**  @param validateHostname Enables hostname validation for https request
			**  @param followRedirect Allow follor redirects to the request.
			**  @param compressedResponse Set if the requested response should be compressed ( if available )
			*/
			Request(const std::string& uri = "/", Method method = Get, const std::string& body = "", bool validateCertificate = true, bool validateHostname = true, bool followRedirect = true, bool compressedResponse = false);

			/** @brief Set the value of a field
			**  The field is created if it doesn't exist. The name of
			**  the field is case insensitive.
			**  By default, a request doesn't contain any field (but the
			**  mandatory fields are added later by the HTTP client when
			**  sending the request).
			**  @param field Name of the field to set
			**  @param value Value of the field */
			void setField(const std::string& field, const std::string& value);

			/** @brief Check if the request defines a field
			**  This function uses case-insensitive comparisons.
			**  @param field Name of the field to test
			**  @return True if the field exists, false otherwise */
			bool hasField(const std::string& field) const;

			/** @brief Get the value of a field
			**  If the field @a field is not found in the response header,
			**  the empty string is returned. This function uses
			**  case-insensitive comparisons.
			**  @param field Name of the field to get
			**  @return Value of the field, or empty string if not found */
			const std::string& getField(const std::string& field) const;

			/** @brief Set the request method
			**  See the Method enumeration for a complete list of all
			**  the availale methods.
			**  The method is Http::Request::Get by default.
			**  @param method Method to use for the request */
			void setMethod(Method method);

			/** @brief Set the requested URI
			**  The URI is the resource (usually a web page or a file)
			**  that you want to get or post.
			**  The URI is "/" (the root page) by default.
			**  @param uri URI to request, relative to the host */
			void setUri(const std::string& uri);

			/** @brief Set the HTTP version for the request
			**  The HTTP version is 1.0 by default.
			**  @param major Major HTTP version number
			**  @param minor Minor HTTP version number */
			void setHttpVersion(unsigned int major, unsigned int minor);

			/** @brief Set the body of the request
			**  The body of a request is optional and only makes sense
			**  for POST requests. It is ignored for all other methods.
			**  The body is empty by default.
			**  @param body Content of the body */
			void setBody(const std::string& body);

			/** @return The request Uri */
			const std::string& getUri() const;

			/** @return If SSL certificate validation is enabled */
			const bool& getValidateCertificate() const;

			/** Enable/disable SSL certificate validation */
			void setValidateCertificate( bool enable );

			/** @return If SSL hostname validation is enabled */
			const bool& getValidateHostname() const;

			/** Enable/disable SSL hostname validation */
			void setValidateHostname( bool enable );

			/** @return If requests follow redirects */
			const bool& getFollowRedirect() const;

			/** Enables/Disables follow redirects */
			void setFollowRedirect( bool follow );

			/** @return The maximun number of redirects allowd if follow redirect is enabled. */
			const unsigned int& getMaxRedirects() const;

			/** Set the maximun number of redirects allowed if follow redirect is enabled. */
			void setMaxRedirects( unsigned int maxRedirects );

			/** Definition of the current progress callback
			 * @param http The http client
			 * @param request The http request
			 * @param totalBytes The total bytes of the document / files ( only available if Content-Length is returned, otherwise is 0 )
			 * @param currentBytes Current received total bytes
			 * @return True if continue the request, false will cancel the current request.
			*/
			typedef std::function<bool( const Http& http, const Http::Request& request, std::size_t totalBytes, std::size_t currentBytes )>		ProgressCallback;

			/** Sets a progress callback */
			void setProgressCallback( const ProgressCallback& progressCallback );

			/** Get the progress callback */
			const ProgressCallback& getProgressCallback() const;

			/** Cancels the current request if being processed */
			void cancel();

			/** @return True if the current request was cancelled */
			const bool& isCancelled() const;

			/** @return If requests a compressed response */
			const bool& isCompressedResponse() const;

			/** Set to request a compressed response from the server
			**  The returned response will be automatically decompressed
			**  by the client.
			*/
			void setCompressedResponse(const bool& compressedResponse);

			private:
			friend class Http;

			/** @brief Prepare the final request to send to the server
			**  This is used internally by Http before sending the
			**  request to the web server.
			**  @return String containing the request, ready to be sent */
			std::string prepare(const Http& http) const;

			/** Prepares a http tunnel request */
			std::string prepareTunnel(const Http& http);

			// Types
			typedef std::map<std::string, std::string> FieldTable;

			// Member data
			FieldTable            mFields;              ///< Fields of the header associated to their value
			Method                mMethod;              ///< Method to use for the request
			std::string           mUri;                 ///< Target URI of the request
			unsigned int          mMajorVersion;        ///< Major HTTP version
			unsigned int          mMinorVersion;        ///< Minor HTTP version
			std::string           mBody;                ///< Body of the request
			bool                  mValidateCertificate; ///< Validates the SSL certificate in case of an HTTPS request
			bool                  mValidateHostname;    ///< Validates the hostname in case of an HTTPS request
			bool                  mFollowRedirect;      ///< Follows redirect response codes
			bool                  mCompressedResponse;  ///< Request comrpessed response
			mutable bool          mCancel;              ///< Cancel state of current request
			ProgressCallback      mProgressCallback;    ///< Progress callback
			unsigned int          mMaxRedirections;     ///< Maximun number of redirections allowed
			mutable unsigned int  mRedirectionCount;    ///< Number of redirections followed by the request
			URI                   mProxy;               ///< Proxy information
		};

		/** @brief Define a HTTP response */
		class EE_API Response {
			public:

			// Types
			typedef std::map<std::string, std::string> FieldTable;

			/** @brief Enumerate all the valid status codes for a response */
			enum Status {
				// 2xx: success
				Ok					= 200, ///< Most common code returned when operation was successful
				Created				= 201, ///< The resource has successfully been created
				Accepted			= 202, ///< The request has been accepted, but will be processed later by the server
				NoContent			= 204, ///< The server didn't send any data in return
				ResetContent		= 205, ///< The server informs the client that it should clear the view (form) that caused the request to be sent
				PartialContent		= 206, ///< The server has sent a part of the resource, as a response to a partial GET request

				// 3xx: redirection
				MultipleChoices		= 300, ///< The requested page can be accessed from several locations
				MovedPermanently	= 301, ///< The requested page has permanently moved to a new location
				MovedTemporarily	= 302, ///< The requested page has temporarily moved to a new location
				NotModified			= 304, ///< For conditionnal requests, means the requested page hasn't changed and doesn't need to be refreshed

				// 4xx: client error
				BadRequest			= 400, ///< The server couldn't understand the request (syntax error)
				Unauthorized		= 401, ///< The requested page needs an authentification to be accessed
				Forbidden			= 403, ///< The requested page cannot be accessed at all, even with authentification
				NotFound			= 404, ///< The requested page doesn't exist
				RangeNotSatisfiable	= 407, ///< The server can't satisfy the partial GET request (with a "Range" header field)

				// 5xx: server error
				InternalServerError	= 500, ///< The server encountered an unexpected error
				NotImplemented		= 501, ///< The server doesn't implement a requested feature
				BadGateway			= 502, ///< The gateway server has received an error from the source server
				ServiceNotAvailable	= 503, ///< The server is temporarily unavailable (overloaded, in maintenance, ...)
				GatewayTimeout		= 504, ///< The gateway server couldn't receive a response from the source server
				VersionNotSupported	= 505, ///< The server doesn't support the requested HTTP version

				// 10xx: Custom codes
				InvalidResponse		= 1000, ///< Response is not a valid HTTP one
				ConnectionFailed	= 1001  ///< Connection with server failed
			};

			/** @return The status string */
			static const char * statusToString( const Status& status );

			/** @brief Default constructor
			**  Constructs an empty response. */
			Response();

			FieldTable getHeaders();

			/** @brief Get the value of a field
			**  If the field @a field is not found in the response header,
			**  the empty string is returned. This function uses
			**  case-insensitive comparisons.
			**  @param field Name of the field to get
			**  @return Value of the field, or empty string if not found */
			const std::string& getField(const std::string& field) const;

			/** @brief Get the response status code
			**  The status code should be the first thing to be checked
			**  after receiving a response, it defines whether it is a
			**  success, a failure or anything else (see the Status
			**  enumeration).
			**  @return Status code of the response */
			Status getStatus() const;

			/** @brief Get the response status description */
			const char * getStatusDescription() const;

			/** @brief Get the major HTTP version number of the response
			**  @return Major HTTP version number
			**  @see GetMinorHttpVersion */
			unsigned int getMajorHttpVersion() const;

			/** @brief Get the minor HTTP version number of the response
			**  @return Minor HTTP version number
			**  @see GetMajorHttpVersion */
			unsigned int getMinorHttpVersion() const;

			/** @brief Get the body of the response
			**  The body of a response may contain:
			**  @li the requested page (for GET requests)
			**  @li a response from the server (for POST requests)
			**  @li nothing (for HEAD requests)
			**  @li an error message (in case of an error)
			**  @return The response body */
			const std::string& getBody() const;
			private :
			friend class Http;

			/** @brief Construct the header from a response string
			**  This function is used by Http to build the response
			**  of a request.
			**  @param data Content of the response to parse */
			void parse(const std::string& data);

			/** @brief Read values passed in the answer header
			**  This function is used by Http to extract values passed
			**  in the response.
			**  @param in String stream containing the header values */
			void parseFields(std::istream &in);

			// Member data
			FieldTable		mFields;		///< Fields of the header
			Status			mStatus;		///< Status code
			unsigned int	mMajorVersion;	///< Major HTTP version
			unsigned int	mMinorVersion;	///< Minor HTTP version
			std::string		mBody;			///< Body of the response
		};

		/** @brief Default constructor */
		Http();

		/** @brief Construct the HTTP client with the target host
		**  This is equivalent to calling setHost(host, port).
		**  The port has a default value of 0, which means that the
		**  HTTP client will use the right port according to the
		**  protocol used (80 for HTTP, 443 for HTTPS). You should
		**  leave it like this unless you really need a port other
		**  than the standard one, or use an unknown protocol.
		**  @param host Web server to connect to
		**  @param port Port to use for connection
		**  @param useSSL force the SSL usage ( if compiled with the support of it ). If the host starts with https:// it will use it by default.
		**  @param proxy Set an http proxy for the host connection
		*/
		Http(const std::string& host, unsigned short port = 0, bool useSSL = false, URI proxy = URI());

		~Http();

		/** @brief Set the target host
		**  This function just stores the host address and port, it
		**  doesn't actually connect to it until you send a request.
		**  The port has a default value of 0, which means that the
		**  HTTP client will use the right port according to the
		**  protocol used (80 for HTTP, 443 for HTTPS). You should
		**  leave it like this unless you really need a port other
		**  than the standard one, or use an unknown protocol.
		**  @param host Web server to connect to
		**  @param port Port to use for connection
		**	@param useSSL force the SSL usage ( if compiled with the support of it ). If the host starts with https:// it will use it by default.
		**	@param proxy Set an http proxy for the host connection
		*/
		void setHost(const std::string& host, unsigned short port = 0, bool useSSL = false, URI proxy = URI());

		/** @brief Send a HTTP request and return the server's response.
		**  You must have a valid host before sending a request (see setHost).
		**  Any missing mandatory header field in the request will be added
		**  with an appropriate value.
		**  Warning: this function waits for the server's response and may
		**  not return instantly; use a thread if you don't want to block your
		**  application, or use a timeout to limit the time to wait. A value
		**  of Time::Zero means that the client will use the system defaut timeout
		**  (which is usually pretty long).
		**  @param request Request to send
		**  @param timeout Maximum time to wait
		**  @return Server's response */
		Response sendRequest(const Request& request, Time timeout = Time::Zero);

		/** @brief Send a HTTP request and writes the server's response to a IOStream file.
		**  You must have a valid host before sending a request (see setHost).
		**  Any missing mandatory header field in the request will be added
		**  with an appropriate value.
		**  Warning: this function waits for the server's response and may
		**  not return instantly; use a thread if you don't want to block your
		**  application, or use a timeout to limit the time to wait. A value
		**  of Time::Zero means that the client will use the system defaut timeout
		**  (which is usually pretty long).
		**  @param request Request to send
		**  @param timeout Maximum time to wait
		**  @return Server's response */
		Response downloadRequest(const Request& request, IOStream& writeTo, Time timeout = Time::Zero);

		/** @brief Send a HTTP request and writes the server's response to a file system path.
		**  You must have a valid host before sending a request (see setHost).
		**  Any missing mandatory header field in the request will be added
		**  with an appropriate value.
		**  Warning: this function waits for the server's response and may
		**  not return instantly; use a thread if you don't want to block your
		**  application, or use a timeout to limit the time to wait. A value
		**  of Time::Zero means that the client will use the system defaut timeout
		**  (which is usually pretty long).
		**  @param request Request to send
		**  @param timeout Maximum time to wait
		**  @return Server's response */
		Response downloadRequest(const Request& request, std::string writePath, Time timeout = Time::Zero);

		/** Definition of the async callback response */
		typedef std::function<void( const Http&, Http::Request&, Http::Response& )>		AsyncResponseCallback;

		/** @brief Sends the request and creates a new thread, when got the response informs the result to the callback.
		**	This function does not lock the caller thread.
		**  @see sendRequest */
		void sendAsyncRequest( AsyncResponseCallback cb, const Http::Request& request, Time timeout = Time::Zero );

		/** @brief Sends the request and creates a new thread, when got the response informs the result to the callback.
		**	This function does not lock the caller thread.
		**  @see downloadRequest */
		void downloadAsyncRequest( AsyncResponseCallback cb, const Http::Request& request, IOStream& writeTo, Time timeout = Time::Zero );

		/** @brief Sends the request and creates a new thread, when got the response informs the result to the callback.
		**	This function does not lock the caller thread.
		**  @see downloadRequest */
		void downloadAsyncRequest( AsyncResponseCallback cb, const Http::Request& request, std::string writePath, Time timeout = Time::Zero );

		/** @return The host address */
		const IpAddress& getHost() const;

		/** @return The host name */
		const std::string& getHostName() const;

		/** @return The host port */
		const unsigned short& getPort() const;

		/** @return If the HTTP client uses SSL/TLS */
		const bool& isSSL() const;

		/** @return The URI from the schema + hostname + port */
		URI getURI() const;

		/** Sets the request proxy */
		void setProxy( const URI& uri );

		/** @return The request proxy */
		const URI& getProxy() const;

		/** @return Is a proxy is need to be used */
		bool isProxied() const;
	private:
		class AsyncRequest : public Thread {
			public:
				AsyncRequest( Http * http, AsyncResponseCallback cb, Http::Request request, Time timeout );

				AsyncRequest( Http * http, AsyncResponseCallback cb, Http::Request request, IOStream& writeTo, Time timeout );

				AsyncRequest( Http * http, AsyncResponseCallback cb, Http::Request request, std::string writePath, Time timeout );

				~AsyncRequest();

				void run();
			protected:
				friend class Http;
				Http *					mHttp;
				AsyncResponseCallback	mCb;
				Http::Request			mRequest;
				Time					mTimeout;
				bool					mRunning;
				bool					mStreamed;
				bool					mStreamOwned;
				IOStream *				mStream;
		};

		class HttpConnection {
			public:
				HttpConnection();

				HttpConnection( TcpSocket * socket );

				~HttpConnection();

				void setSocket( TcpSocket * socket );

				TcpSocket * getSocket() const;

				void disconnect();

				const bool& isConnected() const;

				void setConnected( const bool& connected );

				const bool& isTunneled() const;

				void setTunneled( const bool& tunneled );

				const bool& isSSL() const;

				void setSSL( const bool& ssl );

				const bool& isKeepAlive() const;

				void setKeepAlive( const bool& isKeepAlive );
			protected:
				TcpSocket * mSocket;
				bool mIsConnected;
				bool mIsTunneled;
				bool mIsSSL;
				bool mIsKeepAlive;
		};

		friend class AsyncRequest;
		ThreadLocalPtr<HttpConnection>	mConnection;	///< Connection to the host
		IpAddress						mHost;			///< Web host address
		std::string						mHostName;		///< Web host name
		unsigned short					mPort;			///< Port used for connection with host
		std::list<AsyncRequest*>		mThreads;
		Mutex							mThreadsMutex;
		bool							mIsSSL;
		URI								mProxy;

		void removeOldThreads();

		Request prepareFields(const Http::Request& request);
};

}}

#endif // EE_NETWORKCHTTP_HPP

/**
@class Http
@ingroup Network
Http is a very simple HTTP client that allows you
to communicate with a web server. You can retrieve
web pages, send data to an interactive resource,
download a remote file, etc.
The HTTP client is split into 3 classes:
@li Http::Request
@li Http::Response
@li Http
Http::Request builds the request that will be
sent to the server. A request is made of:
@li a method (what you want to do)
@li a target URI (usually the name of the web page or file)
@li one or more header fields (options that you can pass to the server)
@li an optional body (for POST requests)
Http::Response parse the response from the web server
and provides getters to read them. The response contains:
@li a status code
@li header fields (that may be answers to the ones that you requested)
@li a body, which contains the contents of the requested resource
Http provides a simple function, SendRequest, to send a
Http::Request and return the corresponding Http::Response
from the server.
Usage example:
@code
// Create a new HTTP client
Http http;

// We'll work on http://www.google.com
http.setHost("http://www.google.com");

// Prepare a request to get the 'features.php' page
Http::Request request("features.php");

// Send the request
Http::Response response = http.sendRequest(request);

// Check the status code and display the result
Http::Response::Status status = response.getStatus();
if (status == Http::Response::Ok)
{
	std::cout << response.getBody() << std::endl;
}
else
{
	std::cout << "Error " << status << std::endl;
}
@endcode
*/
