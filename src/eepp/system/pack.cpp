#include <eepp/system/pack.hpp>
#include <eepp/system/packregistry.hpp>
#include <eepp/system/packfileindex.hpp>

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
	PackFileIndex::instance()->onResourceAdd( this );
}

void Pack::onPackClosed() {
	PackFileIndex::instance()->onResourceRemove( this );
}

}} // namespace EE::System
