#include <eepp/graphics/drawablesearcher.hpp>
#include <eepp/graphics/globaltextureatlas.hpp>
#include <eepp/graphics/ninepatchmanager.hpp>
#include <eepp/graphics/resourcescope.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/system/base64.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/md5.hpp>

#include <eepp/network/http.hpp>
#include <eepp/network/uri.hpp>
#include <eepp/window/engine.hpp>
using namespace EE::Window;
using namespace EE::Network;

namespace EE { namespace Graphics {

bool DrawableSearcher::sPrintWarnings = false;

static DrawablePtr getSprite( const std::string& sprite ) {
	std::vector<TextureRegion*> tTextureRegionVec =
		TextureAtlasManager::instance()->getTextureRegionsByPattern( sprite );

	if ( tTextureRegionVec.size() ) {
		SpritePtr tSprite = Sprite::New();
		tSprite->createAnimation();
		tSprite->addFrames( tTextureRegionVec );

		return tSprite;
	}

	return {};
}

static DrawablePtr searchByNameInternal( const std::string& name, ResourceScope& resourceScope ) {
	String::HashType id = String::hash( name );
	Drawable* source = TextureAtlasManager::instance()->getTextureRegionById( id );

	if ( NULL == source ) {
		source = NinePatchManager::instance()->getById( id );
	}

	if ( source ) {
		return source->createInstance();
	}

	TexturePtr texture = resourceScope.findTexture( name );
	return texture ? texture->createInstance() : DrawablePtr{};
}

static DrawablePtr parseDataURI( const std::string& name, ResourceScope& scope ) {
	auto hash = MD5::fromString( name ).toHexString();
	TexturePtr texture = scope.findTexture( hash );
	std::string::size_type formatAndEncSep;
	if ( !texture &&
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

		TexturePtr tex;
		if ( !format.empty() &&
			 ( Image::isImageExtension( "." + format ) || format == "svg+xml" ) ) {
			Image::FormatConfiguration format;
			format.svgScale( PixelDensity::getPixelDensity() );
			if ( decodingType == "base64" ) {
				int fileStart = formatAndEncSep + 1;
				std::string_view fileBase64 = std::string_view{ name }.substr( fileStart );
				std::string buffer;
				int len = Base64::decode( fileBase64, buffer );
				if ( len > 0 ) {
					tex = TextureFactory::instance()->loadFromMemory(
						(const unsigned char*)buffer.c_str(), buffer.size(), false,
						Texture::ClampMode::ClampToEdge, false, false, format );
				}
			} else if ( decodingType == "urldecode" ) {
				int fileStart = formatAndEncSep + 1;
				std::string decoded( URI::decode( name.substr( fileStart ) ) );
				if ( !decoded.empty() ) {
					tex = TextureFactory::instance()->loadFromMemory(
						(const unsigned char*)decoded.c_str(), decoded.size(), false,
						Texture::ClampMode::ClampToEdge, false, false, format );
				}
			}
		}

		if ( tex ) {
			tex->setName( hash );
			scope.publishLocal( hash, tex );
			texture = std::move( tex );
		}
	}
	return texture ? texture->createInstance() : DrawablePtr{};
}

DrawablePtr DrawableSearcher::searchByName( const std::string& name, bool firstSearchSprite,
										  Network::URI referer,
										  ResourceScope* requestedResourceScope ) {
	DrawablePtr drawable;

	if ( name.size() ) {
		ResourceScope& resourceScope =
			requestedResourceScope ? *requestedResourceScope : defaultResourceScope();
		bool searchedSprite = false;

		if ( firstSearchSprite ) {
			if ( String::startsWith( name, "@sprite/" ) ) {
				drawable = getSprite( name.substr( 8 ) );
			} else {
				drawable = getSprite( name );
			}

			if ( drawable ) {
				return drawable;
			}

			searchedSprite = true;
		}

		if ( name[0] == '@' ) {
			if ( String::startsWith( name, "@textureregion/" ) ) {
				if ( Drawable* source =
						 TextureAtlasManager::instance()->getTextureRegionByName( name.substr( 12 ) ) )
					drawable = source->createInstance();
			} else if ( String::startsWith( name, "@image/" ) ) {
				TexturePtr texture = resourceScope.findTexture( name.substr( 7 ) );
				drawable = texture ? texture->createInstance() : DrawablePtr{};
			} else if ( String::startsWith( name, "@texture/" ) ) {
				TexturePtr texture = resourceScope.findTexture( name.substr( 9 ) );
				drawable = texture ? texture->createInstance() : DrawablePtr{};
			} else if ( String::startsWith( name, "@sprite/" ) && !searchedSprite ) {
				drawable = getSprite( name.substr( 8 ) );
			} else if ( String::startsWith( name, "@drawable/" ) ) {
				drawable = searchByNameInternal( name.substr( 10 ), resourceScope );
			} else if ( String::startsWith( name, "@9p/" ) ) {
				if ( Drawable* source = NinePatchManager::instance()->getByName( name.substr( 4 ) ) )
					drawable = source->createInstance();
			} else {
				drawable = searchByNameInternal( name, resourceScope );
			}
		} else if ( String::startsWith( name, "file://" ) ) {
			std::string filePath( name.substr( 7 ) );

#if EE_PLATFORM == EE_PLATFORM_WIN
			if ( filePath.size() >= 3 && filePath[0] == '/' && String::isLetter( filePath[1] ) &&
				 filePath[2] == ':' ) {
				filePath = filePath.substr( 1 );
			}
#endif

			FileSystem::filePathRemoveProcessPath( filePath );

			TexturePtr texture = resourceScope.findTexture( filePath );

			if ( !texture ) {
				TexturePtr tex = TextureFactory::instance()->loadFromFile( filePath );

				if ( tex ) {
					resourceScope.publishLocal( filePath, tex );
					texture = std::move( tex );
				}
			}
			drawable = texture ? texture->createInstance() : DrawablePtr{};
		} else if ( String::startsWith( name, "http://" ) ||
					String::startsWith( name, "https://" ) ) {
			TexturePtr texture = resourceScope.findTexture( name );

			if ( NULL == texture && Engine::instance()->isSharedGLContextEnabled() ) {
				texture = TextureFactory::instance()->createEmptyTexture(
					1, 1, 4, Color::Transparent, false, Texture::ClampMode::ClampToEdge, false,
					false, name );
				if ( texture )
					resourceScope.publishLocal( name, texture );

				std::map<std::string, std::string> headers;
				if ( !referer.empty() )
					headers["referer"] = referer.toString();

				Http::getAsync(
					[texture, name]( const Http&, Http::Request&, Http::Response& response ) {
						if ( response.isOK() && !response.getBody().empty() ) {
							Image image( (const Uint8*)&response.getBody()[0],
										 response.getBody().size() );

							if ( image.getPixels() != NULL )
								texture->replace( &image );
						} else {
							Log::debug( "DrawableSearcher::searchByName: could not download image: "
										"%s. Error: %d\n%s",
										name, response.getStatus(), response.getBody() );
						}
					},
					URI( name ), Seconds( 5 ), {}, headers );
			}

			drawable = texture ? texture->createInstance() : DrawablePtr{};
		} else if ( String::startsWith( name, "data:image/" ) ) {
			drawable = parseDataURI( name, resourceScope );
		} else {
			drawable = searchByNameInternal( name, resourceScope );
		}
	}

	if ( !drawable && sPrintWarnings )
		Log::warning( "DrawableSearcher::searchByName: \"%s\" not found", name.c_str() );

	return drawable;
}

DrawablePtr DrawableSearcher::searchById( const Uint32& id ) {
	Drawable* source = TextureAtlasManager::instance()->getTextureRegionById( id );
	DrawablePtr drawable = source ? source->createInstance() : DrawablePtr{};

	if ( !drawable && sPrintWarnings )
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
