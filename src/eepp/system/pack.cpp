#include <eepp/system/pack.hpp>
#include <eepp/system/packmanager.hpp>

namespace EE { namespace System {

Pack::Pack() :
	Mutex(),
	mIsOpen(false)
{
	PackManager::instance()->Add( this );
}

Pack::~Pack() {
	PackManager::instance()->Remove( this );
}

bool Pack::IsOpen() const {
	return mIsOpen;
}

}}
