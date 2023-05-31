#ifndef EE_UI_UIBUILDSETTINGS_HPP
#define EE_UI_UIBUILDSETTINGS_HPP

#include "projectbuild.hpp"
#include <eepp/ui/uidatabind.hpp>
#include <eepp/ui/uirelativelayout.hpp>

using namespace EE::UI;

namespace ecode {

class UIBuildSettings : public UIRelativeLayout {
  public:
	static UIBuildSettings* New( ProjectBuild& build, ProjectBuildConfiguration& config );

  protected:
	ProjectBuild& mBuild;
	ProjectBuildConfiguration& mConfig;
	UIDataBindHolder mDataBindHolder;

	explicit UIBuildSettings( ProjectBuild& build, ProjectBuildConfiguration& config );

	void updateOS();
};

} // namespace ecode

#endif // EE_UI_UIBUILDSETTINGS_HPP
