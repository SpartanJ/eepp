#ifndef EE_GRAPHICS_RESOURCESCOPE_HPP
#define EE_GRAPHICS_RESOURCESCOPE_HPP

#include <eepp/core/small_vector.hpp>
#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/fontservice.hpp>
#include <eepp/graphics/resourcecatalog.hpp>
#include <eepp/graphics/textureregion.hpp>
#include <functional>
#include <future>
#include <optional>

namespace EE::System {

class ThreadPool;

} // namespace EE::System

namespace EE::UI {

class DrawableResolver;

} // namespace EE::UI

namespace EE { namespace Graphics {

using namespace EE::System;

class ResourceScope;
using ResourceScopePtr = ResourcePtr<ResourceScope>;

/**
 * @brief Graphics resource lookup boundary with local ownership and explicit catalog visibility.
 *
 * A scope owns resources published into its local catalog. Lookups search that local catalog first,
 * followed by imported catalogs in import order. Importing a catalog retains the catalog and makes
 * its resources visible; it does not copy resources and does not transfer them into the local
 * catalog. Consequently, removing an import only removes lookup visibility and releases the
 * scope's catalog reference.
 *
 * Use importCatalog() when resources owned by another lifetime boundary must be visible in this
 * scope, such as application assets shared with a scene or a theme catalog used by UI widgets.
 * Avoid importing unrelated scene or document catalogs: keeping imports explicit prevents name
 * collisions and resource leakage between independent documents. UISceneNode imports the default
 * resource catalog automatically unless that behavior is disabled at construction time.
 */
class EE_API ResourceScope {
  public:
	static ResourceScopePtr New();

	ResourceScope();
	~ResourceScope();

	FontService& getFontService();
	const FontService& getFontService() const;

	TexturePtr findTexture( const ResourceKey& key ) const;
	TexturePtr findTexture( const std::string& key ) const;
	/** Reuses a visible texture or shares one load and publication for this key with other callers.
	 * The first caller runs the loader. Callers for the same key wait for that result, and can
	 * retry a failed load on a later call. The loader must not reenter this method for the same
	 * key. A scene's deferred file load is finished by the calling thread when it can upload
	 * textures. A worker without a shared GL context receives an empty texture instead of
	 * waiting for the scene thread. */
	TexturePtr findOrLoadTexture( const std::string& key, const std::function<TexturePtr()>& load );

	/** Finds a texture shared only while something uses it, such as an SVG rasterized at one
	 * drawn size. */
	TexturePtr findSharedRaster( const std::string& key ) const;

	/** Shares @p texture under @p key while it is in use, without keeping it alive. If a texture
	 * shared under the key is still alive, it is returned instead. */
	TexturePtr publishSharedRaster( const std::string& key, TexturePtr texture );

	DrawablePtr findDrawableSource( const ResourceKey& key ) const;
	DrawablePtr findDrawableSource( const std::string& key ) const;
	DrawablePtr findDrawable( const std::string& name, bool firstSearchSprite = false ) const;
	DrawablePtr findDrawable( ResourceNameHash hash ) const;
	DrawablePtr findDrawable( String::HashType legacyHash ) const;
	TextureAtlasPtr findAtlas( const ResourceKey& key ) const;
	TextureAtlasPtr findAtlas( const std::string& key ) const;
	std::vector<TextureAtlasPtr> getAtlases() const;
	FontPtr findFont( const ResourceKey& key ) const;
	FontPtr findFont( const std::string& key ) const;
	FontPtr findFont( ResourceNameHash hash ) const;
	std::vector<FontPtr> getFonts() const;
	ShaderProgramPtr findShaderProgram( const ResourceKey& key ) const;
	ShaderProgramPtr findShaderProgram( const std::string& key ) const;
	std::vector<ShaderProgramPtr> getShaderPrograms() const;
	std::vector<TextureRegionPtr>
	findTextureRegionsByPattern( const std::string& name, const std::string& extension = "",
								 TextureAtlas* searchInTextureAtlas = nullptr ) const;
	std::vector<TextureRegionPtr>
	findTextureRegionsByPatternId( const String::HashType& id, const std::string& extension = "",
								   TextureAtlas* searchInTextureAtlas = nullptr ) const;

