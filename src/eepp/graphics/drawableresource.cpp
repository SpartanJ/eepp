#include <eepp/graphics/drawableresource.hpp>

namespace EE { namespace Graphics {

DrawableResource::DrawableResource( Type drawableType ) :
	Drawable( drawableType ), mId( 0 ), mNumCallBacks( 0 ) {
	createUnnamed();
}

DrawableResource::DrawableResource( Type drawableType, const std::string& name ) :
	Drawable( drawableType ), mId( 0 ), mNumCallBacks( 0 ) {
	setName( name );
}

DrawableResource::~DrawableResource() {
	sendEvent( Event::Unload );
}

const String::HashType& DrawableResource::getId() const {
	return mId;
}

const std::string DrawableResource::getName() const {
	return mName;
}

void DrawableResource::setName( const std::string& name ) {
	mName = name;
	mId = String::hash( mName );
}

void DrawableResource::createUnnamed() {
	if ( !mName.size() )
		setName( std::string( "unnamed" ) );
}

bool DrawableResource::isDrawableResource() const {
	return true;
}

void DrawableResource::onResourceChange() {
	sendEvent( Event::Change );
}

void DrawableResource::sendEvent( const Event& event ) {
	for ( const auto& cb : mCallbacks ) {
		cb.second( cb.first, event, this );
	}
}

Uint32 DrawableResource::pushResourceChangeCallback( const OnResourceChangeCallback& cb ) {
	mNumCallBacks++;
	mCallbacks[mNumCallBacks] = cb;
	return mNumCallBacks;
}

bool DrawableResource::popResourceChangeCallback( const Uint32& callbackId ) {
	return mCallbacks.erase( callbackId ) > 0;
}

}} // namespace EE::Graphics
