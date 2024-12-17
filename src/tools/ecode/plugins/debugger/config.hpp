#pragma once

#include <eepp/core/containers.hpp>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

using namespace std::literals;

using json = nlohmann::json;

using namespace EE;

namespace ecode {

struct Command {
	std::string command;
	std::vector<std::string> arguments;
	std::unordered_map<std::string, std::string> environment;
};

struct Connection {
	int port;
	std::string host;
};

struct BusSettings {
	std::optional<Command> command;
	std::optional<Connection> connection;

	bool isValid() const;
	bool hasCommand() const;
	bool hasConnection() const;
};

static const auto REQUEST = "request"sv;
static const auto REDIRECT_STDERR = "redirectStderr"sv;
static const auto REDIRECT_STDOUT = "redirectStdout"sv;

struct ProtocolSettings {
	bool linesStartAt1;
	bool columnsStartAt1;
	bool pathFormatURI;
	bool redirectStderr;
	bool redirectStdout;
	bool supportsSourceRequest;
	json launchRequest;
	std::string locale;

	ProtocolSettings() :
		linesStartAt1( true ),
		columnsStartAt1( true ),
		pathFormatURI( false ),
		redirectStderr( false ),
		redirectStdout( false ),
		supportsSourceRequest( true ),
		locale( "en-US" ) {}

	ProtocolSettings( const nlohmann::json& configuration ) :
		linesStartAt1( true ),
		columnsStartAt1( true ),
		pathFormatURI( false ),
		redirectStderr( configuration.value( REDIRECT_STDERR, false ) ),
		redirectStdout( configuration.value( REDIRECT_STDOUT, false ) ),
		supportsSourceRequest( configuration.value( "supportsSourceRequest", true ) ),
		launchRequest( configuration[REQUEST] ),
		locale( "en-US" ) {}
};

} // namespace ecode
