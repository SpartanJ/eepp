#ifndef EE_GRAPHICS_RESOURCESCOPE_HPP
#define EE_GRAPHICS_RESOURCESCOPE_HPP

#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/fontservice.hpp>
#include <eepp/graphics/resourcecatalog.hpp>
#include <eepp/graphics/textureregion.hpp>

namespace EE { namespace Graphics {

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
	DrawablePtr findDrawableSource( const ResourceKey& key ) const;
	DrawablePtr findDrawableSource( const std::string& key ) const;
	DrawablePtr findDrawable( const std::string& name, bool firstSearchSprite = false ) const;
	DrawablePtr findDrawable( const Uint32& id ) const;
	TextureAtlasPtr findAtlas( const ResourceKey& key ) const;
	TextureAtlasPtr findAtlas( const std::string& key ) const;
	std::vector<TextureAtlasPtr> getAtlases() const;
	FontPtr findFont( const ResourceKey& key ) const;
	FontPtr findFont( const std::string& key ) const;
	FontPtr findFont( const String::HashType& id ) const;
	std::vector<FontPtr> getFonts() const;
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
	 */
	void publishLocalFont( ResourceKey key, FontPtr font );
	void publishLocalFont( std::string key, FontPtr font );
	bool eraseLocal( const ResourceKey& key );
	bool eraseLocal( const std::string& key );
	bool eraseLocalDrawable( const ResourceKey& key );
	bool eraseLocalDrawable( const std::string& key );
	bool eraseLocalAtlas( const ResourceKey& key );
	bool eraseLocalAtlas( const std::string& key );
	bool eraseLocalFont( const ResourceKey& key );
	bool eraseLocalFont( const std::string& key );
	bool eraseLocalFont( Font* font );
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
	void attachFontService( const FontPtr& font );
	void detachFontService( const FontPtr& font );

	ResourceCatalogPtr mLocalCatalog;
	FontService mFontService;
	std::vector<ResourceCatalogPtr> mImports;
	mutable System::Mutex mMutex;
};

/** Engine-owned process defaults for pure Graphics and application-wide resolution. */
EE_API ResourceCatalog& globalResourceCatalog();
EE_API ResourceScope& defaultResourceScope();

}} // namespace EE::Graphics

#endif
