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

DrawablePtr parseDataURI( const std::string& name, ResourceScope& resourceScope ) {
	std::string hash = MD5::fromString( name ).toHexString();
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

DrawablePtr DrawableResolver::resolve( const std::string& name, bool firstSearchSprite ) const {
	DrawablePtr drawable;
	if ( name.empty() )
		return drawable;

	ResourceScope& resourceScope = mSceneNode ? *mSceneNode->getResourceScope() : *mResourceScope;
	if ( String::startsWith( name, "file://" ) ) {
		std::string filePath = name.substr( 7 );
#if EE_PLATFORM == EE_PLATFORM_WIN
		if ( filePath.size() >= 3 && filePath[0] == '/' && String::isLetter( filePath[1] ) &&
			 filePath[2] == ':' )
			filePath.erase( 0, 1 );
#endif
		FileSystem::filePathRemoveProcessPath( filePath );
		TexturePtr texture = resourceScope.findTexture( filePath );
		if ( !texture ) {
			texture = TextureFactory::instance()->loadFromFile( filePath );
			if ( texture )
				resourceScope.publishLocal( filePath, texture );
		}
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

DrawablePtr DrawableResolver::resolveById( const Uint32& id ) const {
	ResourceScope& resourceScope = mSceneNode ? *mSceneNode->getResourceScope() : *mResourceScope;
	DrawablePtr drawable = resourceScope.findDrawable( id );
	if ( !drawable && mPrintWarnings )
		Log::warning( "DrawableResolver::resolveById: \"%ld\" not found", id );
	return drawable;
}

void DrawableResolver::setPrintWarnings( bool printWarnings ) {
	mPrintWarnings = printWarnings;
}

bool DrawableResolver::getPrintWarnings() const {
	return mPrintWarnings;
}

}} // namespace EE::UI
