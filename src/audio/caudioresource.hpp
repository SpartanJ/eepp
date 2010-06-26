#ifndef EE_AUDIOCAUDIORESOURCE_H
#define EE_AUDIOCAUDIORESOURCE_H

#include "caudiodevice.hpp"

namespace EE { namespace Audio {

class EE_API cAudioResource {
	protected :
		cAudioResource();
		cAudioResource(const cAudioResource&);
		virtual ~cAudioResource();
};

}}

#endif
