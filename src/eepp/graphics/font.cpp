#include <eepp/graphics/font.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/window/engine.hpp>

namespace EE { namespace Graphics {

Font::Font( const FontType& Type, const std::string& Name ) : mType( Type ), mNumCallBacks( 0 ) {
	this->setName( Name );
	FontManager::instance()->add( this );
}

Font::~Font() {
	if ( !FontManager::instance()->isDestroying() ) {
		FontManager::instance()->remove( this, false );
	}
}

const FontType& Font::getType() const {
	return mType;
}

const std::string& Font::getName() const {
	return mFontName;
}

void Font::setName( const std::string& name ) {
	mFontName = name;
	mFontHash = String::hash( mFontName );
}

const Uint32& Font::getId() {
	return mFontHash;
}

Uint32 Font::pushFontEventCallback( const FontEventCallback& cb ) {
	mNumCallBacks++;
	mCallbacks[mNumCallBacks] = cb;
	return mNumCallBacks;
}

void Font::popFontEventCallback( const Uint32& callbackId ) {
	mCallbacks[callbackId] = 0;
	mCallbacks.erase( mCallbacks.find( callbackId ) );
}

void Font::sendEvent( const Event& event ) {
	for ( const auto& cb : mCallbacks ) {
		cb.second( cb.first, event, this );
	}
}

}} // namespace EE::Graphics
