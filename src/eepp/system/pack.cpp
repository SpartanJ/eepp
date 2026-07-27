#include <eepp/system/pack.hpp>
#include <eepp/system/packregistry.hpp>
#include <eepp/system/virtualfilesystem.hpp>

namespace EE { namespace System {

Pack::Pack() : Mutex(), mIsOpen( false ) {
	PackRegistry::instance()->add( this );
}

Pack::~Pack() {
	PackRegistry::instance()->remove( this );
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

}} // namespace EE::System
