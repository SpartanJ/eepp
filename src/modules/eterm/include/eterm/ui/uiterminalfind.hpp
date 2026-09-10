#ifndef ETERM_UI_UITERMINALFIND_HPP
#define ETERM_UI_UITERMINALFIND_HPP

#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uitextview.hpp>

using namespace EE;
using namespace EE::UI;

namespace eterm { namespace UI {

class UITerminal;

class UITerminalFind : public UILinearLayout {
  public:
	static UITerminalFind* New( UITerminal* terminal );

	void show();

	void hide();

	void refreshStatus();

  protected:
	explicit UITerminalFind( UITerminal* terminal );

	Uint32 onKeyDown( const KeyEvent& event );

	Uint32 onKeyUp( const KeyEvent& event );

	Uint32 onTextInput( const TextInputEvent& event );

	Uint32 onTextEditing( const TextEditingEvent& event );

  private:
	void updateQuery();

	void submitQuery();

	void navigateSearch( int direction );

	UITerminal* mTerminal{ nullptr };
	UITextInput* mInput{ nullptr };
	UISelectButton* mMatchCase{ nullptr };
	UISelectButton* mWholeWord{ nullptr };
	UISelectButton* mRegEx{ nullptr };
	UISelectButton* mLuaPattern{ nullptr };
	UITextView* mStatus{ nullptr };
	Uint64 mRequestId{ 0 };
	bool mReady{ false };
	bool mChangingPattern{ false };
	bool mQueryPending{ false };
};

}} // namespace eterm::UI

#endif
