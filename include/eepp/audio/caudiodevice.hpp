#ifndef EE_AUDIOCAUDIODEVICE_H
#define EE_AUDIOCAUDIODEVICE_H

#include <eepp/audio/caudiolistener.hpp>

namespace EE { namespace Audio {

class EE_API cAudioDevice {
	public :
		cAudioDevice();

		~cAudioDevice();

		static ALenum GetFormatFromChannelsCount( unsigned int ChannelsCount );

		static bool IsExtensionSupported( const std::string& extension );

		static bool IsAvailable();
	private :
		void PrintInfo();
};

}}

#endif
