#ifndef ECODE_STATUSBUILDOUTPUTCONTROLLER_HPP
#define ECODE_STATUSBUILDOUTPUTCONTROLLER_HPP

#include "projectbuild.hpp"
#include "widgetcommandexecuter.hpp"
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

struct StatusMessage {
	ProjectOutputParserTypes type;
	String output;
	String message;
	std::string file;
	std::string fileName;
	Int64 line{ 0 };
	Int64 col{ 0 };
};

struct PatternHolder {
	LuaPattern pattern;
	ProjectBuildOutputParserConfig config;
};

class StatusBuildOutputController {
  public:
	StatusBuildOutputController( UISplitter* mainSplitter, UISceneNode* uiSceneNode, App* app );

	void toggle();

	void hide();

	void show();

	void runBuild( const std::string& buildName, const std::string& buildType,
				   const ProjectBuildOutputParser& outputParser = {} );

	void runClean( const std::string& buildName, const std::string& buildType,
				   const ProjectBuildOutputParser& outputParser = {} );

	UICodeEditor* getContainer();

	void showIssues();

	void showBuildOutput();

  protected:
	UISplitter* mMainSplitter{ nullptr };
	UISceneNode* mUISceneNode{ nullptr };
	App* mApp{ nullptr };
	UICodeEditorSplitter* mSplitter{ nullptr };

	UIRelativeLayoutCommandExecuter* mContainer{ nullptr };
	UICodeEditor* mBuildOutput{ nullptr };
	UISelectButton* mButOutput{ nullptr };
	UISelectButton* mButIssues{ nullptr };
	UITableView* mTableIssues{ nullptr };

	std::vector<StatusMessage> mStatusResults;
	std::vector<PatternHolder> mPatternHolder;
	std::string mCurLineBuffer;

	void createContainer();

	UIPushButton* getBuildButton( App* app );

	UIPushButton* getCleanButton( App* app );

	bool searchFindAndAddStatusResult( const std::vector<PatternHolder>& patterns,
									   const std::string& text, const ProjectBuildCommand* cmd );

	void onLoadDone( const Variant& lineNum, const Variant& colNum );

	void setHeaderWidth();
};

} // namespace ecode

#endif // ECODE_STATUSBUILDOUTPUTCONTROLLER_HPP
