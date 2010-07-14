#include "cpack.hpp"

namespace EE { namespace System {

cPack::cPack() :
	cMutex(),
	mIsOpen(false)
{
}

cPack::~cPack() {
}

bool cPack::IsOpen() const {
	return mIsOpen;
}

}}
