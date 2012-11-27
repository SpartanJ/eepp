#include <eepp/audio/openal.hpp>
#include <eepp/audio/caudiodevice.hpp>
#include <iostream>

namespace EE { namespace Audio {

void ALCheckError(const std::string& File, unsigned int Line) {
	// Get the last error
	ALenum ErrorCode = alGetError();

	if (ErrorCode != AL_NO_ERROR) {
		std::string Error, Desc;

		// Decode the error code
		switch (ErrorCode) {
			case AL_INVALID_NAME :
				Error = "AL_INVALID_NAME";
				Desc  = "an unacceptable name has been specified";
				break;
			case AL_INVALID_ENUM :
				Error = "AL_INVALID_ENUM";
				Desc  = "an unacceptable value has been specified for an enumerated argument";
				break;
			case AL_INVALID_VALUE :
				Error = "AL_INVALID_VALUE";
				Desc  = "a numeric argument is out of range";
				break;
			case AL_INVALID_OPERATION :
				Error = "AL_INVALID_OPERATION";
				Desc  = "the specified operation is not allowed in the current state";
				break;
			case AL_OUT_OF_MEMORY :
				Error = "AL_OUT_OF_MEMORY";
				Desc  = "there is not enough memory left to execute the command";
				break;
		}

		// Log the error
		std::cerr << "An internal OpenAL call failed in " << File.substr(File.find_last_of("\\/") + 1) << " (" << Line << ") : " << Error << ", " << Desc << std::endl;
	}
}

void EnsureALInit() {
	static cAudioDevice GlobalDevice;
}

}}
