#ifndef ETERM_UI_UITERMINAL_HPP
#define ETERM_UI_UITERMINAL_HPP

#include <eepp/ui/uiwidget.hpp>
#include <eterm/terminal/terminaldisplay.hpp>

using namespace EE::UI;
using namespace eterm::Terminal;

namespace eterm { namespace UI {

class EE_API UITerminal : public UIWidget {
  public:
	static UITerminal* New( Font* font, const Float& fontSize, const Sizef& pixelsSize,
							std::string program = "", const std::vector<std::string>& args = {},
							const std::string& workingDir = "", const size_t& historySize = 10000,
							IProcessFactory* processFactory = nullptr,
							const bool& useFrameBuffer = false );

	static UITerminal* New( const std::shared_ptr<TerminalDisplay>& terminalDisplay );

	virtual ~UITerminal();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void draw();

	const std::shared_ptr<TerminalDisplay>& getTerm() const;

	virtual void scheduledUpdate( const Time& time );

	const std::string& getTitle() const;

	void setTitle( const std::string& title );

  protected:
	std::string mTitle;
	bool mIsCustomTitle{ false };
	bool mDraggingSel{ false };

	UITerminal( const std::shared_ptr<TerminalDisplay>& terminalDisplay );

	std::shared_ptr<TerminalDisplay> mTerm;

	virtual Uint32 onTextInput( const TextInputEvent& event );

	virtual Uint32 onKeyDown( const KeyEvent& event );

	virtual Uint32 onKeyUp( const KeyEvent& event );

	virtual Uint32 onMouseMove( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseDown( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseDoubleClick( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseUp( const Vector2i& position, const Uint32& flags );

	virtual void onPositionChange();

	virtual void onSizeChange();

	virtual Uint32 onFocus();

	virtual Uint32 onFocusLoss();
};

}} // namespace eterm::UI

#endif
