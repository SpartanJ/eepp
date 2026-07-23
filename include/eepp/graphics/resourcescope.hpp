#ifndef EE_GRAPHICS_RESOURCESCOPE_HPP
#define EE_GRAPHICS_RESOURCESCOPE_HPP

#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/resourcecatalog.hpp>

namespace EE { namespace Graphics {

class ResourceScope;
using ResourceScopePtr = ResourcePtr<ResourceScope>;

/** Graphics-only semantic lookup boundary with a local catalog and explicit catalog imports. */
class EE_API ResourceScope {
  public:
	static ResourceScopePtr New();

	ResourceScope();

	TexturePtr findTexture( const ResourceKey& key ) const;
	TexturePtr findTexture( const std::string& key ) const;
	DrawablePtr findDrawableSource( const ResourceKey& key ) const;
	DrawablePtr findDrawableSource( const std::string& key ) const;
	DrawablePtr findDrawable( const std::string& name, bool firstSearchSprite = false ) const;
	DrawablePtr findDrawable( const Uint32& id ) const;

	void publishLocal( ResourceKey key, TexturePtr texture );
	void publishLocal( std::string key, TexturePtr texture );
	void publishLocalDrawable( ResourceKey key, DrawablePtr drawable );
	void publishLocalDrawable( std::string key, DrawablePtr drawable );
	bool eraseLocal( const ResourceKey& key );
	bool eraseLocal( const std::string& key );
	bool eraseLocalDrawable( const ResourceKey& key );
	bool eraseLocalDrawable( const std::string& key );
	void clearLocal();

	void importCatalog( ResourceCatalogPtr catalog );
	bool removeCatalog( const ResourceCatalogPtr& catalog );
	void clearImports();

	ResourceCatalogPtr getLocalCatalog() const;

  private:
	ResourceCatalogPtr mLocalCatalog;
	std::vector<ResourceCatalogPtr> mImports;
	mutable System::Mutex mMutex;
};

/** Engine-owned process defaults for pure Graphics and legacy application-wide resolution. */
EE_API ResourceCatalog& globalResourceCatalog();
EE_API ResourceScope& defaultResourceScope();

}} // namespace EE::Graphics

#endif
