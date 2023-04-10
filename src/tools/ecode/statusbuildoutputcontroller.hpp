#ifndef ECODE_STATUSBUILDOUTPUTCONTROLLER_HPP
#define ECODE_STATUSBUILDOUTPUTCONTROLLER_HPP

#include "projectbuild.hpp"
#include <eepp/ui/tools/uicodeeditorsplitter.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uisplitter.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::Tools;

namespace ecode {

class App;

class StatusBuildOutputController {
  public:
	StatusBuildOutputController( UISplitter* mainSplitter, UISceneNode* uiSceneNode, App* app );

	void toggle();

	void hide();

	void show();

	void run( const std::string& buildName, const std::string& buildType,
			  const ProjectBuildOutputParser& outputParser = {} );

	UICodeEditor* getContainer();

  protected:
	UISplitter* mMainSplitter{ nullptr };
	UISceneNode* mUISceneNode{ nullptr };
	App* mApp{ nullptr };
	UICodeEditorSplitter* mSplitter{ nullptr };
	UICodeEditor* mContainer{ nullptr };

	UICodeEditor* createContainer();

	UIPushButton* getBuildButton( App* app );
};

} // namespace ecode

#endif // ECODE_STATUSBUILDOUTPUTCONTROLLER_HPP
