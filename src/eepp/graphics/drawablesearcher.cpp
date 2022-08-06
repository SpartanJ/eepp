#include <eepp/graphics/drawablesearcher.hpp>
#include <eepp/graphics/globaltextureatlas.hpp>
#include <eepp/graphics/ninepatchmanager.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/system/base64.hpp>
#include <eepp/system/md5.hpp>

#include <eepp/network/http.hpp>
#include <eepp/network/uri.hpp>
#include <eepp/window/engine.hpp>
using namespace EE::Window;
using namespace EE::Network;

namespace EE { namespace Graphics {

bool DrawableSearcher::sPrintWarnings = false;

static Drawable* getSprite( const std::string& sprite ) {
	std::vector<TextureRegion*> tTextureRegionVec =
		TextureAtlasManager::instance()->getTextureRegionsByPattern( sprite );

	if ( tTextureRegionVec.size() ) {
		Sprite* tSprite = Graphics::Sprite::New();
		tSprite->createAnimation();
		tSprite->addFrames( tTextureRegionVec );

		return tSprite;
	}

	return NULL;
}

static Drawable* searchByNameInternal( const std::string& name ) {
	String::HashType id = String::hash( name );
	Drawable* drawable = TextureAtlasManager::instance()->getTextureRegionById( id );

	if ( NULL == drawable ) {
		drawable = NinePatchManager::instance()->getById( id );
	}

	if ( NULL == drawable ) {
		drawable = TextureFactory::instance()->getByHash( id );
	}

	return drawable;
}

static Drawable* parseDataURI( const std::string& name ) {
	auto hash = MD5::fromString( name ).toHexString();
	Drawable* drawable = TextureFactory::instance()->getByName( hash );
	std::string::size_type formatAndEncSep;
	if ( nullptr == drawable &&
		 ( formatAndEncSep = name.find_first_of( ',' ) ) != std::string::npos ) {
		std::string decodingType = "urldecode";
		std::string mediaType = name.substr( 0, formatAndEncSep );
		std::string format;
		auto parts = String::split( mediaType, ';' );
		if ( parts.empty() )
			return nullptr;
		auto formatNamePos = parts[0].find_first_of( '/' );
		if ( formatNamePos + 1 < mediaType.size() )
			format = parts[0].substr( formatNamePos + 1 );
		if ( parts.size() > 1 ) {
			for ( size_t i = 1; i < parts.size(); ++i ) {
				if ( "base64" == parts[i] ) {
					decodingType = parts[i];
					break;
				}
			}
		}

		Uint32 texId = 0;
		if ( !format.empty() &&
			 ( Image::isImageExtension( "." + format ) || format == "svg+xml" ) ) {
			if ( decodingType == "base64" ) {
				int fileStart = formatAndEncSep + 1;
				int base64Size = name.size() - fileStart;
				int bufSize = Base64::decodeSafeOutLen( base64Size );
				if ( bufSize <= 0 )
					return nullptr;
				ScopedBuffer buffer( bufSize );
				int len = Base64::decode( base64Size, &name[fileStart], bufSize, buffer.get() );
				if ( len > 0 )
					texId = TextureFactory::instance()->loadFromMemory( buffer.get(), len );
			} else if ( decodingType == "urldecode" ) {
				int fileStart = formatAndEncSep + 1;
				std::string decoded( URI::decode( name.substr( fileStart ) ) );
				if ( !decoded.empty() )
					texId = TextureFactory::instance()->loadFromMemory(
						(const unsigned char*)decoded.c_str(), decoded.size() );
			}
		}

		if ( texId > 0 ) {
			Texture* tex = TextureFactory::instance()->getTexture( texId );
			tex->setName( hash );
			drawable = tex;
		}
	}
	return drawable;
}

Drawable* DrawableSearcher::searchByName( const std::string& name, bool firstSearchSprite ) {
	Drawable* drawable = NULL;

	if ( name.size() ) {
		bool searchedSprite = false;

		if ( firstSearchSprite ) {
			if ( String::startsWith( name, "@sprite/" ) ) {
				drawable = getSprite( name.substr( 8 ) );
			} else {
				drawable = getSprite( name );
			}

			if ( NULL != drawable ) {
				return drawable;
			}

			searchedSprite = true;
		}

		if ( name[0] == '@' ) {
			if ( String::startsWith( name, "@textureregion/" ) ) {
				drawable =
					TextureAtlasManager::instance()->getTextureRegionByName( name.substr( 12 ) );
			} else if ( String::startsWith( name, "@image/" ) ) {
				drawable = TextureFactory::instance()->getByName( name.substr( 7 ) );
			} else if ( String::startsWith( name, "@texture/" ) ) {
				drawable = TextureFactory::instance()->getByName( name.substr( 9 ) );
			} else if ( String::startsWith( name, "@sprite/" ) && !searchedSprite ) {
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
		} else if ( String::startsWith( name, "http://" ) ||
					String::startsWith( name, "https://" ) ) {
			Texture* texture = TextureFactory::instance()->getByName( name );

			if ( NULL == texture && Engine::instance()->isSharedGLContextEnabled() ) {
				Uint32 texId = TextureFactory::instance()->createEmptyTexture(
					1, 1, 4, Color::Transparent, false, Texture::ClampMode::ClampToEdge, false,
					false, name );

				texture = TextureFactory::instance()->getTexture( texId );

				Http::getAsync(
					[=]( const Http&, Http::Request&, Http::Response& response ) {
						if ( !response.getBody().empty() ) {
							Image image( (const Uint8*)&response.getBody()[0],
										 response.getBody().size() );

							if ( image.getPixels() != NULL )
								texture->replace( &image );
						}
					},
					URI( name ), Seconds( 5 ) );
			}

			drawable = texture;
		} else if ( String::startsWith( name, "data:image/" ) ) {
			drawable = parseDataURI( name );
		} else {
			drawable = searchByNameInternal( name );
		}
	}

	if ( NULL == drawable && sPrintWarnings )
		Log::warning( "DrawableSearcher::searchByName: \"%s\" not found", name.c_str() );

	return drawable;
}

Drawable* DrawableSearcher::searchById( const Uint32& id ) {
	Drawable* drawable = TextureAtlasManager::instance()->getTextureRegionById( id );

	if ( NULL == drawable ) {
		drawable = TextureFactory::instance()->getByHash( id );
	}

	if ( NULL == drawable && sPrintWarnings )
		Log::warning( "DrawableSearcher::searchById: \"%ld\" not found", id );

	return drawable;
}

void DrawableSearcher::setPrintWarnings( const bool& print ) {
	sPrintWarnings = print;
}

bool DrawableSearcher::getPrintWarnings() {
	return sPrintWarnings;
}

}} // namespace EE::Graphics
