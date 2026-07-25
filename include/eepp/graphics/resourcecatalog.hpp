#ifndef EE_GRAPHICS_RESOURCECATALOG_HPP
#define EE_GRAPHICS_RESOURCECATALOG_HPP

#include <eepp/core/containers.hpp>
#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/resource.hpp>
#include <eepp/graphics/shaderprogram.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/textureatlas.hpp>
#include <eepp/system/mutex.hpp>

namespace EE { namespace Graphics {

class ResourceCatalog;
using ResourceCatalogPtr = ResourcePtr<ResourceCatalog>;

/**
 * @brief Thread-safe, strongly owning collection of named graphics resources.
 *
 * A catalog maps semantic string keys to textures, drawable sources, texture atlases, and fonts.
 * Publishing a resource gives the catalog shared ownership of it. Publishing another resource of
 * the same kind under the same key replaces that binding; the key and resource name are never
 * changed automatically. Publishing a null handle is equivalent to erasing the corresponding key,
 * while an empty key is ignored.
 *
 * Lookups return owning handles. A resource obtained from a catalog therefore remains alive even
 * if its binding is subsequently replaced, erased, or the catalog is cleared. Final handles are
 * released after dropping the catalog mutex so resource destruction and callbacks never execute
 * while the catalog is locked.
 *
 * Drawable and font hash lookups use String::hash() of the semantic key as a weak secondary index.
 * They are compatibility/convenience lookups and do not replace full-key lookup when collision-safe
 * identity is required. Textures use ResourceId for process-wide object identity elsewhere; the
 * hash accepted by findDrawable() and findFont() is not a ResourceId.
 *
 * ResourceCatalog performs no parent, scene, live-registry, filesystem, or fallback search. A
 * ResourceScope defines lookup precedence by searching its local catalog and then explicitly
 * imported catalogs. Importing a catalog shares this object and its live contents; it does not copy
 * any resource.
 *
 * All public operations are safe to call concurrently. Enumeration methods return snapshots and
 * allocate vectors containing owning handles.
 */
class EE_API ResourceCatalog {
  public:
	/** @return A new empty catalog using eepp resource allocation and deletion. */
	static ResourceCatalogPtr New();

	/** @brief Publishes or replaces a texture binding. A null texture erases @p key. */
	void publish( ResourceKey key, TexturePtr texture );
	/** @copydoc publish(ResourceKey,TexturePtr) */
	void publish( std::string key, TexturePtr texture );

	/** @brief Publishes or replaces a drawable-source binding. A null drawable erases @p key. */
	void publishDrawable( ResourceKey key, DrawablePtr drawable );
	/** @copydoc publishDrawable(ResourceKey,DrawablePtr) */
	void publishDrawable( std::string key, DrawablePtr drawable );

	/** @brief Publishes or replaces a texture-atlas binding. A null atlas erases @p key. */
	void publishAtlas( ResourceKey key, TextureAtlasPtr atlas );
	/** @copydoc publishAtlas(ResourceKey,TextureAtlasPtr) */
	void publishAtlas( std::string key, TextureAtlasPtr atlas );

	/** @brief Publishes or replaces a font binding. A null font erases @p key. */
	void publishFont( ResourceKey key, FontPtr font );
	/** @copydoc publishFont(ResourceKey,FontPtr) */
	void publishFont( std::string key, FontPtr font );

	/** @brief Publishes or replaces a shader-program binding. A null program erases @p key. */
	void publishShaderProgram( ResourceKey key, ShaderProgramPtr program );
	/** @copydoc publishShaderProgram(ResourceKey,ShaderProgramPtr) */
	void publishShaderProgram( std::string key, ShaderProgramPtr program );

	/** @return The texture bound to @p key, or an empty handle when it is not present. */
	TexturePtr findTexture( const ResourceKey& key ) const;
	/** @copydoc findTexture(const ResourceKey&)const */
	TexturePtr findTexture( const std::string& key ) const;

