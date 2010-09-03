#ifndef EE_AUDIOCAUDIODEVICE_H
#define EE_AUDIOCAUDIODEVICE_H

#include "caudiolistener.hpp"

namespace EE { namespace Audio {

class EE_API cAudioDevice {
	public :
		static cAudioDevice * 	instance();

		static void 			AddReference();

		static void 			RemoveReference();

		ALCdevice * GetDevice() const;

		ALenum GetFormatFromChannelsCount(unsigned int ChannelsCount) const;

		bool IsExtensionSupported( const std::string& extension );

		bool isCreated();

		~cAudioDevice();
	private :
		cAudioDevice();

		static cAudioDevice * mInstance;

		ALCdevice *		mDevice;
		ALCcontext *	mContext;
		unsigned int	mRefCount;
};

}}

#endif
