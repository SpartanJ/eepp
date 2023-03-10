#ifndef EE_UI_TOOLS_UIDOCFINDREPLACE_HPP
#define EE_UI_TOOLS_UIDOCFINDREPLACE_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uidatabind.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/widgetcommandexecuter.hpp>

namespace EE { namespace UI { namespace Tools {

class EE_API UIDocFindReplace : public UILinearLayout, public WidgetCommandExecuter {
  public:
	static std::unordered_map<std::string, std::string> getDefaultKeybindings() {
		return { { "mod+g", "repeat-find" },
				 { "escape", "close-find-replace" },
				 { "mod+r", "replace-selection" },
				 { "mod+shift+n", "find-and-replace" },
				 { "mod+shift+r", "replace-all" },
				 { "mod+s", "change-case" },
				 { "mod+w", "change-whole-word" },
				 { "mod+l", "toggle-lua-pattern" },
				 { "mod+e", "change-escape-sequence" },
				 { "mod+shift+g", "find-prev" } };
	}

	static UIDocFindReplace*
	New( UIWidget* parent, const std::shared_ptr<Doc::TextDocument>& doc,
		 std::unordered_map<std::string, std::string> keybindings = getDefaultKeybindings() );

	const std::shared_ptr<Doc::TextDocument>& getDoc() const;

	void setDoc( const std::shared_ptr<Doc::TextDocument>& doc );

	virtual void show( bool expanded = false );

	virtual void hide();

  protected:
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
	std::vector<std::unique_ptr<UIDataBind<bool>>> mDataBinds;
	std::unique_ptr<UIDataBind<TextDocument::FindReplaceType>> mPatternBind;

	UIDocFindReplace(
		UIWidget* parent, const std::shared_ptr<Doc::TextDocument>& doc,
		std::unordered_map<std::string, std::string> keybindings = getDefaultKeybindings() );

	TextSearchParams mSearchState;
	String mLastSearch;

	bool findAndReplace( TextSearchParams& search, const String& replace );

	bool findPrevText( TextSearchParams& search );

	bool findNextText( TextSearchParams& search );

	int replaceAll( TextSearchParams& search, const String& replace );

	virtual Uint32 onKeyDown( const KeyEvent& event );
};

}}} // namespace EE::UI::Tools

#endif // EE_UI_TOOLS_UIDOCFINDREPLACE_HPP
