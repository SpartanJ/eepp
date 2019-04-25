#include <eepp/ee.hpp>
#include <args/args.hxx>

void printResponseHeaders( Http::Response& response ) {
	Http::Response::FieldTable headers = response.getHeaders();

	std::cout << "\r\nHeaders: " << std::endl;

	for ( auto&& head : headers ) {
		std::cout << "\t" << head.first << ": " << head.second << std::endl;
	}
}

EE_MAIN_FUNC int main (int argc, char * argv []) {
	args::ArgumentParser parser("HTTP request program example");
	args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
	args::ValueFlag<std::string> output(parser, "file",  "Write to file instead of stdout", {'o', "output"} );
	args::Flag head(parser, "head", "Show document info", {'I',"head"} );
	args::Flag progress(parser, "progress", "Show current progress of a download", {'p',"progress"} );
	args::Positional<std::string> url(parser, "url", "The url to request");

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
					if ( head ) {
						printResponseHeaders(response);

						std::cout << std::endl << "Body: " << std::endl;
					}

					std::cout << response.getBody() << std::endl;
				} else {
					std::cout << "Error " << status << std::endl;
				}
			} else {
				if ( progress ) {
					request.setProgressCallback( []( const Http& http, const Http::Request& request, size_t totalBytes, size_t currentBytes ) {
						std::cout << "\rDownloaded " << FileSystem::sizeToString( currentBytes ).c_str() << " of " << FileSystem::sizeToString( totalBytes ).c_str() << "          ";
						std::cout << std::flush;
						return true;
					});
				}

				Http::Response response = http.downloadRequest(request, output.Get(), Seconds(5));

				if ( head )
					printResponseHeaders(response);
			}
		}
	}

	if ( head )
		MemoryManager::showResults();

	return EXIT_SUCCESS;
}
