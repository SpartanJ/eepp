#include <eepp/graphics/font.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/window/engine.hpp>

namespace EE { namespace Graphics {

Font::Font( const Uint32& Type, const std::string& Name ) :
	mType( Type )
{
	this->setName( Name );
	FontManager::instance()->add( this );
}

Font::~Font() {
	if ( !FontManager::instance()->isDestroying() ) {
		FontManager::instance()->remove( this, false );
	}
}

const Uint32& Font::getType() const {
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

}}
