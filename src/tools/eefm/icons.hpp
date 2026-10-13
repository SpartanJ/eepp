#ifndef EEFM_ICONS_HPP
#define EEFM_ICONS_HPP

#include <eepp/ui/uiscenenode.hpp>

namespace eefm {

/** Loads the icon fonts shared with ecode (remixicon, nonicons, codicons), installs the base
 * IconManager theme and adds the icons specific to eefm (places, devices, file types). */
void loadIcons( EE::UI::UISceneNode* ui, const std::string& assetsPath );

} // namespace eefm

#endif
