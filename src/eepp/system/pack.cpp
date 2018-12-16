#include <eepp/system/pack.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/system/virtualfilesystem.hpp>

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

void Pack::onPackOpened() {
	VirtualFileSystem::instance()->onResourceAdd( this );
}

void Pack::onPackClosed() {
	VirtualFileSystem::instance()->onResourceRemove( this );
}

}}
