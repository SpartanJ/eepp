#ifndef EE_UI_DRAWABLERESOLVER_HPP
#define EE_UI_DRAWABLERESOLVER_HPP

#include <eepp/graphics/drawable.hpp>
#include <eepp/graphics/resourcescope.hpp>

namespace EE { namespace UI {

class UISceneNode;

/** Resolves named drawables through an explicit UI scene or Graphics resource scope. */
class EE_API DrawableResolver {
  public:
	explicit DrawableResolver( UISceneNode& sceneNode );
	explicit DrawableResolver( Graphics::ResourceScope& resourceScope );

	Graphics::DrawablePtr resolve( const std::string& name, bool firstSearchSprite = false ) const;
	Graphics::DrawablePtr resolveById( Graphics::ResourceNameHash hash ) const;
	Graphics::DrawablePtr resolveById( String::HashType legacyHash ) const;

	void setPrintWarnings( bool printWarnings );
	bool getPrintWarnings() const;

  protected:
	UISceneNode* mSceneNode{ nullptr };
	Graphics::ResourceScope* mResourceScope{ nullptr };
	bool mPrintWarnings{ false };
};

}} // namespace EE::UI

#endif
