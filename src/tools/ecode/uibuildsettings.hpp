#ifndef EE_UI_UIBUILDSETTINGS_HPP
#define EE_UI_UIBUILDSETTINGS_HPP

#include "projectbuild.hpp"
#include <eepp/ui/uirelativelayout.hpp>
#include <eepp/ui/uidatabind.hpp>

using namespace EE::UI;

namespace ecode {

class UIBuildSettings : public UIRelativeLayout {
  public:
	static UIBuildSettings* New( ProjectBuild& build );

  protected:
	ProjectBuild& mBuild;
	UIDataBindHolder mDataBindHolder;

	explicit UIBuildSettings( ProjectBuild& build );

	void updateOS();
};

} // namespace ecode

#endif // EE_UI_UIBUILDSETTINGS_HPP
