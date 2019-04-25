#include <eepp/ee.hpp>
#include <args/args.hxx>

EE_MAIN_FUNC int main (int argc, char * argv []) {
	args::ArgumentParser parser("HTTP request program example");
	args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
	args::ValueFlag<std::string> output(parser, "file",  "Write to file instead of stdout", {'o', "output"} );
	args::Positional<std::string> url(parser, "url", "The url to request");
	args::Flag verbose(parser, "verbose", "Prints the request response headers", {'v',"verbose"} );

	try {
		parser.ParseCLI(argc, argv);
	} catch (const args::Help&) {
		std::cout << parser;
		return 0;
	} catch (const args::ParseError& e) {
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return 1;
	} catch (args::ValidationError& e) {
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return 1;
	}

	{
		// Create a new HTTP client
		Http http;
		Http::Request request;

		if ( !url ) {
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
			URI uri( url.Get() );

			if ( uri.getScheme().empty() ) {
				uri = URI( "http://" + url.Get() );
			}

			// Set the host and port from the URI
			http.setHost( uri.getHost(), uri.getPort() );

			// Set the path and query parts for the request
			request.setUri( uri.getPathEtc() );

			if ( !output ) {
				// Send the request
				Http::Response response = http.sendRequest(request);

				// Check the status code and display the result
				Http::Response::Status status = response.getStatus();

				if ( status == Http::Response::Ok ) {
					if ( verbose ) {
						Http::Response::FieldTable headers = response.getHeaders();

						std::cout << "Headers: " << std::endl;

						for ( auto&& head : headers ) {
							std::cout << "\t" << head.first << ": " << head.second << std::endl;
						}

						std::cout << std::endl << "Body: " << std::endl;
					}

					std::cout << response.getBody() << std::endl;
				} else {
					std::cout << "Error " << status << std::endl;
				}
			} else {
				http.downloadRequest(request, output.Get(), Seconds(5));
			}
		}
	}

	if ( verbose )
		MemoryManager::showResults();

	return EXIT_SUCCESS;
}
