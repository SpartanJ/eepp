#include <eepp/system/pack.hpp>
#include <eepp/system/packfileindex.hpp>
#include <eepp/system/packregistry.hpp>

namespace EE { namespace System {

Pack::Pack() : Pack( true ) {}

Pack::Pack( bool registerResource ) : mIsOpen( false ), mRegisterResource( registerResource ) {
	if ( mRegisterResource )
		PackRegistry::instance()->add( this );
}

Pack::~Pack() {
	if ( mRegisterResource )
		PackRegistry::instance()->remove( this );
}

bool Pack::isOpen() const {
	return mIsOpen;
}

void Pack::onPackOpened() {
	if ( mRegisterResource )
		PackFileIndex::instance()->onResourceAdd( this );
}

void Pack::onPackClosed() {
	if ( mRegisterResource )
		PackFileIndex::instance()->onResourceRemove( this );
}

}} // namespace EE::System
