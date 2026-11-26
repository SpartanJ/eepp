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

// Cache key of a web SVG rasterized at svgScale. It follows the base image's texture, which each
// web cache entry owns, so partitions holding different responses for one URL don't share it.
std::string webSvgRasterKey( const Texture& base, Float svgScale ) {
	return String::format( "web-svg:%llu:%.9g",
						   static_cast<unsigned long long>( base.getTextureId().value() ),
						   svgScale );
}

TexturePtr loadFileTexture( const std::string& path, Float svgScale ) {
	Image::FormatConfiguration format;
	format.svgScale( svgScale );
	return TextureFactory::instance()->loadFromFile( path, false, Texture::ClampMode::ClampToEdge,
													 false, false, format );
}

// Decodes an image on a worker and uploads it on the UI thread. Construct it on the UI thread: the
// loader captures renderer capabilities.
struct SourceTextureLoad {
	struct FromMemory {};

	// The data a memory loader reads. Declared before the loader, which points into it.
	std::shared_ptr<const std::string> bytes;
	std::string name;
	TextureLoader loader;
	std::once_flag preparation;
	std::exception_ptr error;

	SourceTextureLoad( const std::string& path, Float scale ) : loader( path ) {
		configure( scale );
	}

	SourceTextureLoad( FromMemory, std::shared_ptr<const std::string> data, std::string textureName,
					   Float scale ) :
		bytes( std::move( data ) ),
		name( std::move( textureName ) ),
		loader( reinterpret_cast<const unsigned char*>( bytes->data() ),
				static_cast<unsigned int>( bytes->size() ) ) {
		configure( scale );
	}

	void configure( Float scale ) {
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
		const TexturePtr& texture = loader.getTexture();
		if ( texture && !name.empty() )
			texture->setName( name );
		return texture;
	}
};

class PendingTextureDrawable final : public TextureDrawable {
  public:
	struct Load {
		TexturePtr texture;
		// A pending image usually has its drawable and one clone.
		SmallVector<std::weak_ptr<PendingTextureDrawable>, 2> drawables;

