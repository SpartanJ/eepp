#include <eepp/ee.hpp>

EE_MAIN_FUNC int main (int argc, char * argv []) {
	{
		// Create a new HTTP client
		Http http;
		Http::Request request;

		if ( argc < 2 ) {
			// We'll work on en.wikipedia.org
			if ( SSLSocket::isSupported() ) {
				http.setHost("https://en.wikipedia.org");
			} else {
				http.setHost("http://en.wikipedia.org");
			}

			// Prepare a request to get the wikipedia main page
			request.setUri("/wiki/Main_Page");

			// Creates an async http request
			Http::Request asyncRequest( "/wiki/" + Version::getCodename() );

			http.sendAsyncRequest([]( const Http& http, Http::Request& request, Http::Response& response ) {
				std::cout << "Got response from request: " << http.getHostName() << request.getUri() << std::endl;

				if ( response.getStatus() == Http::Response::Ok ) {
					std::cout << response.getBody() << std::endl;
				} else {
					std::cout << "Error " << response.getStatus() << std::endl;
				}
			}, asyncRequest, Seconds( 5 ) );
		} else {
			// If the user provided the URI, creates an instance of URI to parse it.
			URI uri( argv[1] );

			// Set the host and port from the URI
			http.setHost( uri.getHost(), uri.getPort() );

			// Set the path and query parts for the request
			request.setUri( uri.getPathEtc() );

			// Send the request
			Http::Response response = http.sendRequest(request);

			// Check the status code and display the result
			Http::Response::Status status = response.getStatus();

			if ( status == Http::Response::Ok ) {
				Http::Response::FieldTable headers = response.getHeaders();

				std::cout << "Headers: " << std::endl;

				for ( auto head = headers.begin(); head != headers.end(); ++head ) {
					std::cout << "\t" << head->first << ": " << head->second << std::endl;
				}

				std::cout << std::endl << "Body: " << std::endl;

				std::cout << response.getBody() << std::endl;
			} else {
				std::cout << "Error " << status << std::endl;
			}
		}
	}

	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
