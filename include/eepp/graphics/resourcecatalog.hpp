#ifndef EE_GRAPHICS_RESOURCECATALOG_HPP
#define EE_GRAPHICS_RESOURCECATALOG_HPP

#include <eepp/core/containers.hpp>
#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/resource.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/system/mutex.hpp>

namespace EE { namespace Graphics {

class ResourceCatalog;
using ResourceCatalogPtr = ResourcePtr<ResourceCatalog>;

/** Strong, named ownership for Graphics resources. The live registry is never searched here. */
class EE_API ResourceCatalog {
  public:
	static ResourceCatalogPtr New();

	void publish( ResourceKey key, TexturePtr texture );
	void publish( std::string key, TexturePtr texture );
	void publishDrawable( ResourceKey key, DrawablePtr drawable );
	void publishDrawable( std::string key, DrawablePtr drawable );

	TexturePtr findTexture( const ResourceKey& key ) const;
	TexturePtr findTexture( const std::string& key ) const;
	DrawablePtr findDrawable( const ResourceKey& key ) const;
	DrawablePtr findDrawable( const std::string& key ) const;

	bool erase( const ResourceKey& key );
	bool erase( const std::string& key );
	bool eraseDrawable( const ResourceKey& key );
	bool eraseDrawable( const std::string& key );
	void clear();
	std::size_t size() const;

  private:
	mutable System::Mutex mMutex;
	UnorderedMap<std::string, TexturePtr> mTextures;
	UnorderedMap<std::string, DrawablePtr> mDrawables;
};

}} // namespace EE::Graphics

#endif