	/** @return The drawable source bound to @p key, or an empty handle when it is not present. */
	DrawablePtr findDrawable( const ResourceKey& key ) const;
	/** @copydoc findDrawable(const ResourceKey&)const */
	DrawablePtr findDrawable( const std::string& key ) const;
	/**
	 * @brief Looks up a drawable through the weak String::hash(key) secondary index.
	 * @return An owning handle when the indexed drawable is still present, otherwise an empty
	 * handle.
	 */
	DrawablePtr findDrawable( const String::HashType& id ) const;

	/** @return The texture atlas bound to @p key, or an empty handle when it is not present. */
	TextureAtlasPtr findAtlas( const ResourceKey& key ) const;
	/** @copydoc findAtlas(const ResourceKey&)const */
	TextureAtlasPtr findAtlas( const std::string& key ) const;
	/** @return An owning snapshot of all texture atlases currently published in this catalog. */
	std::vector<TextureAtlasPtr> getAtlases() const;

	/** @return The font bound to @p key, or an empty handle when it is not present. */
	FontPtr findFont( const ResourceKey& key ) const;
	/** @copydoc findFont(const ResourceKey&)const */
	FontPtr findFont( const std::string& key ) const;
	/**
	 * @brief Looks up a font through the weak String::hash(key) secondary index.
	 * @return An owning handle when the indexed font is still present, otherwise an empty handle.
	 */
	FontPtr findFont( const String::HashType& id ) const;
	/** @return An owning snapshot of all fonts currently published in this catalog. */
	std::vector<FontPtr> getFonts() const;

	/** @return The shader program bound to @p key, or an empty handle when absent. */
	ShaderProgramPtr findShaderProgram( const ResourceKey& key ) const;
	/** @copydoc findShaderProgram(const ResourceKey&)const */
	ShaderProgramPtr findShaderProgram( const std::string& key ) const;
	/** @return An owning snapshot of all shader programs published in this catalog. */
	std::vector<ShaderProgramPtr> getShaderPrograms() const;

	/** @brief Removes the texture binding for @p key. @return Whether a binding was removed. */
	bool erase( const ResourceKey& key );
	/** @copydoc erase(const ResourceKey&) */
	bool erase( const std::string& key );

	/** @brief Removes the drawable binding for @p key. @return Whether a binding was removed. */
	bool eraseDrawable( const ResourceKey& key );
	/** @copydoc eraseDrawable(const ResourceKey&) */
	bool eraseDrawable( const std::string& key );

	/** @brief Removes the texture-atlas binding for @p key. @return Whether a binding was removed.
	 */
	bool eraseAtlas( const ResourceKey& key );
	/** @copydoc eraseAtlas(const ResourceKey&) */
	bool eraseAtlas( const std::string& key );

	/** @brief Removes the font binding for @p key. @return Whether a binding was removed. */
	bool eraseFont( const ResourceKey& key );
	/** @copydoc eraseFont(const ResourceKey&) */
	bool eraseFont( const std::string& key );
	/**
	 * @brief Removes @p font only when its current name maps to that exact font in this catalog.
	 * @return Whether the matching binding was removed.
	 */
	bool eraseFont( Font* font );

	/** @brief Removes the shader-program binding for @p key. */
	bool eraseShaderProgram( const ResourceKey& key );
	/** @copydoc eraseShaderProgram(const ResourceKey&) */
	bool eraseShaderProgram( const std::string& key );

	/** @brief Removes every binding while allowing previously returned handles to remain valid. */
	void clear();

	/** @return The total number of bindings of every supported resource kind. */
	std::size_t size() const;

  private:
	mutable System::Mutex mMutex;
	UnorderedMap<std::string, TexturePtr> mTextures;
	UnorderedMap<std::string, DrawablePtr> mDrawables;
	UnorderedMap<String::HashType, DrawableWeakPtr> mDrawablesById;
	UnorderedMap<std::string, TextureAtlasPtr> mAtlases;
	UnorderedMap<std::string, FontPtr> mFonts;
	UnorderedMap<String::HashType, FontWeakPtr> mFontsById;
	UnorderedMap<std::string, ShaderProgramPtr> mShaderPrograms;
};

}} // namespace EE::Graphics

#endif
