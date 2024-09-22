#ifndef ECODE_DOCSEARCHCONTROLLER_HPP
#define ECODE_DOCSEARCHCONTROLLER_HPP

#include "appconfig.hpp"
#include <eepp/ee.hpp>

namespace ecode {

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

	TextSearchParams toTextSearchParams() {
		return { text, range, caseSensitive, wholeWord, escapeSequences, type };
	}
};

class App;
class UISearchBar;

class DocSearchController {
  public:
	static std::unordered_map<std::string, std::string> getDefaultKeybindings() {
		return { { "mod+g", "repeat-find" },
				 { "escape", "close-searchbar" },
				 { "mod+r", "replace-selection" },
				 { "mod+shift+n", "find-and-replace" },
				 { "mod+shift+r", "replace-all" },
				 { "mod+s", "change-case" },
				 { "mod+w", "change-whole-word" },
				 { "mod+l", "toggle-lua-pattern" },
				 { "mod+p", "toggle-regex" },
				 { "mod+e", "change-escape-sequence" },
				 { "mod+shift+g", "find-prev" },
				 { "mod+shift+a", "select-all-results" } };
	}

	DocSearchController( UICodeEditorSplitter*, App* app );

	~DocSearchController();

	void initSearchBar(
		UISearchBar* searchBar, const SearchBarConfig& searchBarConfig,
		std::unordered_map<std::string, std::string> keybindings = getDefaultKeybindings() );

	void showFindView();

	void findPrevText( SearchState& search );

	void findNextText( SearchState& search, bool resetSelection = false );

	bool replaceSelection( SearchState& search, const String& replacement );

	int replaceAll( SearchState& search, const String& replace );

	void findAndReplace( SearchState& search, const String& replace );

	void hideSearchBar();

	void onCodeEditorFocusChange( UICodeEditor* editor );

	SearchState& getSearchState();

	SearchBarConfig getSearchBarConfig() const;

	void selectAll( SearchState& search );

	void refreshHighlight();

  protected:
	UICodeEditorSplitter* mSplitter{ nullptr };
	UITextInput* mFindInput{ nullptr };
	UITextInput* mReplaceInput{ nullptr };
	UISearchBar* mSearchBarLayout{ nullptr };
	UICheckBox* mCaseSensitiveChk{ nullptr };
	UICheckBox* mEscapeSequenceChk{ nullptr };
	UICheckBox* mWholeWordChk{ nullptr };
	UICheckBox* mRegExChk{ nullptr };
	UICheckBox* mLuaPatternChk{ nullptr };
	App* mApp{ nullptr };
	SearchState mSearchState;
	String mLastSearch;
	size_t mSearchingCount{ 0 };
	bool mValueChanging{ false };
};

} // namespace ecode

#endif // ECODE_DOCSEARCHCONTROLLER_HPP
