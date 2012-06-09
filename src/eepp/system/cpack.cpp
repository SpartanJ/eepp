#include <eepp/system/cpack.hpp>
#include <eepp/system/cpackmanager.hpp>

namespace EE { namespace System {

cPack::cPack() :
	cMutex(),
	mIsOpen(false)
{
	cPackManager::instance()->Add( this );
}

cPack::~cPack() {
	cPackManager::instance()->Remove( this );
}

bool cPack::IsOpen() const {
	return mIsOpen;
}

}}
