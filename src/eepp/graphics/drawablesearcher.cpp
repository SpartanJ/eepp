#include <eepp/graphics/drawablesearcher.hpp>
#include <eepp/graphics/globaltextureatlas.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/sprite.hpp>

namespace EE { namespace Graphics {

bool DrawableSearcher::sPrintWarnings = false;

static Drawable * getSprite( const std::string& sprite ) {
	std::vector<DrawableResource*> tSubTextureVec = TextureAtlasManager::instance()->getSubTexturesByPattern( sprite );

	if ( tSubTextureVec.size() ) {
		Sprite * tSprite = eeNew( Graphics::Sprite, () );
		tSprite->createAnimation();
		tSprite->addFrames( tSubTextureVec );

		return tSprite;
	}

	return NULL;
}

static Drawable * searchByNameInternal( const std::string& name ) {
	Uint32 id = String::hash( name );
	Drawable * drawable = TextureAtlasManager::instance()->getSubTextureById( id );

	if ( NULL == drawable ) {
		drawable = TextureFactory::instance()->getByHash( id );
	}

	return drawable;
}

Drawable * DrawableSearcher::searchByName( const std::string& name ) {
	Drawable * drawable = NULL;

	if ( name.size() ) {
		if ( name[0] == '@' ) {
			if ( String::startsWith( name, "@subtexture/" ) ) {
				drawable = TextureAtlasManager::instance()->getSubTextureByName( name.substr( 12 ) );
			} else if ( String::startsWith( name, "@image/" ) ) {
				drawable = TextureFactory::instance()->getByName( name.substr( 7 ) );
			} else if ( String::startsWith( name, "@texture/" ) ) {
				drawable = TextureFactory::instance()->getByName( name.substr( 9 ) );
			} else if ( String::startsWith( name, "@sprite/" ) ) {
				drawable = getSprite( name.substr( 8 ) );
			} else if ( String::startsWith( name, "@drawable/" ) ) {
				drawable = searchByNameInternal( name.substr( 10 ) );
			} else {
				drawable = searchByNameInternal( name );
			}
		} else {
			drawable = searchByNameInternal( name );
		}
	}

	if ( NULL == drawable && sPrintWarnings )
		eePRINTL( "DrawableSearcher::searchByName: \"%s\" not found", name.c_str() );

	return drawable;
}

Drawable * DrawableSearcher::searchById( const Uint32& id ) {
	Drawable * drawable = TextureAtlasManager::instance()->getSubTextureById( id );

	if ( NULL == drawable ) {
		drawable = TextureFactory::instance()->getByHash( id );
	}

	if ( NULL == drawable && sPrintWarnings )
		eePRINTL( "DrawableSearcher::searchById: \"%ld\" not found", id );

	return drawable;
}

void DrawableSearcher::setPrintWarnings( const bool& print ) {
	sPrintWarnings = print;
}

bool DrawableSearcher::getPrintWarnings() {
	return sPrintWarnings;
}

}}
