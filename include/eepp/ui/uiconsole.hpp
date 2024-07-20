#ifndef EE_UI_UICONSOLE_HPP
#define EE_UI_UICONSOLE_HPP

#include <deque>
#include <eepp/graphics/text.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <eepp/ui/keyboardshortcut.hpp>
#include <eepp/ui/uifontstyleconfig.hpp>
#include <eepp/ui/uiwidget.hpp>

using namespace EE::Graphics;
using namespace EE::System;
using namespace EE::UI::Doc;

namespace EE { namespace UI {

class UIPopUpMenu;
class UIMenuItem;

class EE_API UIConsole : public UIWidget,
						 protected LogReaderInterface,
						 public TextDocument::Client {
  public:
	//! The Console Callback return a vector of parameters ( String )
	typedef std::function<void( const std::vector<String>& )> ConsoleCallback;

	static UIConsole* New();

	static UIConsole* NewOpt( Font* font, const bool& makeDefaultCommands = true,
							  const bool& attachToLog = true,
							  const unsigned int& maxLogLines = 1024 );

	virtual ~UIConsole();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	Font* getFont() const;

	const UIFontStyleConfig& getFontStyleConfig() const;

	UIConsole* setFont( Font* font );

	UIConsole* setFontSize( const Float& size );

	const Float& getFontSize() const;

	UIConsole* setFontColor( const Color& color );

	const Color& getFontColor() const;

	UIConsole* setFontSelectedColor( const Color& color );

	const Color& getFontSelectedColor() const;

	UIConsole* setFontSelectionBackColor( const Color& color );

	const Color& getFontSelectionBackColor() const;

	UIConsole* setFontShadowColor( const Color& color );

	const Color& getFontShadowColor() const;

	UIConsole* setFontShadowOffset( const Vector2f& offset );

	const Vector2f& getFontShadowOffset() const;

	UIConsole* setFontStyle( const Uint32& fontStyle );

	UIConsole* setFontOutlineThickness( const Float& outlineThickness );

	const Float& getFontOutlineThickness() const;

	UIConsole* setFontOutlineColor( const Color& outlineColor );

	const Color& getFontOutlineColor() const;

	void addCommand( const std::string& command, const ConsoleCallback& cb );

	void setCommand( const std::string& command, const ConsoleCallback& cb );

	const Uint32& getMaxLogLines() const;

	void setMaxLogLines( const Uint32& maxLogLines );

	Int32 linesOnScreen();

	virtual void draw();

	/** @return If the console is rendering the FPS count. */
	const bool& isShowingFps() const;

	/** Activate/Deactive fps rendering */
	void showFps( const bool& show );

	bool getEscapePastedText() const;

	void setEscapePastedText( bool escapePastedText );

	bool isTextSelectionEnabled() const;

	/** Add Text to Console */
	void pushText( const String& str );

	/** Add formated Text to console */
	template <typename... Args> void pushText( std::string_view format, Args&&... args ) {
		pushText( String::format(
			format, FormatArg<std::decay_t<Args>>::get( std::forward<Args>( args ) )... ) );
	}

	Float getLineHeight() const;

	bool getQuakeMode() const;

	void setQuakeMode( bool quakeMode );

	void show();

	void hide();

	void toggle();

	bool isActive() const;

	Float getQuakeModeHeightPercent() const;

	void setQuakeModeHeightPercent( const Float& quakeModeHeightPercent );

	virtual void scheduledUpdate( const Time& time );

	const Time& getBlinkTime() const;

	void setBlinkTime( const Time& blinkTime );

	size_t getMenuIconSize() const;

	void setMenuIconSize( size_t menuIconSize );

	KeyBindings& getKeyBindings();

	TextDocument& getDoc();

  protected:
	struct TextCache {
		Text text;
		String::HashType hash;
	};
	struct CommandLogCache {
		String log;
		String::HashType hash;
	};
	Mutex mMutex;
	std::map<String, ConsoleCallback> mCallbacks;
	std::deque<CommandLogCache> mCmdLog;
	std::deque<String> mLastCommands;
	std::vector<TextCache> mTextCache;
	UIFontStyleConfig mFontStyleConfig;
	Uint32 mMaxLogLines{ 8192 };
	TextDocument mDoc;
	KeyBindings mKeyBindings;
	TextRange mSelection;

	struct sCon {
		int min{ 0 };
		int max{ 0 };
		int modif{ 0 };
	};
	sCon mCon;

	bool mShowFps{ false };
	bool mEscapePastedText{ false };
	bool mMouseDown{ false };
	bool mQuakeMode{ false };
	bool mShowing{ false };
	bool mHiding{ false };
	bool mFading{ false };
	Clock mBlinkTimer;
	Time mBlinkTime{ Seconds( 0.f ) };
	bool mCursorVisible{ true };
	int mLastLogPos{ 0 };
#if EE_PLATFORM == EE_PLATFORM_ANDROID || EE_PLATFORM == EE_PLATFORM_IOS
	Float mQuakeModeHeightPercent{ 0.5f };
#else
	Float mQuakeModeHeightPercent{ 0.6f };
#endif
	Uint64 mLastExecuteEventId{ 0 };
	UIPopUpMenu* mCurrentMenu{ nullptr };
	size_t mMenuIconSize{ 16 };

	UIConsole( Font* Font, const bool& makeDefaultCommands = true, const bool& attachToLog = true,
			   const unsigned int& maxLogLines = 1024 );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	void updateCacheSize();

	virtual Uint32 onPressEnter();

	virtual void onFontChanged();

	virtual void onFontStyleChanged();

	virtual Uint32 onMouseDown( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseMove( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseDoubleClick( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseUp( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onFocus( NodeFocusReason reason );

	virtual Uint32 onFocusLoss();

	virtual void onDocumentTextChanged( const DocumentContentChange& );

	virtual void onDocumentCursorChange( const TextPosition& );

	virtual void onDocumentSelectionChange( const TextRange& );

	virtual void onDocumentLineCountChange( const size_t& lastCount, const size_t& newCount );

	virtual void onDocumentLineChanged( const Int64& lineIndex );

	virtual void onDocumentUndoRedo( const TextDocument::UndoRedo& );

	virtual void onDocumentSaved( TextDocument* );

	virtual void onDocumentMoved( TextDocument* );

	virtual void onDocumentReset( TextDocument* ) {}

	virtual bool onCreateContextMenu( const Vector2i& position, const Uint32& flags );

	void onDocumentClosed( TextDocument* ) {};

	void onDocumentDirtyOnFileSystem( TextDocument* ) {};

	virtual Uint32 onKeyDown( const KeyEvent& event );

	virtual Uint32 onTextInput( const TextInputEvent& event );

	virtual Uint32 onTextEditing( const TextEditingEvent& event );

	virtual void onSelectionChange();

	virtual void onSizeChange();

	virtual void onParentSizeChange( const Vector2f& sizeChange );

	void registerKeybindings();

	void registerCommands();

	void copy();

	void cut();

	void paste();

	void createDefaultCommands();

	/** Internal Callback for default command ( cmdlist ) */
	void cmdCmdList();

	/** Internal Callback for default command ( showcursor ) */
	void cmdShowCursor( const std::vector<String>& params );

	/** Internal Callback for default command ( setfpslimit ) */
	void cmdFrameLimit( const std::vector<String>& params );

	/** Internal Callback for default command ( setgamma ) */
	void cmdSetGamma( const std::vector<String>& params );

	/** Internal Callback for default command ( setvolume ) */
	void cmdSetVolume( const std::vector<String>& params );

	/** Internal Callback for default command ( dir and ls ) */
	void cmdDir( const std::vector<String>& params );

	/** Internal Callback for default command ( showfps ) */
	void cmdShowFps( const std::vector<String>& params );

	/** Internal Callback for default command ( gettexturememory ) */
	void cmdGetTextureMemory();

	/** The Default Commands Callbacks for the Console ( don't call it ) */
	void privInputCallback( InputEvent* Event );

	/** Clear the Console */
	void cmdClear();

	/** Add the current log to the console */
	void cmdGetLog();

	/** Add the GPU Extensions supported to the console */
	void cmdGetGpuExtensions();

	/** Add command to grep the console log */
	void cmdGrep( const std::vector<String>& params );

	void privPushText( String&& str );

	void writeLog( const std::string_view& text );

	void resetCursor();

	void getFilesFrom( std::string txt, const Uint32& curPos );

	void printCommandsStartingWith( const String& start );

	String getLastCommonSubStr( std::vector<String>& cmds );

	void processLine();

	Int32 maxLinesOnScreen();

	void updateQuakeMode();

	TextPosition getPositionOnScreen( Vector2f position );

	UIMenuItem* menuAdd( UIPopUpMenu* menu, const String& translateString, const std::string& icon,
						 const std::string& cmd );

	Drawable* findIcon( const std::string& name );

	void copySelection();

	void updateIMELocation( const Rectf& loc );
};

}} // namespace EE::UI

#endif
