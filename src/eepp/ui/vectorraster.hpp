#ifndef EE_UI_VECTORRASTER_HPP
#define EE_UI_VECTORRASTER_HPP

#include <eepp/graphics/drawableresource.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/texturedrawable.hpp>
#include <eepp/math/size.hpp>
#include <eepp/scene/action.hpp>
#include <functional>
#include <memory>
#include <string>

namespace EE { namespace UI {

using namespace EE::Graphics;
using namespace EE::Math;
using namespace EE::Scene;

class UINode;

/** Keeps a consumer's SVG raster at the size it is drawn. The consumer owns the helper and reports
 * its destination size; when its current TextureDrawable draws an SVG raster at another size, the
 * helper rasterizes the source again and retargets that drawable at the new texture. */
class VectorRaster {
  public:
	/** Returns the consumer's current drawable, or null when it is not a TextureDrawable. The
	 * consumer can replace its drawable at any time; the helper asks whenever it needs it. */
	using DrawableAccess = std::function<TextureDrawable*()>;

	/** Rasters that would be larger than this, per side, keep their current resolution. It stays
	 * within the texture size every supported GPU accepts. */
	static constexpr Float MaxRasterSize = 4096.f;

	/** @return A helper for @p owner, or nothing when @p source cannot name an SVG: only web URLs,
	 * SVG data URIs and .svg paths can. */
	static std::unique_ptr<VectorRaster> create( UINode* owner, std::string source,
												 DrawableAccess currentDrawable );

	/** @return @p drawable as a TextureDrawable, or null. For consumers' DrawableAccess. */
	static TextureDrawable* textureDrawableOf( const DrawablePtr& drawable );

	VectorRaster( UINode* owner, std::string source, DrawableAccess currentDrawable );

	~VectorRaster();

	VectorRaster( const VectorRaster& ) = delete;

	VectorRaster& operator=( const VectorRaster& ) = delete;

	/** Requests a raster at @p destinationPixels when the drawable draws an SVG raster of another
	 * size. Requests run on the next scene update; later size changes are debounced. The raster is
	 * decoded on the scene's thread pool or downloaded, then applied on the UI thread. */
	void update( const Sizef& destinationPixels );

  private:
	Action::UniqueID firstTag() const;

	Action::UniqueID resizeTag() const;

	void request( Float svgScale, Sizef destinationPixels );

	void cancelPending();

	void apply( TexturePtr texture, const Sizef& destinationPixels );

	UINode* mOwner;
	DrawableAccess mCurrentDrawable;
	std::string mSource;
	// The applied raster stays alive while the consumer rebuilds its drawable at the base scale.
	TexturePtr mRaster;
	// Rasters arrive later on the UI thread: through this token, results for a destroyed helper
	// are dropped.
	std::shared_ptr<VectorRaster*> mLifetime;
	// Only the latest request applies; earlier sizes arriving late are dropped.
	Uint64 mRequestId{ 0 };
	// Only the latest scheduled request runs.
	Uint64 mScheduledId{ 0 };
	// The destination mRaster was made for.
	Sizef mRasterDestination;
	Sizef mRequestedSize;
	bool mHasRequested{ false };
};

}} // namespace EE::UI

#endif