		// An empty texture means the load failed: the drawables stay empty and notify their
		// consumers, which can show a fallback such as alt text.
		void finish( TexturePtr ready ) {
			texture = std::move( ready );
			for ( const auto& drawable : drawables ) {
				if ( auto instance = drawable.lock() ) {
					instance->mLoad.reset();
					if ( texture ) {
						instance->setName( texture->getName() );
						instance->setTexture( texture );
					} else {
						instance->onResourceChange();
					}
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

// Cache key of a data URI image. Shared catalogs can serve multiple scenes: an SVG raster at one
// scale must not satisfy a request at a different scale.
std::string dataURITextureKey( const std::string& name, Float svgScale ) {
	MD5::Context hashContext;
	MD5::init( hashContext );
	MD5::update( hashContext, name.data(), name.size() );
	if ( String::startsWith( name, "data:image/svg+xml" ) )
		MD5::update( hashContext, &svgScale, sizeof( svgScale ) );
	return MD5::result( hashContext ).toHexString();
}

// Decodes the payload of an image data URI into bytes. False for other media types.
bool decodeDataURI( const std::string& name, std::string& bytes ) {
	const std::string::size_type formatAndEncSep = name.find_first_of( ',' );
	if ( formatAndEncSep == std::string::npos )
		return false;
	std::string mediaType = name.substr( 0, formatAndEncSep );
	auto parts = String::split( mediaType, ';' );
	if ( parts.empty() )
		return false;
	std::string formatName;
	auto formatNamePos = parts[0].find_first_of( '/' );
	if ( formatNamePos + 1 < mediaType.size() )
		formatName = parts[0].substr( formatNamePos + 1 );
	if ( formatName.empty() ||
		 ( !Image::isImageExtension( "." + formatName ) && formatName != "svg+xml" ) )
		return false;
	bool base64 = false;
	for ( size_t i = 1; i < parts.size() && !base64; ++i )
		base64 = parts[i] == "base64";

	std::string_view encoded = std::string_view{ name }.substr( formatAndEncSep + 1 );
	if ( base64 )
		return Base64::decode( encoded, bytes ) > 0;
	URI::decode( encoded, bytes );
	return !bytes.empty();
}

// Decodes a data URI image on the calling thread and publishes it in the scope. SVGs rasterize at
// svgScale.
TexturePtr loadDataURITexture( const std::string& name, ResourceScope& resourceScope,
							   Float svgScale ) {
	std::string key = dataURITextureKey( name, svgScale );
	if ( TexturePtr texture = resourceScope.findTexture( key ) )
		return texture;

	std::string bytes;
	if ( !decodeDataURI( name, bytes ) )
		return {};
	Image::FormatConfiguration format;
	format.svgScale( svgScale );
	TexturePtr decoded = TextureFactory::instance()->loadFromMemory(
		reinterpret_cast<const unsigned char*>( bytes.data() ), bytes.size(), false,
		Texture::ClampMode::ClampToEdge, false, false, format );
	if ( !decoded )
		return {};
	decoded->setName( key );
	resourceScope.publishLocal( key, decoded );
	return decoded;
}

DrawablePtr parseDataURI( const std::string& name, ResourceScope& resourceScope ) {
	TexturePtr texture = loadDataURITexture( name, resourceScope, PixelDensity::getPixelDensity() );
	return texture ? texture->clone() : DrawablePtr{};
}

} // namespace

DrawableResolver::DrawableResolver( UISceneNode& sceneNode ) : mSceneNode( &sceneNode ) {}

DrawableResolver::DrawableResolver( ResourceScope& resourceScope ) :
	mResourceScope( &resourceScope ) {}

bool DrawableResolver::isSvgPath( std::string_view path ) {
	return path.size() >= 4 && String::iequals( path.substr( path.size() - 4 ), ".svg" );
}

std::optional<TexturePtr> DrawableResolver::requestDeferredTexture(
	const std::string& key, bool sharedRaster,
	const std::function<ResourceScope::TextureLoad()>& createLoad,
	std::function<void( TexturePtr )> ready ) const {
	if ( !mSceneNode )
		return std::nullopt;
	auto pool = mSceneNode->getThreadPool();
	if ( pool && !pool->numThreads() )
		pool.reset();

	auto state = mSceneNode->getAsyncResourceLoadState();
	const Uint64 generation = state->generation.load( std::memory_order_acquire );
	return ResourceScope::requestTextureLoad(
		mSceneNode->getResourceScope(), key, pool.get(),
		[&] {
			auto load = createLoad();
			return ResourceScope::TextureLoad{
				[prepare = std::move( load.prepare ), state, generation] {
					// A cancelled request skips decoding. Its upload is discarded.
					if ( UISceneNode::isAsyncResourceLoadCurrent( state, generation ) )
						prepare();
				},
				std::move( load.upload ) };
		},
		[state, generation]( std::function<void()> upload ) {
			UISceneNode::postAsyncResourceOnMainThread(
				state, generation, [upload = std::move( upload )]( UISceneNode* ) { upload(); } );
		},
		[state, generation, ready = std::move( ready )]( TexturePtr texture ) mutable {
			// Failures are delivered too, as an empty texture.
			UISceneNode::postAsyncResourceOnMainThread(
				state, generation,
				[ready = std::move( ready ), texture = std::move( texture )](
					UISceneNode* ) mutable { ready( std::move( texture ) ); } );
		},
		sharedRaster );
}

TexturePtr DrawableResolver::rasterizeSvgSource( const std::string& key,
												 std::shared_ptr<const std::string> source,
												 Float svgScale,
												 std::function<void( TexturePtr )> ready ) const {
	ResourceScope& scope = mSceneNode ? *mSceneNode->getResourceScope() : *mResourceScope;
	if ( auto texture = scope.findTexture( key ) )
		return texture;
	if ( auto texture = scope.findSharedRaster( key ) )
		return texture;
	auto result = requestDeferredTexture(
		key, true,
		[&] {
			auto load = std::make_shared<SourceTextureLoad>( SourceTextureLoad::FromMemory{},
															 source, key, svgScale );
			return ResourceScope::TextureLoad{ [load] { load->prepare(); },
											   [load] { return load->upload(); } };
		},
		std::move( ready ) );
	if ( result )
		return *result;

	// Without a thread pool the source rasterizes on the calling thread.
	Image::FormatConfiguration format;
	format.svgScale( svgScale );
	TexturePtr texture = TextureFactory::instance()->loadFromMemory(
		reinterpret_cast<const unsigned char*>( source->data() ), source->size(), false,
		Texture::ClampMode::ClampToEdge, false, false, format );
	if ( !texture )
		return {};
	texture->setName( key );
	return scope.publishSharedRaster( key, std::move( texture ) );
}

TexturePtr DrawableResolver::resolveSvgRaster( const std::string& name, Float svgScale,
											   std::function<void( TexturePtr )> ready ) const {
	if ( name.empty() || !( svgScale > 0.f ) )
		return {};

	ResourceScope& scope = mSceneNode ? *mSceneNode->getResourceScope() : *mResourceScope;
	if ( String::startsWith( name, "data:image/svg+xml" ) ) {
		const std::string key = dataURITextureKey( name, svgScale );
		if ( auto texture = scope.findTexture( key ) )
			return texture;
		if ( auto texture = scope.findSharedRaster( key ) )
			return texture;
		// Decoding the URI is cheap; rasterizing it runs on a worker.
		std::string bytes;
		if ( !decodeDataURI( name, bytes ) )
			return {};
		return rasterizeSvgSource( key, std::make_shared<const std::string>( std::move( bytes ) ),
								   svgScale, std::move( ready ) );
	}

	if ( String::startsWith( name, "file://" ) ) {
		const std::string path = fileTexturePath( name );
		std::string svgKey;
		const std::string& key = fileTextureKey( path, svgScale, svgKey );
		if ( auto texture = scope.findTexture( key ) )
			return texture;
		if ( auto texture = scope.findSharedRaster( key ) )
			return texture;
		auto result = requestDeferredTexture(
			key, true,
			[&] {
				auto load = std::make_shared<SourceTextureLoad>( path, svgScale );
				return ResourceScope::TextureLoad{ [load] { load->prepare(); },
												   [load] { return load->upload(); } };
			},
			std::move( ready ) );
		// Without a thread pool the file rasterizes on the calling thread.
		return result ? *result
					  : scope.publishSharedRaster( key, loadFileTexture( path, svgScale ) );
	}

	if ( mSceneNode &&
		 ( String::startsWith( name, "http://" ) || String::startsWith( name, "https://" ) ) ) {
		// The base image keeps its SVG source in the web cache: rasterize other sizes from it
		// instead of downloading and pinning one entry per size.
		WebResourceRequest request;
		request.uri = URI( name );
		request.kind = WebResourceKind::Image;
		request.proxy = Http::getEnvProxyURI();
		// The callback runs only while the scene is current, so it uses the scene's resolver:
		// this one may be a temporary.
		mSceneNode->requestWebTexture( std::move( request ), [scene = mSceneNode, svgScale,
															  ready = std::move( ready )](
																 const WebResourceResult& result ) {
			if ( !ready || !result.success || !result.data || !result.texture )
				return;
			if ( TexturePtr texture = scene->getDrawableResolver().rasterizeSvgSource(
					 webSvgRasterKey( *result.texture, svgScale ), result.data, svgScale, ready ) )
				ready( std::move( texture ) );
		} );
		return {};
	}
	return {};
}

DrawablePtr DrawableResolver::resolveAsync( const std::string& name,
											bool firstSearchSprite ) const {
	if ( !String::startsWith( name, "file://" ) )
		return resolve( name, firstSearchSprite );

	const std::string path = fileTexturePath( name );
	const Float density =
		mSceneNode ? mSceneNode->getPixelDensity() : PixelDensity::getPixelDensity();
	const Float scale = isSvgPath( path ) ? density : 1.f;
	std::string svgKey;
	const std::string& key = fileTextureKey( path, scale, svgKey );
	ResourceScope& scope = mSceneNode ? *mSceneNode->getResourceScope() : *mResourceScope;
	if ( auto cached = scope.findTexture( key ) )
		return cached->clone();

	// Allocated only for uncached files: the drawables wait on it until the texture is ready.
	auto pending = std::make_shared<PendingTextureDrawable::Load>();
	auto result = requestDeferredTexture(
		key, false,
		[&] {
			auto load = std::make_shared<SourceTextureLoad>( path, scale );
			return ResourceScope::TextureLoad{ [load] { load->prepare(); },
											   [load] { return load->upload(); } };
		},
		[pending]( TexturePtr texture ) { pending->finish( std::move( texture ) ); } );
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