	void publishLocal( ResourceKey key, TexturePtr texture );
	void publishLocal( std::string key, TexturePtr texture );
	void publishLocalDrawable( ResourceKey key, DrawablePtr drawable );
	void publishLocalDrawable( std::string key, DrawablePtr drawable );
	void publishLocalAtlas( ResourceKey key, TextureAtlasPtr atlas );
	void publishLocalAtlas( std::string key, TextureAtlasPtr atlas );
	/**
	 * @brief Publishes a font under @p key, replacing any existing local binding for that key.
	 *
	 * The requested semantic key is preserved. Fonts are never renamed to avoid a collision.
	 * A FontTrueType instance can belong locally to only one ResourceScope because it has one
	 * FontService association. To expose the same font to another scope, import this scope's local
	 * catalog instead of publishing the same handle locally again.
	 */
	void publishLocalFont( ResourceKey key, FontPtr font );
	void publishLocalFont( std::string key, FontPtr font );
	void publishLocalShaderProgram( ResourceKey key, ShaderProgramPtr program );
	void publishLocalShaderProgram( std::string key, ShaderProgramPtr program );
	bool eraseLocal( const ResourceKey& key );
	bool eraseLocal( const std::string& key );
	bool eraseLocalDrawable( const ResourceKey& key );
	bool eraseLocalDrawable( const std::string& key );
	bool eraseLocalAtlas( const ResourceKey& key );
	bool eraseLocalAtlas( const std::string& key );
	bool eraseLocalFont( const ResourceKey& key );
	bool eraseLocalFont( const std::string& key );
	bool eraseLocalFont( Font* font );
	bool eraseLocalShaderProgram( const ResourceKey& key );
	bool eraseLocalShaderProgram( const std::string& key );
	void clearLocal();

	/**
	 * @brief Makes resources from @p catalog visible after this scope's local resources.
	 *
	 * Importing the same catalog more than once has no effect. The catalog is retained strongly for
	 * as long as it remains imported. Catalog contents stay live: resources published after this
	 * call become visible without importing the catalog again.
	 */
	void importCatalog( ResourceCatalogPtr catalog );

	/** @brief Removes an imported catalog without modifying the catalog or its resources. */
	bool removeCatalog( const ResourceCatalogPtr& catalog );

	/** @brief Removes every imported catalog while preserving this scope's local resources. */
	void clearImports();

	ResourceCatalogPtr getLocalCatalog() const;

  private:
	friend class UI::DrawableResolver;

	using TextureLoadCallback = std::function<void( TexturePtr )>;
	using PostTextureUpload = std::function<void( std::function<void()> )>;

	struct TextureLoad {
		/** Runs on a worker. Upload must prepare the source itself when it runs first. */
		std::function<void()> prepare;
		std::function<TexturePtr()> upload;
	};

	struct DeferredTextureLoad {
		DeferredTextureLoad( const ResourceScopePtr& scope, const std::string& key );

		~DeferredTextureLoad();

		std::weak_ptr<ResourceScope> scope;
		std::string key;
		std::packaged_task<TexturePtr()> upload;
		// Usually a single scene waits for a load.
		SmallVector<PostTextureUpload, 1> dispatchers;
		bool prepared{ false };
	};

	struct PendingTextureLoad {
		std::shared_future<TexturePtr> result;
		SmallVector<TextureLoadCallback, 1> callbacks;
		// Queued preparation and upload callbacks own the deferred load. If they are all
		// discarded, the load removes itself and releases its prepared data.
		std::optional<std::weak_ptr<DeferredTextureLoad>> deferred;
	};

	/** Shared raster loads publish into the cache of rasters shared while in use, instead of the
	 * local catalog. */
	static std::optional<TexturePtr>
	requestTextureLoad( const ResourceScopePtr& scope, const std::string& key, ThreadPool* pool,
						const std::function<TextureLoad()>& createLoad, PostTextureUpload dispatch,
						TextureLoadCallback ready, bool sharedRaster = false );

	/** Whether the calling thread can upload a deferred texture itself. */
	static bool canUploadTextures();

	void finishTextureLoad( const std::shared_ptr<DeferredTextureLoad>& deferred );

	void cancelUnownedTextureLoad( const std::string& key );

	std::packaged_task<TexturePtr()> textureLoadTask( const std::string& key,
													  std::function<TexturePtr()> load,
													  bool sharedRaster = false );

	void runTextureLoad( const std::string& key, std::packaged_task<TexturePtr()> task,
						 const std::shared_future<TexturePtr>& result );

	void attachFontService( const FontPtr& font );
	void detachFontService( const FontPtr& font );

	ResourceCatalogPtr mLocalCatalog;
	FontService mFontService;
	// Scenes import the default local catalog and the default theme catalog.
	SmallVector<ResourceCatalogPtr, 2> mImports;
	UnorderedMap<std::string, PendingTextureLoad> mPendingTextureLoads;
	UnorderedMap<std::string, std::weak_ptr<Texture>> mSharedRasters;
	mutable Mutex mMutex;
};

/** Engine-owned process defaults for pure Graphics and application-wide resolution. */
EE_API ResourceCatalog& globalResourceCatalog();
EE_API ResourceScope& defaultResourceScope();

}} // namespace EE::Graphics

#endif
