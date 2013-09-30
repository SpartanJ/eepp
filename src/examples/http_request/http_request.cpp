#include <eepp/ee.hpp>

void AsyncRequestCallback( const cHttp& http, cHttp::Request& request, cHttp::Response& response ) {
	std::cout << "Got response from request: " << http.GetHostName() << request.GetUri() << std::endl;

	if ( response.GetStatus() == cHttp::Response::Ok ) {
		std::cout << response.GetBody() << std::endl;
	} else {
		std::cout << "Error " << response.GetStatus() << std::endl;
	}
}

EE_MAIN_FUNC int main (int argc, char * argv []) {
	{
		// Create a new HTTP client
		cHttp http;

		// We'll work on http://en.wikipedia.org
		http.SetHost("http://en.wikipedia.org");

		// Prepare a request to get the '/' page
		cHttp::Request request("/wiki/Main_Page");

		// Send the request
		cHttp::Response response = http.SendRequest(request);

		// Check the status code and display the result
		cHttp::Response::Status status = response.GetStatus();

		if ( status == cHttp::Response::Ok ) {
			std::cout << response.GetBody() << std::endl;
		} else {
			std::cout << "Error " << status << std::endl;
		}

		cHttp::Request asyncRequest( "/wiki/" + Version::GetCodename() );

		http.SendAsyncRequest( cb::Make3( AsyncRequestCallback ), asyncRequest, Seconds( 5 ) );
	}

	MemoryManager::ShowResults();

	return EXIT_SUCCESS;
}
