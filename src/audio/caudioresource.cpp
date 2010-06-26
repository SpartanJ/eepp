#include "caudioresource.hpp"

namespace EE { namespace Audio {

cAudioResource::cAudioResource() {
	cAudioDevice::AddReference();
}

cAudioResource::cAudioResource(const cAudioResource&) {
	cAudioDevice::AddReference();
}

cAudioResource::~cAudioResource() {
	cAudioDevice::RemoveReference();
}

}}
