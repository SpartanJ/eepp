#include <eepp/graphics/drawablesearcher.hpp>
#include <eepp/graphics/globaltextureatlas.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/sprite.hpp>

namespace EE { namespace Graphics {

static Drawable * getSprite( const std::string& sprite ) {
	std::vector<SubTexture*> tSubTextureVec = TextureAtlasManager::instance()->getSubTexturesByPattern( sprite );

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
	Drawable * drawable = GlobalTextureAtlas::instance()->getById( id );

	if ( NULL == drawable ) {
		drawable = TextureFactory::instance()->getByHash( id );
	}

	if ( NULL == drawable ) {
		drawable = getSprite( name );
	}

	return drawable;
}

Drawable * DrawableSearcher::searchByName( const std::string& name ) {
	Drawable * drawable = NULL;

	if ( name.size() ) {
		if ( name[0] == '@' ) {
			if ( String::startsWith( name, "@subtexture/" ) ) {
				return GlobalTextureAtlas::instance()->getByName( name.substr( 12 ) );
			} else if ( String::startsWith( name, "@image/" ) ) {
				return TextureFactory::instance()->getByName( name.substr( 7 ) );
			} else if ( String::startsWith( name, "@texture/" ) ) {
				return TextureFactory::instance()->getByName( name.substr( 9 ) );
			} else if ( String::startsWith( name, "@sprite/" ) ) {
				return getSprite( name.substr( 8 ) );
			} else if ( String::startsWith( name, "@drawable/" ) ) {
				return searchByNameInternal( name.substr( 10 ) );
			} else {
				return searchByNameInternal( name );
			}
		} else {
			return searchByNameInternal( name );
		}
	}

	return drawable;
}

Drawable * DrawableSearcher::searchById( const Uint32& id ) {
	Drawable * drawable = GlobalTextureAtlas::instance()->getById( id );

	if ( NULL == drawable ) {
		drawable = TextureFactory::instance()->getByHash( id );
	}

	if ( NULL == drawable ) {
		std::vector<SubTexture*> tSubTextureVec = TextureAtlasManager::instance()->getSubTexturesByPatternId( id );

		if ( tSubTextureVec.size() ) {
			Sprite * tSprite = eeNew( Graphics::Sprite, () );
			tSprite->createAnimation();
			tSprite->addFrames( tSubTextureVec );

			drawable = tSprite;
		}
	}

	return drawable;
}

}}
