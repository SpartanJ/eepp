#ifndef GLOBALSEARCHCONTROLLER_HPP
#define GLOBALSEARCHCONTROLLER_HPP

#include "projectsearch.hpp"
#include <eepp/ee.hpp>

class App;
class UIGlobalSearchBar;
class UITreeViewGlobalSearch;

class GlobalSearchController {
  public:
	GlobalSearchController( UICodeEditorSplitter*, UISceneNode*, App* );

	void updateGlobalSearchBar();

	void initGlobalSearchBar( UIGlobalSearchBar* globalSearchBar );

	void hideGlobalSearchBar();

	void updateGlobalSearchBarResults( const std::string& search,
									   std::shared_ptr<ProjectSearch::ResultModel> model,
									   bool searchReplace );

	void initGlobalSearchTree( UITreeViewGlobalSearch* searchTree );

	void doGlobalSearch( const String& text, bool caseSensitive, bool wholeWord, bool luaPattern,
						 bool searchReplace, bool searchAgain = false );

	size_t replaceInFiles( const String& replaceText,
						   std::shared_ptr<ProjectSearch::ResultModel> model );

	void showGlobalSearch( bool searchAndReplace = false );

	void updateColorScheme( const SyntaxColorScheme& colorScheme );

	bool isUsingSearchReplaceTree();

  protected:
	UICodeEditorSplitter* mEditorSplitter{ nullptr };
	UISceneNode* mUISceneNode{ nullptr };
	App* mApp{ nullptr };

	UIGlobalSearchBar* mGlobalSearchBarLayout{ nullptr };
	UILayout* mGlobalSearchLayout{ nullptr };
	UITreeViewGlobalSearch* mGlobalSearchTree{ nullptr };
	UITreeViewGlobalSearch* mGlobalSearchTreeSearch{ nullptr };
	UITreeViewGlobalSearch* mGlobalSearchTreeReplace{ nullptr };
	UITextInput* mGlobalSearchInput{ nullptr };
	UIDropDownList* mGlobalSearchHistoryList{ nullptr };
	Uint32 mGlobalSearchHistoryOnItemSelectedCb{ 0 };
	std::deque<std::pair<std::string, std::shared_ptr<ProjectSearch::ResultModel>>>
		mGlobalSearchHistory;
};

#endif // GLOBALSEARCHCONTROLLER_HPP
