#ifndef EE_UI_DRAWABLERESOLVER_HPP
#define EE_UI_DRAWABLERESOLVER_HPP

#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/resourcescope.hpp>
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

	/** Returns a cached local texture or an empty drawable that updates on the UI thread.
	 * An uncached local file requires a scene with a thread pool. Other sources use resolve(). */
	DrawablePtr resolveAsync( const std::string& name, bool firstSearchSprite = false ) const;

	static bool isSvgPath( std::string_view path );
	DrawablePtr resolveById( ResourceNameHash hash ) const;
	DrawablePtr resolveById( String::HashType legacyHash ) const;

	void setPrintWarnings( bool printWarnings );
	bool getPrintWarnings() const;

  protected:
	UISceneNode* mSceneNode{ nullptr };
	ResourceScope* mResourceScope{ nullptr };
	bool mPrintWarnings{ false };
};

}} // namespace EE::UI

#endif
