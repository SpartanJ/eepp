#include <eepp/graphics/image.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/network/http.hpp>
#include <eepp/network/uri.hpp>
#include <eepp/system/base64.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/md5.hpp>
#include <eepp/ui/drawableresolver.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/engine.hpp>

using namespace EE::Graphics;
using namespace EE::Network;
using namespace EE::Window;

namespace EE { namespace UI {

namespace {

std::string fileTexturePath( const std::string& name ) {
	// Layouts also use file://assets/... for paths relative to the working directory.
	std::string path = String::startsWith( name, "file://" ) ? name.substr( 7 ) : name;
#if EE_PLATFORM == EE_PLATFORM_WIN
	if ( path.size() >= 3 && path[0] == '/' && String::isLetter( path[1] ) && path[2] == ':' )
		path.erase( 0, 1 );
#endif
	FileSystem::filePathRemoveProcessPath( path );
	return path;
}

std::string fileTextureKey( std::string key, Float svgScale ) {
	if ( svgScale == 1.f )
		return key;
	MD5::Context hash;
	MD5::init( hash );
	MD5::update( hash, key.data(), key.size() );
	MD5::update( hash, &svgScale, sizeof( svgScale ) );
	return "svg:" + MD5::result( hash ).toHexString();
}

TexturePtr loadFileTexture( ResourceScope& scope, const std::string& path, const std::string& key,
							Float svgScale ) {
	return scope.findOrLoadTexture( key, [&] {
		Image::FormatConfiguration format;
		format.svgScale( svgScale );
		return TextureFactory::instance()->loadFromFile(
			path, false, Texture::ClampMode::ClampToEdge, false, false, format );
	} );
}

DrawablePtr parseDataURI( const std::string& name, ResourceScope& resourceScope ) {
	MD5::Context hashContext;
	MD5::init( hashContext );
	MD5::update( hashContext, name.data(), name.size() );
	if ( String::startsWith( name, "data:image/svg+xml" ) ) {
		// Shared catalogs can serve multiple scenes. The SVG source is shared, but its raster
		// at one target density must not satisfy a request at a different target density.
		const Float density = PixelDensity::getPixelDensity();
		MD5::update( hashContext, &density, sizeof( density ) );
	}
	std::string hash = MD5::result( hashContext ).toHexString();
	TexturePtr texture = resourceScope.findTexture( hash );
	std::string::size_type formatAndEncSep;
	if ( !texture && ( formatAndEncSep = name.find_first_of( ',' ) ) != std::string::npos ) {
		std::string decodingType = "urldecode";
		std::string mediaType = name.substr( 0, formatAndEncSep );
		std::string formatName;
		auto parts = String::split( mediaType, ';' );
		if ( parts.empty() )
			return {};
		auto formatNamePos = parts[0].find_first_of( '/' );
		if ( formatNamePos + 1 < mediaType.size() )
			formatName = parts[0].substr( formatNamePos + 1 );
		for ( size_t i = 1; i < parts.size(); ++i ) {
			if ( parts[i] == "base64" ) {
				decodingType = parts[i];
				break;
			}
		}

		TexturePtr decodedTexture;
		if ( !formatName.empty() &&
			 ( Image::isImageExtension( "." + formatName ) || formatName == "svg+xml" ) ) {
			Image::FormatConfiguration format;
			format.svgScale( PixelDensity::getPixelDensity() );
			std::string_view encoded = std::string_view{ name }.substr( formatAndEncSep + 1 );
			if ( decodingType == "base64" ) {
				std::string buffer;
				if ( Base64::decode( encoded, buffer ) > 0 ) {
					decodedTexture = TextureFactory::instance()->loadFromMemory(
						reinterpret_cast<const unsigned char*>( buffer.data() ), buffer.size(),
						false, Texture::ClampMode::ClampToEdge, false, false, format );
				}
			} else {
				std::string decoded = URI::decode( encoded );
				if ( !decoded.empty() ) {
					decodedTexture = TextureFactory::instance()->loadFromMemory(
						reinterpret_cast<const unsigned char*>( decoded.data() ), decoded.size(),
						false, Texture::ClampMode::ClampToEdge, false, false, format );
				}
			}
		}

		if ( decodedTexture ) {
			decodedTexture->setName( hash );
			resourceScope.publishLocal( hash, decodedTexture );
			texture = std::move( decodedTexture );
		}
	}

	return texture ? texture->clone() : DrawablePtr{};
}

} // namespace

DrawableResolver::DrawableResolver( UISceneNode& sceneNode ) : mSceneNode( &sceneNode ) {}

DrawableResolver::DrawableResolver( ResourceScope& resourceScope ) :
	mResourceScope( &resourceScope ) {}

bool DrawableResolver::isSvgPath( std::string_view path ) {
	return path.size() >= 4 && String::iequals( path.substr( path.size() - 4 ), ".svg" );
}

bool DrawableResolver::requestFileTexture( const URI& uri,
										   std::function<void( TexturePtr )> callback ) const {
	if ( !mSceneNode || !mSceneNode->hasThreadPool() )
		return false;
	std::string path = uri.getFSPath();
	const bool svg = isSvgPath( path );
	if ( !svg && !Engine::instance()->isSharedGLContextEnabled() )
		return false;
	const Float scale = svg ? mSceneNode->getPixelDensity() : 1.f;
	// Match the synchronous resolver's key. getFSPath() uses backslashes on Windows.
	const std::string url = uri.toString();
	std::string key = fileTextureKey(
		fileTexturePath( String::startsWith( url, "file://" ) ? url : path ), scale );
	auto scope = mSceneNode->getResourceScope();
	if ( auto texture = scope->findTexture( key ) ) {
		callback( std::move( texture ) );
		return true;
	}
	auto state = mSceneNode->getAsyncResourceLoadState();
	const Uint64 generation = state ? state->generation.load( std::memory_order_acquire ) : 0;
	mSceneNode->getThreadPool()->run( [state = std::move( state ), generation,
									   scope = std::move( scope ), path = std::move( path ),
									   key = std::move( key ), scale, svg,
									   callback = std::move( callback )]() mutable {
		if ( !UISceneNode::isAsyncResourceLoadCurrent( state, generation ) )
			return;
		if ( svg ) {
			Image::FormatConfiguration format;
			const Float rasterScale = Image::getFormat( path ) == Image::Format::SVG ? scale : 1.f;
			format.svgScale( rasterScale );
			auto image = std::make_shared<Image>( std::move( path ), 0, format );
			if ( !image->getPixelsPtr() )
				return;
			UISceneNode::runAsyncResourceOnMainThread(
				state, generation,
				[scope = std::move( scope ), key = std::move( key ), rasterScale,
				 image = std::move( image ), callback = std::move( callback )]( UISceneNode* ) {
					auto texture = scope->findOrLoadTexture( key, [&] {
						auto loaded = TextureFactory::instance()->loadFromPixels(
							image->getPixelsPtr(), image->getWidth(), image->getHeight(),
							image->getChannels() );
						if ( loaded )
							loaded->setRasterScale( rasterScale );
						return loaded;
					} );
					if ( texture )
						callback( std::move( texture ) );
				} );
			return;
		}

		if ( auto texture = loadFileTexture( *scope, path, key, 1.f ) ) {
			UISceneNode::runAsyncResourceOnMainThread(
				state, generation,
				[texture = std::move( texture ), callback = std::move( callback )](
					UISceneNode* ) mutable { callback( std::move( texture ) ); } );
		}
	} );
	return true;
}

DrawablePtr DrawableResolver::resolve( const std::string& name, bool firstSearchSprite ) const {
	auto densityContext = PixelDensity::makeCurrent( mSceneNode ? mSceneNode->getPixelDensity()
																: PixelDensity::getPixelDensity() );
	DrawablePtr drawable;
	if ( name.empty() )
		return drawable;

	ResourceScope& resourceScope = mSceneNode ? *mSceneNode->getResourceScope() : *mResourceScope;
	if ( String::startsWith( name, "file://" ) ) {
		const std::string path = fileTexturePath( name );
		const Float scale = isSvgPath( path ) ? PixelDensity::getPixelDensity() : 1.f;
		auto texture = loadFileTexture( resourceScope, path, fileTextureKey( path, scale ), scale );
		drawable = texture ? texture->clone() : DrawablePtr{};
	} else if ( String::startsWith( name, "http://" ) || String::startsWith( name, "https://" ) ) {
		TexturePtr texture = resourceScope.findTexture( name );
		if ( mSceneNode && Engine::instance()->isSharedGLContextEnabled() ) {
			if ( !texture ) {
				WebResourceRequest request;
				request.uri = URI( name );
				request.kind = WebResourceKind::Image;
				texture = mSceneNode->requestWebTexture(
					std::move( request ), [name]( const WebResourceResult& result ) {
						if ( !result.success )
							Log::debug( "DrawableResolver::resolve: could not download image: %s. "
										"Error: %d\n%s",
										name, result.status, result.error );
					} );
			}
		}
		if ( !mSceneNode && !texture && Engine::instance()->isSharedGLContextEnabled() ) {
			texture = TextureFactory::instance()->createEmptyTexture(
				1, 1, 4, Color::Transparent, false, Texture::ClampMode::ClampToEdge, false, false,
				name );
			if ( texture )
				resourceScope.publishLocal( name, texture );

			Http::Request::FieldTable headers;
			if ( mSceneNode && !mSceneNode->getReferer().empty() )
				headers["referer"] = mSceneNode->getReferer().toString();
			Http::getAsync(
				[texture, name]( const Http&, Http::Request&, Http::Response& response ) {
					if ( response.isOK() && !response.getBody().empty() ) {
						Image image( reinterpret_cast<const Uint8*>( response.getBody().data() ),
									 response.getBody().size() );
						if ( image.getPixels() )
							texture->replace( &image );
					} else {
						Log::debug(
							"DrawableResolver::resolve: could not download image: %s. Error: "
							"%d\n%s",
							name, response.getStatus(), response.getBody() );
					}
				},
				URI( name ), Seconds( 5 ), {}, headers );
		}
		drawable = texture ? texture->clone() : DrawablePtr{};
	} else if ( String::startsWith( name, "data:image/" ) ) {
		drawable = parseDataURI( name, resourceScope );
	} else {
		drawable = resourceScope.findDrawable( name, firstSearchSprite );
	}

	if ( !drawable && mPrintWarnings )
		Log::warning( "DrawableResolver::resolve: \"%s\" not found", name.c_str() );
	return drawable;
}

DrawablePtr DrawableResolver::resolveById( ResourceNameHash hash ) const {
	ResourceScope& resourceScope = mSceneNode ? *mSceneNode->getResourceScope() : *mResourceScope;
	DrawablePtr drawable = resourceScope.findDrawable( hash );
	if ( !drawable && mPrintWarnings )
		Log::warning( "DrawableResolver::resolveById: \"%llu\" not found",
					  static_cast<unsigned long long>( hash.value() ) );
	return drawable;
}

DrawablePtr DrawableResolver::resolveById( String::HashType legacyHash ) const {
	ResourceScope& resourceScope = mSceneNode ? *mSceneNode->getResourceScope() : *mResourceScope;
	DrawablePtr drawable = resourceScope.findDrawable( legacyHash );
	if ( !drawable && mPrintWarnings )
		Log::warning( "DrawableResolver::resolveById: legacy hash \"%u\" not found", legacyHash );
	return drawable;
}

void DrawableResolver::setPrintWarnings( bool printWarnings ) {
	mPrintWarnings = printWarnings;
}

bool DrawableResolver::getPrintWarnings() const {
	return mPrintWarnings;
}

}} // namespace EE::UI
