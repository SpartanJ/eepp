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

		// We'll work on http://en.wikipedia.org
		if ( SSLSocket::IsSupported() ) {
			http.SetHost("https://en.wikipedia.org");
		} else {
			http.SetHost("http://en.wikipedia.org");
		}

		// Prepare a request to get the wikipedia main page
		Http::Request request("/wiki/Main_Page");

		// Send the request
		Http::Response response = http.SendRequest(request);

		// Check the status code and display the result
		Http::Response::Status status = response.GetStatus();

		if ( status == Http::Response::Ok ) {
			std::cout << response.GetBody() << std::endl;
		} else {
			std::cout << "Error " << status << std::endl;
		}

		Http::Request asyncRequest( "/wiki/" + Version::GetCodename() );

		http.SendAsyncRequest( cb::Make3( AsyncRequestCallback ), asyncRequest, Seconds( 5 ) );
	}

	MemoryManager::ShowResults();

	return EXIT_SUCCESS;
}
