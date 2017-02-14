#include <eepp/system/pack.hpp>
#include <eepp/system/packmanager.hpp>

namespace EE { namespace System {

Pack::Pack() :
	Mutex(),
	mIsOpen(false)
{
	PackManager::instance()->add( this );
}

Pack::~Pack() {
	PackManager::instance()->remove( this );
}

bool Pack::isOpen() const {
	return mIsOpen;
}

}}
