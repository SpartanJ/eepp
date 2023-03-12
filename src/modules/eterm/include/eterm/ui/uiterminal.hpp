#ifndef ETERM_UI_UITERMINAL_HPP
#define ETERM_UI_UITERMINAL_HPP

#include <eepp/ui/keyboardshortcut.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eterm/terminal/terminaldisplay.hpp>

using namespace EE::UI;
using namespace eterm::Terminal;

namespace eterm { namespace UI {

class UITerminal : public UIWidget {
  public:
	enum ScrollViewType { Inclusive, Exclusive };

	static UITerminal* New( Font* font, const Float& fontSize, const Sizef& pixelsSize,
							const std::string& program = "",
							const std::vector<std::string>& args = {},
							const std::string& workingDir = "", const size_t& historySize = 10000,
							IProcessFactory* processFactory = nullptr,
							const bool& useFrameBuffer = false );
	typedef std::function<void()> TerminalCommand;

	static UITerminal* New( const std::shared_ptr<TerminalDisplay>& terminalDisplay );

	virtual ~UITerminal();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void draw();

	const std::shared_ptr<TerminalDisplay>& getTerm() const;

	virtual void scheduledUpdate( const Time& time );

	const std::string& getTitle() const;

	void setTitle( const std::string& title );

	KeyBindings& getKeyBindings();

	Float getFontSize() const;

	void setFontSize( Float fontSize );

	Font* getFont() const;

	void setFont( Font* font );

	void setKeyBindings( const KeyBindings& keyBindings );

	void addKeyBindingString( const std::string& shortcut, const std::string& command );

	void addKeyBinding( const KeyBindings::Shortcut& shortcut, const std::string& command );

	void replaceKeyBindingString( const std::string& shortcut, const std::string& command );

	void replaceKeyBinding( const KeyBindings::Shortcut& shortcut, const std::string& command );

	void addKeyBindsString( const std::map<std::string, std::string>& binds );

	void addKeyBinds( const std::map<KeyBindings::Shortcut, std::string>& binds );

	void execute( const std::string& command );

	void setCommands( const std::map<std::string, TerminalCommand>& cmds );

	void setCommand( const std::string& command, const TerminalCommand& func );

	bool hasCommand( const std::string& command );

	static std::string getExclusiveModeToggleCommandName() {
		return "terminal-toggle-exclusive-mode";
	}

	bool getExclusiveMode() const;

	/** Exclusive mode disables the global keybindings except for the enable/disable exclusive mode.
	 *  This allows the user to use all the keybindings in the terminal. */
	void setExclusiveMode( bool exclusiveMode );

	bool isUsingCustomTitle() const;

	void setVerticalScrollMode( const ScrollBarMode& Mode );

	const ScrollBarMode& getVerticalScrollMode() const;

	UIScrollBar* getVerticalScrollBar() const;

	const ScrollViewType& getViewType() const;

	void setViewType( const ScrollViewType& viewType );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	void executeFile( const std::string& cmd );

	const TerminalColorScheme& getColorScheme() const;

	void setColorScheme( const TerminalColorScheme& colorScheme );

  protected:
	std::string mTitle;
	bool mIsCustomTitle{ false };
	bool mExclusiveMode{ false };
	bool mCreateDefaultContextMenuOptions{ true };
	KeyBindings mKeyBindings;
	std::map<std::string, TerminalCommand> mCommands;
	UIPopUpMenu* mCurrentMenu{ nullptr };
	size_t mMenuIconSize{ 16 };
	ScrollViewType mViewType{ Inclusive };
	ScrollBarMode mVScrollMode{ ScrollBarMode::Auto };
	UIScrollBar* mVScroll{ nullptr };
	int mScrollOffset;
	bool mScrollByBar{ false };
	Clock mMouseClock;

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

	virtual void onAlphaChange();

	virtual void onPaddingChange();

	virtual Uint32 onFocus();

	virtual Uint32 onFocusLoss();

	virtual void updateScroll();

	virtual void onContentSizeChange();

	virtual int getContentSize() const;

	UIMenuItem* menuAdd( UIPopUpMenu* menu, const std::string& translateKey,
						 const String& translateString, const std::string& icon,
						 const std::string& cmd );

	virtual bool onCreateContextMenu( const Vector2i& position, const Uint32& flags );

	Drawable* findIcon( const std::string& name );

	void createDefaultContextMenuOptions( UIPopUpMenu* menu );

	int getScrollableArea() const;

	int getVisibleArea() const;

	virtual void updateScrollPosition();

	virtual void onScrollChange();
};

}} // namespace eterm::UI

#endif
