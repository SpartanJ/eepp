#ifndef EEFM_ICONS_HPP
#define EEFM_ICONS_HPP

#include <eepp/ui/uiscenenode.hpp>
#include <string>

namespace eefm {

class Icons {
  public:
	/** Loads the icon fonts shared with ecode (remixicon, nonicons, codicons), installs the base
	 * IconManager theme and adds the icons specific to eefm (places, devices, file types).
	 * Caches the window icon as "eefm" for menu reuse. */
	static void load( EE::UI::UISceneNode* ui, const std::string& assetsPath,
					  const std::string& windowIconPath );
};

} // namespace eefm

#endif
