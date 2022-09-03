#ifndef EE_UI_TOOLS_UIDOCFINDREPLACE_HPP
#define EE_UI_TOOLS_UIDOCFINDREPLACE_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/widgetcommandexecuter.hpp>
#include <memory>

namespace EE { namespace UI { namespace Tools {

class EE_API UIDocFindReplace : public UILinearLayout, public WidgetCommandExecuter {
  public:
	static std::unordered_map<std::string, std::string> getDefaultKeybindings() {
		return { { "mod+g", "repeat-find" },		   { "escape", "close-find-replace" },
				 { "mod+r", "replace-selection" },	   { "mod+shift+n", "find-and-replace" },
				 { "mod+shift+r", "replace-all" },	   { "mod+s", "change-case" },
				 { "mod+w", "change-whole-word" },	   { "mod+l", "toggle-lua-pattern" },
				 { "mod+e", "change-escape-sequence" } };
	}

	static UIDocFindReplace*
	New( UIWidget* parent, const std::shared_ptr<Doc::TextDocument>& doc,
		 std::unordered_map<std::string, std::string> keybindings = getDefaultKeybindings() );

	const std::shared_ptr<Doc::TextDocument>& getDoc() const;

	void setDoc( const std::shared_ptr<Doc::TextDocument>& doc );

	virtual void show();

	virtual void hide();

  protected:
	struct SearchState {
		String text;
		TextRange range = TextRange();
		bool caseSensitive{ false };
		bool wholeWord{ false };
		bool escapeSequences{ false };
		TextDocument::FindReplaceType type{ TextDocument::FindReplaceType::Normal };
		void reset() {
			range = TextRange();
			text = "";
		}
	};

	bool mReady{ false };
	UITextInput* mFindInput{ nullptr };
	UITextInput* mReplaceInput{ nullptr };
	UISelectButton* mCaseSensitive{ nullptr };
	UISelectButton* mLuaPattern{ nullptr };
	UISelectButton* mWholeWord{ nullptr };
	UISelectButton* mEscapeSequences{ nullptr };
	UIWidget* mToggle{ nullptr };
	UIWidget* mReplaceBox{ nullptr };
	std::shared_ptr<Doc::TextDocument> mDoc;

	UIDocFindReplace(
		UIWidget* parent, const std::shared_ptr<Doc::TextDocument>& doc,
		std::unordered_map<std::string, std::string> keybindings = getDefaultKeybindings() );

	SearchState mSearchState;
	String mLastSearch;

	bool findAndReplace( SearchState& search, const String& replace );

	bool findPrevText( SearchState& search );

	bool findNextText( SearchState& search );

	int replaceAll( SearchState& search, const String& replace );

	virtual Uint32 onKeyDown( const KeyEvent& event );
};

}}} // namespace EE::UI::Tools

#endif // EE_UI_TOOLS_UIDOCFINDREPLACE_HPP
