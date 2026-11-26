#ifndef EE_UI_DRAWABLERESOLVER_HPP
#define EE_UI_DRAWABLERESOLVER_HPP

#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/resourcescope.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>

namespace EE { namespace UI {

using namespace EE::Graphics;

class UISceneNode;

/** Resolves named drawables through an explicit UI scene or Graphics resource scope. */
class EE_API DrawableResolver {
  public:
	explicit DrawableResolver( UISceneNode& sceneNode );
	explicit DrawableResolver( ResourceScope& resourceScope );

	/** Resolves local files synchronously, including files whose load is already in progress. */
	DrawablePtr resolve( const std::string& name, bool firstSearchSprite = false ) const;

	/** Returns a cached local texture or an empty drawable that updates on the UI thread. If the
	 * load fails, the drawable stays empty and notifies its resource-change listeners.
	 * An uncached local file requires a scene with a thread pool. Other sources use resolve(). */
	DrawablePtr resolveAsync( const std::string& name, bool firstSearchSprite = false ) const;

	/** Rasterizes the SVG at @p name (a file:// or web URL, or an SVG data URI) at @p svgScale
	 * texture pixels per SVG unit. Returns the raster when it is already available. Otherwise
	 * returns nothing and calls @p ready on the UI thread once the raster is decoded on the scene's
	 * thread pool. Web SVGs rasterize from the source their base image keeps in the web cache.
	 * Without a thread pool, rasters are made at once. Rasters at scales other than the scene
	 * density are shared only while in use. */
	TexturePtr resolveSvgRaster( const std::string& name, Float svgScale,
								 std::function<void( TexturePtr )> ready ) const;

	static bool isSvgPath( std::string_view path );
	DrawablePtr resolveById( ResourceNameHash hash ) const;
	DrawablePtr resolveById( String::HashType legacyHash ) const;

	void setPrintWarnings( bool printWarnings );
	bool getPrintWarnings() const;

  protected:
	UISceneNode* mSceneNode{ nullptr };
	ResourceScope* mResourceScope{ nullptr };
	bool mPrintWarnings{ false };

  private:
	/** Decodes on the scene's thread pool and uploads on the UI thread. Returns the texture when it
	 * is already loaded, an empty texture while it loads, and nothing without a scene or pool.
	 * Calls @p ready on the UI thread with the texture, or with an empty one if loading failed. */
	std::optional<TexturePtr>
	requestDeferredTexture( const std::string& key, bool sharedRaster,
							const std::function<ResourceScope::TextureLoad()>& createLoad,
							std::function<void( TexturePtr )> ready ) const;

	/** Rasterizes an SVG source held in memory, as requestDeferredTexture() does, into the
	 * cache of rasters shared while in use, under @p key. Rasterizes at once without a pool. */
	TexturePtr rasterizeSvgSource( const std::string& key,
								   std::shared_ptr<const std::string> source, Float svgScale,
								   std::function<void( TexturePtr )> ready ) const;
};

}} // namespace EE::UI

#endif
