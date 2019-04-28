#include <eepp/ee.hpp>
#include <args/args.hxx>

// Prints the response headers
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
	args::ValueFlag<std::string> postData(parser, "data", "HTTP POST data", {'d', "data"});
	args::ValueFlagList<std::string> headers(parser, "header", "Pass custom header(s) to server", {'H', "header"});
	args::Flag head(parser, "head", "Show document info", {'I',"head"});
	args::Flag insecure(parser, "insecure", "Allow insecure server connections when using SSL", {'k',"insecure"});
	args::Flag location(parser, "location", "Follow redirects", {'L',"location"});
	args::ValueFlag<unsigned int> maxRedirs(parser, "max-redirs",  "Maximum number of redirects allowed", {"max-redirs"});
	args::ValueFlag<std::string> output(parser, "file",  "Write to file instead of stdout", {'o', "output"});
	args::Flag progress(parser, "progress", "Show current progress of a download", {'p',"progress"});
	args::ValueFlag<std::string> requestMethod(parser, "request", "Specify request command to use", {'X', "request"});
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
			// If the user provided the URL, creates an instance of URI to parse it.
			URI uri( url.Get() );

			// If no scheme provided asume HTTP
			if ( uri.getScheme().empty() ) {
				uri = URI( "http://" + url.Get() );
			}

			// Allow insecure connections if requested
			if ( insecure ) {
				request.setValidateCertificate(false);
				request.setValidateHostname(false);
			}

			// Set the host and port from the URI
			http.setHost( uri.getHost(), uri.getPort() );

			// Set the path and query parts for the request
			request.setUri( uri.getPathEtc() );

			// Set the headers
			for ( const std::string& header : args::get(headers) ) {
				std::string::size_type pos = header.find_first_of( ':' );
				if ( std::string::npos != pos ) {
					std::string key( header.substr( 0, pos ) );
					std::string val( String::trim( header.substr( pos + 1 ) ) );
					request.setField(key, val);
				}
			}

			// Set the request method
			if ( requestMethod ) {
				request.setMethod( Http::Request::methodFromString( requestMethod.Get() ) );
			}

			// Set the post data / body
			if ( postData ) {
				request.setBody( postData.Get() );
			}

			// If progress requested print a progress on screen
			if ( progress ) {
				request.setProgressCallback( []( const Http&, const Http::Request&, size_t totalBytes, size_t currentBytes ) {
					std::cout << "\rDownloaded " << FileSystem::sizeToString( currentBytes ).c_str() << " of " << FileSystem::sizeToString( totalBytes ).c_str() << "          ";
					std::cout << std::flush;
					return true;
				});
			}

			// Set follow redirect
			request.setFollowRedirect(location.Get());

			// Set the maximun number of redirects
			if ( maxRedirs ) {
				request.setMaxRedirects(maxRedirs.Get());
			}

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
					std::cout << "Error " << status << std::endl << response.getStatusDescription() << std::endl;
				}
			} else {
				std::string path( output.Get() );

				// If output path is a directory guess a file name
				if ( FileSystem::isDirectory( path ) ) {
					std::string lastPathSegment = uri.getLastPathSegment();

					// If there's a path end segment
					if ( !lastPathSegment.empty() ) {
						FileSystem::dirPathAddSlashAtEnd( path );

						// Save with the path end segment name
						if ( !FileSystem::fileExists( path + lastPathSegment ) ) {
							path += lastPathSegment;
						} else {
							path += FileSystem::fileGetNumberedFileNameFromPath( path, lastPathSegment );
						}
					} else {
						// Create a file name if no name found
						path += FileSystem::fileGetNumberedFileNameFromPath( path, "eepp-network-file", "-" );
					}
				}

				// Download the request response into a file
				Http::Response response = http.downloadRequest(request, path, Seconds(5));

				if ( head )
					printResponseHeaders(response);
			}
		}
	}

	if ( head )
		MemoryManager::showResults();

	return EXIT_SUCCESS;
}
