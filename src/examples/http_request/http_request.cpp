#include <eepp/ee.hpp>

/// Entry point of application
EE_MAIN_FUNC int main (int argc, char * argv [])
{
	// Create a new HTTP client
	cHttp http;

	// We'll work on http://www.wikipedia.org
	http.SetHost("http://www.wikipedia.org");

	// Prepare a request to get the '/' page
	cHttp::Request request("/");

	// Send the request
	cHttp::Response response = http.SendRequest(request);

	// Check the status code and display the result
	cHttp::Response::Status status = response.GetStatus();

	if ( status == cHttp::Response::Ok ) {
		std::cout << response.GetBody() << std::endl;
	} else {
		std::cout << "Error " << status << std::endl;
	}

	return EXIT_SUCCESS;
}
