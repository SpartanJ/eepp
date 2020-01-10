#include <eepp/audio/alcheck.hpp>
#include <eepp/core/core.hpp>
#include <string>

namespace EE { namespace Audio {

void alCheckError( const char* file, unsigned int line, const char* expression ) {
	// Get the last error
	ALenum errorCode = alGetError();

	if ( errorCode != AL_NO_ERROR ) {
		std::string fileString = file;
		std::string error = "Unknown error";
		std::string description = "No description";

		// Decode the error code
		switch ( errorCode ) {
			case AL_INVALID_NAME: {
				error = "AL_INVALID_NAME";
				description = "A bad name (ID) has been specified.";
				break;
			}
			case AL_INVALID_ENUM: {
				error = "AL_INVALID_ENUM";
				description =
					"An unacceptable value has been specified for an enumerated argument.";
				break;
			}
			case AL_INVALID_VALUE: {
				error = "AL_INVALID_VALUE";
				description = "A numeric argument is out of range.";
				break;
			}
			case AL_INVALID_OPERATION: {
				error = "AL_INVALID_OPERATION";
				description = "The specified operation is not allowed in the current state.";
				break;
			}
			case AL_OUT_OF_MEMORY: {
				error = "AL_OUT_OF_MEMORY";
				description = "There is not enough memory left to execute the command.";
				break;
			}
		}

		// Log the error
		eePRINTL( "An internal OpenAL call failed in %s ( %d )\nExpression:\n%s\nError "
				  "description:\n   %s\n%s\n",
				  fileString.substr( fileString.find_last_of( "\\/" ) + 1 ).c_str(), line,
				  expression, error.c_str(), description.c_str() );
	}
}

}} // namespace EE::Audio
