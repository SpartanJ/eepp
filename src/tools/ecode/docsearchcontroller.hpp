#ifndef DOCSEARCHCONTROLLER_HPP
#define DOCSEARCHCONTROLLER_HPP

#include <eepp/ee.hpp>

struct SearchState {
	UICodeEditor* editor{ nullptr };
	String text;
	TextRange range = TextRange();
	bool caseSensitive{ false };
	bool wholeWord{ false };
	bool escapeSequences{ false };
	TextDocument::FindReplaceType type{ TextDocument::FindReplaceType::Normal };
	void reset() {
		editor = nullptr;
		range = TextRange();
		text = "";
	}
};

class App;
class UISearchBar;

class DocSearchController {
  public:
	static std::unordered_map<std::string, std::string> getDefaultKeybindings() {
		return { { "mod+g", "repeat-find" },		   { "escape", "close-searchbar" },
				 { "mod+r", "replace-selection" },	   { "mod+shift+f", "find-and-replace" },
				 { "mod+shift+r", "replace-all" },	   { "mod+s", "change-case" },
				 { "mod+w", "change-whole-word" },	   { "mod+l", "toggle-lua-pattern" },
				 { "mod+e", "change-escape-sequence" } };
	}

	DocSearchController( UICodeEditorSplitter*, App* app );

	void initSearchBar(
		UISearchBar* searchBar,
		std::unordered_map<std::string, std::string> keybindings = getDefaultKeybindings() );

	void showFindView();

	bool findPrevText( SearchState& search );

	bool findNextText( SearchState& search );

	bool replaceSelection( SearchState& search, const String& replacement );

	int replaceAll( SearchState& search, const String& replace );

	bool findAndReplace( SearchState& search, const String& replace );

	void hideSearchBar();

	void onCodeEditorFocusChange( UICodeEditor* editor );

	SearchState& getSearchState();

  protected:
	UICodeEditorSplitter* mEditorSplitter{ nullptr };
	UISearchBar* mSearchBarLayout{ nullptr };
	App* mApp{ nullptr };
	SearchState mSearchState;
	String mLastSearch;
};

#endif // DOCSEARCHCONTROLLER_HPP
