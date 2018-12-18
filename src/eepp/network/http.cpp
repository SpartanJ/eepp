#include <eepp/network/http.hpp>
#include <eepp/network/ssl/sslsocket.hpp>
#include <eepp/system/iostream.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <cctype>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <limits>

using namespace EE::Network::SSL;

namespace EE { namespace Network {

Http::Request::Request(const std::string& uri, Method method, const std::string& body, bool validateCertificate, bool validateHostname ) :
	mValidateCertificate( validateCertificate ),
	mValidateHostname( validateHostname )
{
	setMethod(method);
	setUri(uri);
	setHttpVersion(1, 0);
	setBody(body);
}

void Http::Request::setField(const std::string& field, const std::string& value) {
	mFields[String::toLower(field)] = value;
}

void Http::Request::setMethod(Http::Request::Method method) {
	mMethod = method;
}

void Http::Request::setUri(const std::string& uri) {
	mUri = uri;

	// Make sure it starts with a '/'
	if (mUri.empty() || (mUri[0] != '/'))
		mUri.insert(0, "/");
}

void Http::Request::setHttpVersion(unsigned int major, unsigned int minor) {
	mMajorVersion = major;
	mMinorVersion = minor;
}

void Http::Request::setBody(const std::string& body) {
	mBody = body;
}

const std::string &Http::Request::getUri() const {
	return mUri;
}

const bool& Http::Request::getValidateCertificate() const {
	return mValidateCertificate;
}

void Http::Request::setValidateCertificate(bool enable) {
	mValidateCertificate = enable;
}

const bool &Http::Request::getValidateHostname() const {
	return mValidateHostname;
}

void Http::Request::setValidateHostname(bool enable) {
	mValidateHostname = enable;
}

std::string Http::Request::prepare() const {
	std::ostringstream out;

	// Convert the method to its string representation
	std::string method;
	switch (mMethod) {
		default :
		case Get:		method = "GET";  break;
		case Post:		method = "POST"; break;
		case Head:		method = "HEAD"; break;
		case Put:		method = "PUT"; break;
		case Delete:	method = "DELETE"; break;
	}

	// Write the first line containing the request type
	out << method << " " << mUri << " ";
	out << "HTTP/" << mMajorVersion << "." << mMinorVersion << "\r\n";

	// Write fields
	for (FieldTable::const_iterator i = mFields.begin(); i != mFields.end(); ++i) {
		out << i->first << ": " << i->second << "\r\n";
	}

	// Use an extra \r\n to separate the header from the body
	out << "\r\n";

	// Add the body
	out << mBody;

	return out.str();
}

bool Http::Request::hasField(const std::string& field) const {
	return mFields.find(String::toLower(field)) != mFields.end();
}

Http::Response::Response() :
	mStatus	  (ConnectionFailed),
	mMajorVersion(0),
	mMinorVersion(0)
{
}

Http::Response::FieldTable Http::Response::getHeaders() {
	return mFields;
}

const std::string& Http::Response::getField(const std::string& field) const {
	FieldTable::const_iterator it = mFields.find(String::toLower(field));
	if (it != mFields.end()) {
		return it->second;
	} else {
		static const std::string empty = "";
		return empty;
	}
}

Http::Response::Status Http::Response::getStatus() const {
	return mStatus;
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

void Http::Response::parse(const std::string& data) {
	std::istringstream in(data);

	// Extract the HTTP version from the first line
	std::string version;

	if (in >> version) {
		std::locale loc;
		if ((version.size() >= 8) && (version[6] == '.') &&
			(String::toLower(version.substr(0, 5)) == "http/")   &&
			 std::isdigit(version[5],loc) && std::isdigit(version[7],loc)) {
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

	if (in >> status) {
		mStatus = static_cast<Status>(status);
	} else {
		// Invalid status code
		mStatus = InvalidResponse;
		return;
	}

	// Ignore the end of the first line
	in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Parse the other lines, which contain fields, one by one
	parseFields(in);

	// Finally extract the body
	mBody.clear();

	// Determine whether the transfer is chunked
	if (String::toLower(getField("transfer-encoding")) != "chunked") {
		// Not chunked - everything at once
		std::copy(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>(), std::back_inserter(mBody));
	} else {
		// Chunked - have to read chunk by chunk
		std::size_t length;

		// Read all chunks, identified by a chunk-size not being 0
		while (in >> std::hex >> length) {
			// Drop the rest of the line (chunk-extension)
			in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

			// Copy the actual content data
			std::istreambuf_iterator<char> it(in);
			for (std::size_t i = 0; i < length; i++)
				mBody.push_back(*it++);
		}

		// Drop the rest of the line (chunk-extension)
		in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Read all trailers (if present)
		parseFields(in);
	}
}

void Http::Response::parseFields(std::istream &in) {
	std::string line;
	while (std::getline(in, line) && (line.size() > 2)) {
		std::string::size_type pos = line.find(": ");

		if (pos != std::string::npos) {
			// Extract the field name and its value
			std::string field = line.substr(0, pos);
			std::string value = line.substr(pos + 2);

			// Remove any trailing \r
			if (!value.empty() && (*value.rbegin() == '\r'))
				value.erase(value.size() - 1);

			// Add the field
			mFields[String::toLower(field)] = value;
		}
	}
}

Http::Http() :
	mConnection( NULL ),
	mHost(),
	mPort(0),
	mIsSSL( false )
{
}

Http::Http(const std::string& host, unsigned short port, bool useSSL) :
	mConnection( NULL ),
	mIsSSL( false )
{
	setHost(host, port, useSSL);
}

Http::~Http() {
	std::list<AsyncRequest*>::iterator itt;

	// First we wait to finish any request pending
	for ( itt = mThreads.begin(); itt != mThreads.end(); ++itt ) {
		(*itt)->wait();
	}

	for ( itt = mThreads.begin(); itt != mThreads.end(); ++itt ) {
		eeDelete( *itt );
	}

	// Then we destroy the last open connection
	TcpSocket * tcp = mConnection;

	eeSAFE_DELETE( tcp );
}

void Http::setHost(const std::string& host, unsigned short port, bool useSSL) {
	// Check the protocol
	if (String::toLower(host.substr(0, 7)) == "http://") {
		// HTTP protocol
		mHostName = host.substr(7);
		mPort	 = (port != 0 ? port : 80);
	} else if (String::toLower(host.substr(0, 8)) == "https://") {
		// HTTPS protocol
		#ifdef EE_SSL_SUPPORT
		mIsSSL		= true;
		mHostName	= host.substr(8);
		mPort		= (port != 0 ? port : 443);
		#else
		mHostName	= "";
		mPort		= 0;
		#endif
	} else {
		// Undefined protocol - use HTTP, unless SSL is specified
		mHostName	= host;
		mPort		= (port != 0 ? port : 80);

		#ifdef EE_SSL_SUPPORT
		mPort		= useSSL ? (port != 0 ? port : 443) : mPort;
		mIsSSL		= useSSL || mPort == 443;
		#endif
	}

	// Remove any trailing '/' from the host name
	if (!mHostName.empty() && (*mHostName.rbegin() == '/'))
		mHostName.erase(mHostName.size() - 1);

	mHost = IpAddress(mHostName);
}

Http::Response Http::sendRequest(const Http::Request& request, Time timeout) {
	if ( 0 == mHost.toInteger() ) {
		return Response();
	}

	if ( NULL == mConnection ) {
		TcpSocket * Conn	= mIsSSL ? SSLSocket::New( mHostName, request.getValidateCertificate(), request.getValidateHostname() ) : TcpSocket::New();
		mConnection			= Conn;
	}

	// First make sure that the request is valid -- add missing mandatory fields
	Request toSend(prepareFields(request));

	// Prepare the response
	Response received;

	// Connect the socket to the host
	if (mConnection->connect(mHost, mPort, timeout) == Socket::Done) {
		// Convert the request to string and send it through the connected socket
		std::string requestStr = toSend.prepare();

		if (!requestStr.empty()) {
			// Send it through the socket
			if (mConnection->send(requestStr.c_str(), requestStr.size()) == Socket::Done) {
				// Wait for the server's response
				std::string receivedStr;
				std::size_t size = 0;
				char buffer[1024];

				while (mConnection->receive(buffer, sizeof(buffer), size) == Socket::Done) {
					receivedStr.append(buffer, buffer + size);
				}

				// Build the Response object from the received data
				received.parse(receivedStr);
			}
		}

		// Close the connection
		mConnection->disconnect();
	}

	return received;
}


Http::Response Http::downloadRequest(const Http::Request & request, IOStream & writeTo, Time timeout) {
	if ( 0 == mHost.toInteger() ) {
		return Response();
	}

	if ( NULL == mConnection ) {
		TcpSocket * Conn	= mIsSSL ? SSLSocket::New( mHostName, request.getValidateCertificate(), request.getValidateHostname() ) : TcpSocket::New();
		mConnection			= Conn;
	}

	Request toSend(prepareFields(request));
	Response received;

	if (mConnection->connect(mHost, mPort, timeout) == Socket::Done) {
		std::string requestStr = toSend.prepare();

		if (!requestStr.empty()) {
			if (mConnection->send(requestStr.c_str(), requestStr.size()) == Socket::Done) {
				int isnheader = 0;
				size_t len = 0;
				char * eol; // end of line
				char * bol; // beginning of line
				std::size_t size = 0;
				size_t bufferSize = 1024;
				char buffer[bufferSize+1];
				std::string header;

				while (mConnection->receive(buffer, bufferSize, size) == Socket::Done) {
					if ( isnheader != 0 )
						writeTo.write( buffer, size );

					if ( isnheader == 0 ) {
						// calculate combined length of unprocessed data and new data
						len += size;

						// NULL terminate buffer for string functions
						buffer[len] = '\0';

						// checks if the header break happened to be the first line of the buffer
						if ( !( strncmp( buffer, "\r\n", 2 ) ) ) {
							if (len > 2)
								writeTo.write(buffer, (len-2));

							continue;
						}

						if ( !( strncmp( buffer, "\n", 1 ) ) ) {
							if ( len > 1 )
								writeTo.write(buffer, (len-1));

							continue;
						}

						// process each line in buffer looking for header break
						bol = buffer;

						while( ( eol = strchr( bol, '\n') ) != NULL ) {
							// update bol based upon the value of eol
							bol = eol + 1;

							// test if end of headers has been reached
							if ( ( !( strncmp( bol, "\r\n", 2 ) ) ) || ( ! ( strncmp( bol, "\n", 1) ) ) ) {
								// note that end of headers has been reached
								isnheader = 1;

								// update the value of bol to reflect the beginning of the line
								// immediately after the headers
								if ( bol[0] != '\n' )
									bol += 1;

								bol += 1;

								// calculate the amount of data remaining in the buffer
								len = len - ( bol - buffer );

								// write remaining data to FILE stream
								if ( len > 0 )
									writeTo.write( bol, len );

								header.append( buffer, ( bol - buffer ) );

								// reset length of left over data to zero and continue processing
								// non-header information
								len = 0;
							}
						}

						if ( isnheader == 0 ) {
							header.append( buffer, ( bol - buffer ) );
						}
					}
				}

				if ( !header.empty() )
					received.parse(header);
			}
		}

		// Close the connection
		mConnection->disconnect();
	}

	return received;
}

Http::Response Http::downloadRequest(const Http::Request & request, std::string writePath, Time timeout) {
	IOStreamFile file( writePath, "wb" );
	return downloadRequest( request, file, timeout );
}

Http::AsyncRequest::AsyncRequest(Http *http, AsyncResponseCallback cb, Http::Request request, Time timeout) :
	mHttp( http ),
	mCb( cb ),
	mRequest( request ),
	mTimeout( timeout ),
	mRunning( true ),
	mStreamed( false ),
	mStreamOwned( false ),
	mStream(NULL)
{
}

Http::AsyncRequest::AsyncRequest(Http * http, Http::AsyncResponseCallback cb, Http::Request request, IOStream & writeTo, Time timeout) :
	mHttp( http ),
	mCb( cb ),
	mRequest( request ),
	mTimeout( timeout ),
	mRunning( true ),
	mStreamed( true ),
	mStreamOwned( false ),
	mStream( &writeTo )
{
}

Http::AsyncRequest::AsyncRequest(Http * http, Http::AsyncResponseCallback cb, Http::Request request, std::string writePath, Time timeout) :
	mHttp( http ),
	mCb( cb ),
	mRequest( request ),
	mTimeout( timeout ),
	mRunning( true ),
	mStreamed( true ),
	mStreamOwned( true ),
	mStream( IOStreamFile::New( writePath, "wb" ) )
{
}

Http::AsyncRequest::~AsyncRequest() {
	if ( mStreamOwned )
		eeSAFE_DELETE( mStream );
}

void Http::AsyncRequest::run() {
	Http::Response response = mStreamed ? mHttp->downloadRequest( mRequest, *mStream, mTimeout ) : mHttp->sendRequest( mRequest, mTimeout );

	mCb( *mHttp, mRequest, response );

	if ( mStreamed && mStreamOwned ) {
		eeSAFE_DELETE( mStream );
	}

	// The Async Request destroys the socket used to create the request
	TcpSocket * tcp = mHttp->mConnection;
	eeSAFE_DELETE( tcp );
	mHttp->mConnection = NULL;

	mRunning = false;
}

void Http::removeOldThreads() {
	std::list<AsyncRequest*> remove;

	std::list<AsyncRequest*>::iterator it = mThreads.begin();

	for ( ; it != mThreads.end(); ++it ) {
		AsyncRequest * ar = (*it);

		if ( !ar->mRunning ) {
			// We need to be sure, since the state is set in the thread, this will not block the thread anyway
			ar->wait();

			eeDelete( ar );

			remove.push_back( ar );
		}
	}

	for ( it = remove.begin(); it != remove.end(); ++it ) {
		mThreads.remove( (*it) );
	}
}

Http::Request Http::prepareFields(const Http::Request & request) {
	Request toSend(request);

	if (!toSend.hasField("From")) {
		toSend.setField("From", "user@eepp.com.ar");
	}

	if (!toSend.hasField("User-Agent")) {
		toSend.setField("User-Agent", "eepp-network");
	}

	if (!toSend.hasField("Host")) {
		toSend.setField("Host", mHostName);
	}

	if (!toSend.hasField("Content-Length")) {
		std::ostringstream out;
		out << toSend.mBody.size();
		toSend.setField("Content-Length", out.str());
	}

	if ((toSend.mMethod == Request::Post) && !toSend.hasField("Content-Type")) {
		toSend.setField("Content-Type", "application/x-www-form-urlencoded");
	}

	if ((toSend.mMajorVersion * 10 + toSend.mMinorVersion >= 11) && !toSend.hasField("Connection")) {
		toSend.setField("Connection", "close");
	}

	return toSend;
}

void Http::sendAsyncRequest( AsyncResponseCallback cb, const Http::Request& request, Time timeout ) {
	AsyncRequest * thread = eeNew( AsyncRequest, ( this, cb, request, timeout ) );

	thread->launch();

	// Clean old threads
	Lock l( mThreadsMutex );

	removeOldThreads();

	mThreads.push_back( thread );
}

void Http::downloadAsyncRequest(Http::AsyncResponseCallback cb, const Http::Request & request, IOStream & writeTo, Time timeout) {
	AsyncRequest * thread = eeNew( AsyncRequest, ( this, cb, request, timeout ) );

	thread->launch();

	// Clean old threads
	Lock l( mThreadsMutex );

	removeOldThreads();

	mThreads.push_back( thread );
}

const IpAddress &Http::getHost() const {
	return mHost;
}

const std::string &Http::getHostName() const {
	return mHostName;
}

const unsigned short& Http::getPort() const {
	return mPort;
}

}}
