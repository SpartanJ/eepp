#ifndef EE_AUDIOCAUDIODEVICE_H
#define EE_AUDIOCAUDIODEVICE_H

#include <eepp/audio/audiolistener.hpp>

namespace EE { namespace Audio {

/**
 @brief High-level wrapper around the audio API, it manages
		the creation and destruction of the audio device and
		context and stores the device capabilities
*/
class EE_API AudioDevice {
	public :
		AudioDevice();

		~AudioDevice();

		/** Get the OpenAL format that matches the given number of channels */
		static int getFormatFromChannelCount( unsigned int ChannelCount );

		/** Checks if a AL or ALC extension is supported */
		static bool isExtensionSupported( const std::string& extension );

		/** @return True if the audio device was initialized */
		static bool isAvailable();
	private :
		void printInfo();
};

}}

#endif
