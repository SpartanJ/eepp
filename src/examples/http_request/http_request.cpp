#include <eepp/ee.hpp>

void AsyncRequestCallback( const Http& http, Http::Request& request, Http::Response& response ) {
	std::cout << "Got response from request: " << http.GetHostName() << request.GetUri() << std::endl;

	if ( response.GetStatus() == Http::Response::Ok ) {
		std::cout << response.GetBody() << std::endl;
	} else {
		std::cout << "Error " << response.GetStatus() << std::endl;
	}
}

EE_MAIN_FUNC int main (int argc, char * argv []) {
	{
		// Create a new HTTP client
		Http http;
		Http::Request request;

		if ( argc < 2 ) {
			// We'll work on http://en.wikipedia.org
			if ( SSLSocket::IsSupported() ) {
				http.SetHost("https://en.wikipedia.org");
			} else {
				http.SetHost("http://en.wikipedia.org");
			}

			// Prepare a request to get the wikipedia main page
			request.SetUri("/wiki/Main_Page");

			// Creates an async http request
			Http::Request asyncRequest( "/wiki/" + Version::GetCodename() );

			http.SendAsyncRequest( cb::Make3( AsyncRequestCallback ), asyncRequest, Seconds( 5 ) );
		} else {
			// If the user provided the URI, creates an instance of URI to parse it.
			URI uri( argv[1] );

			// Set the host and port from the URI
			http.SetHost( uri.GetHost(), uri.GetPort() );

			// Set the path and query parts for the request
			request.SetUri( uri.GetPathAndQuery() );
		}

		// Send the request
		Http::Response response = http.SendRequest(request);

		// Check the status code and display the result
		Http::Response::Status status = response.GetStatus();

		if ( status == Http::Response::Ok ) {
			std::cout << response.GetBody() << std::endl;
		} else {
			std::cout << "Error " << status << std::endl;
		}
	}

	MemoryManager::ShowResults();

	return EXIT_SUCCESS;
}
