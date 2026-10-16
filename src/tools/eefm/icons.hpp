#ifndef EEFM_ICONS_HPP
#define EEFM_ICONS_HPP

#include <eepp/ui/uiscenenode.hpp>
#include <string>

namespace eefm {

class Icons {
  public:
	/** Loads the icon fonts shared with ecode (remixicon, nonicons, codicons), installs the base
	 * IconManager theme and adds the icons specific to eefm (places, devices, file types). */
	static void load( EE::UI::UISceneNode* ui, const std::string& assetsPath );
};

} // namespace eefm

#endif
