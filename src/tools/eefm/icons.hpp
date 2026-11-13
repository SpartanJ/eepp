#ifndef EEFM_ICONS_HPP
#define EEFM_ICONS_HPP

#include <eepp/graphics/drawable.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <string>

namespace eefm {

using namespace EE::UI;

class Icons {
  public:
	/** Loads the icon fonts shared with ecode (remixicon, nonicons, codicons), installs the base
	 * IconManager theme and adds the icons specific to eefm (places, devices, file types).
	 * Caches the window icon as "eefm" for menu reuse. */
	static void load( UISceneNode* ui, const std::string& assetsPath,
					  const std::string& windowIconPath );

	/** Updates the tint-owning widget's color_icon class, including for color glyphs. */
	static void updateColorClass( UIWidget* widget, const DrawablePtr& drawable );
};

} // namespace eefm

#endif
