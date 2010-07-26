#ifndef EE_AUDIOCAUDIODEVICE_H
#define EE_AUDIOCAUDIODEVICE_H

#include "caudiolistener.hpp"

namespace EE { namespace Audio {

class cAudioDevice {
	public :
		static cAudioDevice * 	instance();

		static void 			AddReference();

		static void 			RemoveReference();

		ALCdevice * GetDevice() const;

		ALenum GetFormatFromChannelsCount(unsigned int ChannelsCount) const;

		bool IsExtensionSupported( const std::string& extension );

		bool isCreated();
	private :
		cAudioDevice();

		~cAudioDevice();

		static cAudioDevice * mInstance;

		ALCdevice *		mDevice;
		ALCcontext *	mContext;
		unsigned int	mRefCount;
};

}}

#endif
