#include <eepp/graphics/drawablesearcher.hpp>
#include <eepp/graphics/globaltextureatlas.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/ninepatchmanager.hpp>
#include <eepp/graphics/sprite.hpp>

namespace EE { namespace Graphics {

bool DrawableSearcher::sPrintWarnings = false;

static Drawable * getSprite( const std::string& sprite ) {
	std::vector<TextureRegion*> tTextureRegionVec = TextureAtlasManager::instance()->getTextureRegionsByPattern( sprite );

	if ( tTextureRegionVec.size() ) {
		Sprite * tSprite = Graphics::Sprite::New();
		tSprite->createAnimation();
		tSprite->addFrames( tTextureRegionVec );

		return tSprite;
	}

	return NULL;
}

static Drawable * searchByNameInternal( const std::string& name ) {
	Uint32 id = String::hash( name );
	Drawable * drawable = TextureAtlasManager::instance()->getTextureRegionById( id );

	if ( NULL == drawable) {
		drawable = NinePatchManager::instance()->getById( id );
	}

	if ( NULL == drawable ) {
		drawable = TextureFactory::instance()->getByHash( id );
	}

	return drawable;
}

Drawable * DrawableSearcher::searchByName( const std::string& name ) {
	Drawable * drawable = NULL;

	if ( name.size() ) {
		if ( name[0] == '@' ) {
			if ( String::startsWith( name, "@textureregion/" ) || String::startsWith( name, "@subtexture/" ) ) {
				drawable = TextureAtlasManager::instance()->getTextureRegionByName( name.substr( 12 ) );
			} else if ( String::startsWith( name, "@image/" ) ) {
				drawable = TextureFactory::instance()->getByName( name.substr( 7 ) );
			} else if ( String::startsWith( name, "@texture/" ) ) {
				drawable = TextureFactory::instance()->getByName( name.substr( 9 ) );
			} else if ( String::startsWith( name, "@sprite/" ) ) {
				drawable = getSprite( name.substr( 8 ) );
			} else if ( String::startsWith( name, "@drawable/" ) ) {
				drawable = searchByNameInternal( name.substr( 10 ) );
			} else if ( String::startsWith( name, "@9p/" ) ) {
				drawable = NinePatchManager::instance()->getByName( name.substr( 4 ) );
			} else {
				drawable = searchByNameInternal( name );
			}
		} else if ( String::startsWith( name, "file://" ) ) {
			std::string filePath( name.substr( 7 ) );

			drawable = TextureFactory::instance()->getByName( filePath );

			if ( NULL == drawable ) {
				Uint32 texId = TextureFactory::instance()->loadFromFile( filePath );

				if ( texId > 0 )
					drawable = TextureFactory::instance()->getTexture( texId );
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
	Drawable * drawable = TextureAtlasManager::instance()->getTextureRegionById( id );

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
