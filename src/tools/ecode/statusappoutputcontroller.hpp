#ifndef ECODE_STATUSAPPOUTPUTCONTROLLER_HPP
#define ECODE_STATUSAPPOUTPUTCONTROLLER_HPP

#include "projectbuild.hpp"
#include "statusbuildoutputcontroller.hpp"
#include "uistatusbar.hpp"
#include <eepp/system/luapattern.hpp>
#include <eepp/ui/tools/uicodeeditorsplitter.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uirelativelayout.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uisplitter.hpp>
#include <eepp/ui/uitableview.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::Tools;

namespace ecode {

class App;

class StatusAppOutputController : public StatusBarElement {
  public:
	StatusAppOutputController( UISplitter* mainSplitter, UISceneNode* uiSceneNode, App* app );

	virtual ~StatusAppOutputController(){};

	void run( const ProjectBuildCommand& runData, const ProjectBuildOutputParser& outputParser );

	UIWidget* getWidget();

	UIWidget* createWidget();

	UICodeEditor* getContainer();

	UIPushButton* getRunButton( App* app );

  protected:
	UILinearLayout* mContainer{ nullptr };
	UICodeEditor* mAppOutput{ nullptr };
	std::vector<PatternHolder> mPatternHolder;
	UIPushButton* mClearButton{ nullptr };
	UIPushButton* mRunButton{ nullptr };
	UIPushButton* mStopButton{ nullptr };
	UIPushButton* mFindButton{ nullptr };
	UIPushButton* mConfigureButton{ nullptr };

	bool mScrollLocked{ true };

	void createContainer();
};

} // namespace ecode

#endif // ECODE_StatusAppOutputController_HPP
