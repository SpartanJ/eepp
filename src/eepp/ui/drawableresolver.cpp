#include <eepp/core/small_vector.hpp>
#include <eepp/graphics/image.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/texturedrawable.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/textureloader.hpp>
#include <eepp/network/http.hpp>
#include <eepp/network/uri.hpp>
#include <eepp/system/base64.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/md5.hpp>
#if EE_PLATFORM == EE_PLATFORM_WIN
#include <eepp/system/packregistry.hpp>
#endif
#include <eepp/ui/drawableresolver.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/engine.hpp>
#include <mutex>

using namespace EE::Graphics;
using namespace EE::Network;
using namespace EE::Window;

namespace EE { namespace UI {

namespace {

std::string fileTexturePath( const std::string& name ) {
	std::string path;
	if ( !String::startsWith( name, "file://" ) ) {
		path = name;
	} else {
		URI uri( name );
		if ( uri.getHost().empty() || uri.getHost() == "localhost" ) {
			path = uri.getFSPath();
		} else {
			// Layouts also use file://assets/... for paths relative to the working directory.
			URI::decode( std::string_view( name ).substr( 7 ), path );
#if EE_PLATFORM == EE_PLATFORM_WIN
			if ( !uri.getPath().empty() && !FileSystem::fileExists( path ) ) {
				auto* packs = PackRegistry::instance();
				if ( !packs->isFallbackToPacksActive() || !packs->exists( path ) )
					path = uri.getFSPath();
			}
#endif
		}
	}
	FileSystem::filePathRemoveProcessPath( path );
	return path;
}

// Bitmaps are cached under their path, so only scaled SVGs build a separate key in svgKey.
const std::string& fileTextureKey( const std::string& path, Float svgScale, std::string& svgKey ) {
	if ( svgScale == 1.f )
		return path;
	MD5::Context hash;
	MD5::init( hash );
	MD5::update( hash, path.data(), path.size() );
	MD5::update( hash, &svgScale, sizeof( svgScale ) );
	svgKey = "svg:" + MD5::result( hash ).toHexString();
	return svgKey;
}

TexturePtr loadFileTexture( const std::string& path, Float svgScale ) {
	Image::FormatConfiguration format;
	format.svgScale( svgScale );
	return TextureFactory::instance()->loadFromFile( path, false, Texture::ClampMode::ClampToEdge,
													 false, false, format );
}

struct FileTextureLoad {
	TextureLoader loader;
	std::once_flag preparation;
	std::exception_ptr error;

	FileTextureLoad( const std::string& path, Float scale ) : loader( path ) {
		Image::FormatConfiguration format;
		format.svgScale( scale );
		loader.setFormatConfiguration( format );
	}

	void prepare() {
		std::call_once( preparation, [this] {
			try {
				loader.prepare();
			} catch ( ... ) {
				error = std::current_exception();
			}
		} );
	}

	TexturePtr upload() {
		prepare();
		if ( error )
			std::rethrow_exception( error );
		loader.load();
		return loader.getTexture();
	}
};

class PendingTextureDrawable final : public TextureDrawable {
  public:
	struct Load {
		TexturePtr texture;
		// A pending image usually has its drawable and one clone.
		SmallVector<std::weak_ptr<PendingTextureDrawable>, 2> drawables;

		void finish( TexturePtr ready ) {
			texture = std::move( ready );
			for ( const auto& drawable : drawables ) {
				if ( auto instance = drawable.lock() ) {
					instance->mLoad.reset();
					instance->setName( texture->getName() );
					instance->setTexture( texture );
				}
			}
		}
	};

	explicit PendingTextureDrawable( const std::shared_ptr<Load>& load ) :
		TextureDrawable( load->texture ), mLoad( load ) {}

	static TextureDrawablePtr New( const std::shared_ptr<Load>& load ) {
		if ( load->texture )
			return TextureDrawable::New( load->texture );
		auto drawable = makeResource<PendingTextureDrawable>( load );
		load->drawables.emplace_back( drawable );
		return drawable;
	}

	DrawablePtr clone() const override {
		auto load = mLoad.lock();
		if ( !load )
			return TextureDrawable::clone();
		auto drawable = New( load );
		drawable->setColor( mColor );
		drawable->setPosition( mPosition );
		return drawable;
	}

  private:
	std::weak_ptr<Load> mLoad;
};

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

DrawablePtr DrawableResolver::resolveAsync( const std::string& name,
											bool firstSearchSprite ) const {
	if ( !String::startsWith( name, "file://" ) )
		return resolve( name, firstSearchSprite );

	const std::string path = fileTexturePath( name );
	auto scope = mSceneNode ? mSceneNode->getResourceScope() : ResourceScopePtr{};
	const Float density =
		mSceneNode ? mSceneNode->getPixelDensity() : PixelDensity::getPixelDensity();
	const Float scale = isSvgPath( path ) ? density : 1.f;
	std::string svgKey;
	const std::string& key = fileTextureKey( path, scale, svgKey );
	if ( auto cached = ( scope ? scope.get() : mResourceScope )->findTexture( key ) )
		return cached->clone();

	if ( !mSceneNode )
		return {};
	auto pool = mSceneNode->getThreadPool();
	if ( pool && !pool->numThreads() )
		pool.reset();

	// Allocated only for uncached files: the drawables wait on it until the texture is ready.
	auto pending = std::make_shared<PendingTextureDrawable::Load>();
	auto state = mSceneNode->getAsyncResourceLoadState();
	const Uint64 generation = state->generation.load( std::memory_order_acquire );
	auto result = ResourceScope::requestTextureLoad(
		scope, key, pool.get(),
		[&] {
			// Construct on the scene thread: the loader captures renderer capabilities here.
			auto load = std::make_shared<FileTextureLoad>( path, scale );
			return ResourceScope::TextureLoad{
				[load, state, generation] {
					// A cancelled request skips decoding. Its upload is discarded.
					if ( UISceneNode::isAsyncResourceLoadCurrent( state, generation ) )
						load->prepare();
				},
				[load] { return load->upload(); } };
		},
		[state, generation]( std::function<void()> upload ) {
			UISceneNode::postAsyncResourceOnMainThread(
				state, generation, [upload = std::move( upload )]( UISceneNode* ) { upload(); } );
		},
		[state, generation, pending]( TexturePtr texture ) {
			if ( texture ) {
				UISceneNode::postAsyncResourceOnMainThread(
					state, generation,
					[pending, texture = std::move( texture )]( UISceneNode* ) mutable {
						pending->finish( std::move( texture ) );
					} );
			}
		} );
	if ( !result )
		return {};
	return *result ? ( *result )->clone() : PendingTextureDrawable::New( pending );
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
		std::string svgKey;
		auto texture = resourceScope.findOrLoadTexture(
			fileTextureKey( path, scale, svgKey ), [&] { return loadFileTexture( path, scale ); } );
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
