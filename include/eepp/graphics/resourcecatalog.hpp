#ifndef EE_GRAPHICS_RESOURCECATALOG_HPP
#define EE_GRAPHICS_RESOURCECATALOG_HPP

#include <eepp/core/containers.hpp>
#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/resource.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/textureatlas.hpp>
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
	void publishAtlas( ResourceKey key, TextureAtlasPtr atlas );
	void publishAtlas( std::string key, TextureAtlasPtr atlas );

	TexturePtr findTexture( const ResourceKey& key ) const;
	TexturePtr findTexture( const std::string& key ) const;
	DrawablePtr findDrawable( const ResourceKey& key ) const;
	DrawablePtr findDrawable( const std::string& key ) const;
	DrawablePtr findDrawable( const String::HashType& id ) const;
	TextureAtlasPtr findAtlas( const ResourceKey& key ) const;
	TextureAtlasPtr findAtlas( const std::string& key ) const;
	std::vector<TextureAtlasPtr> getAtlases() const;

	bool erase( const ResourceKey& key );
	bool erase( const std::string& key );
	bool eraseDrawable( const ResourceKey& key );
	bool eraseDrawable( const std::string& key );
	bool eraseAtlas( const ResourceKey& key );
	bool eraseAtlas( const std::string& key );
	void clear();
	std::size_t size() const;

  private:
	mutable System::Mutex mMutex;
	UnorderedMap<std::string, TexturePtr> mTextures;
	UnorderedMap<std::string, DrawablePtr> mDrawables;
	UnorderedMap<String::HashType, DrawableWeakPtr> mDrawablesById;
	UnorderedMap<std::string, TextureAtlasPtr> mAtlases;
};

}} // namespace EE::Graphics

#endif
