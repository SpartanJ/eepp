#include "config.hpp"

namespace ecode {

bool Command::isValid() const {
	return !command.empty();
}

bool Connection::isValid() const {
	return ( port > 0 ) && !host.empty();
}

bool BusSettings::isValid() const {
	return hasCommand() || hasConnection();
}

bool BusSettings::hasCommand() const {
	return command && command->isValid();
}

bool BusSettings::hasConnection() const {
	return connection && connection->isValid();
}

ProtocolSettings::ProtocolSettings( const nlohmann::json& configuration ) :
	linesStartAt1( true ),
	columnsStartAt1( true ),
	pathFormatURI( false ),
	redirectStderr( configuration.value( REDIRECT_STDERR, false ) ),
	redirectStdout( configuration.value( REDIRECT_STDOUT, false ) ),
	supportsSourceRequest( configuration.value( "supportsSourceRequest", true ) ),
	launchArgs( configuration[REQUEST] ),
	locale( "en-US" ) {}

} // namespace ecode
