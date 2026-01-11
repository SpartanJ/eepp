#ifndef ECODE_STATUSTERMINALCONTROLLER_HPP
#define ECODE_STATUSTERMINALCONTROLLER_HPP

#include "uistatusbar.hpp"
#include "widgetcommandexecuter.hpp"

#include <eepp/ui/tools/uicodeeditorsplitter.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uisplitter.hpp>
#include <eterm/ui/uiterminal.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::Tools;
using namespace eterm::UI;

namespace ecode {

class App;

class StatusTerminalController : public StatusBarElement {
  public:
	StatusTerminalController( UISplitter* mainSplitter, UISceneNode* uiSceneNode, App* app );

	virtual ~StatusTerminalController() {}

	UIWidget* getWidget();

	UIWidget* createWidget();

	UITerminal* getUITerminal();

	UITabWidget* getTabWidget();

	UITerminal* createTerminal( const std::string& workingDir = "", std::string program = "",
								std::vector<std::string> args = {},
								const std::unordered_map<std::string, std::string>& env = {} );

  protected:
	App* mApp;

	UIHLinearLayoutCommandExecuter* mContainer{ nullptr };
	UITabWidget* mTabWidget{ nullptr };

	UIPushButton* mAddBtn{ nullptr };

	UITerminal* mUITerminal{ nullptr };

	UIHLinearLayoutCommandExecuter* createContainer();

	bool tryTabClose( UITab* tab );
};

} // namespace ecode

#endif // ECODE_STATUSTERMINALCONTROLLER_HPP
