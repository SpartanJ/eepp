#ifndef ECODE_STATUSTERMINALCONTROLLER_HPP
#define ECODE_STATUSTERMINALCONTROLLER_HPP

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

class StatusTerminalController {
  public:
	StatusTerminalController( UISplitter* mainSplitter, UISceneNode* uiSceneNode, App* app );

	void toggle();

	void hide();

	void show();

	UITerminal* getUITerminal();

  protected:
	UISplitter* mMainSplitter{ nullptr };
	UISceneNode* mUISceneNode{ nullptr };
	App* mApp{ nullptr };
	UITerminal* mUITerminal{ nullptr };
	UICodeEditorSplitter* mSplitter{ nullptr };

	UITerminal* createTerminal( const std::string& workingDir = "", std::string program = "",
								const std::vector<std::string>& args = {} );
};

} // namespace ecode

#endif // ECODE_STATUSTERMINALCONTROLLER_HPP
