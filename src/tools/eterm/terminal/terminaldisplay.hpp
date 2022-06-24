#ifndef ETERMINALDISPLAY_HPP
#define ETERMINALDISPLAY_HPP

#include "../system/iprocessfactory.hpp"
#include "iterminaldisplay.hpp"
#include "terminalemulator.hpp"
#include <eepp/config.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/window/inputevent.hpp>
#include <eepp/window/keycodes.hpp>
#include <eepp/window/window.hpp>

#include <atomic>
#include <memory>
#include <unordered_map>
#include <vector>

using namespace EE;
using namespace EE::Terminal;
using namespace EE::Window;
using namespace EE::System;

enum class TerminalShortcutAction { PASTE, COPY };

struct TerminalConfig {
	int options;
};

struct TerminalKey {
	Keycode keysym;
	Uint32 mask;
	const char* string;
	int appkey;
	int appcursor;
};

struct TerminalScancode {
	Scancode scancode;
	Uint32 mask;
	std::string string;
	int appkey;
	int appcursor;
};

struct TerminalShortcut {
	Keycode keysym;
	Uint32 mask;
	TerminalShortcutAction action;
	int appkey;
	int appcursor;
};

struct TerminalKeyMapEntry {
	Uint32 mask;
	std::string string;
	int appkey;
	int appcursor;
};

struct TerminalKeyMapShortcut {
	Uint32 mask;
	TerminalShortcutAction action;
	int appkey;
	int appcursor;
};

class TerminalKeyMap {
  private:
	std::unordered_map<Keycode, std::vector<TerminalKeyMapEntry>> mKeyMap;
	std::unordered_map<Scancode, std::vector<TerminalKeyMapEntry>> mPlatformKeyMap;
	std::unordered_map<Keycode, std::vector<TerminalKeyMapShortcut>> mShortcuts;

  public:
	TerminalKeyMap( const TerminalKey keys[], size_t keysLen, const TerminalScancode platformKeys[],
					size_t platformKeysLen, const TerminalShortcut shortcuts[],
					size_t shortcutsLen );

	inline const std::unordered_map<Keycode, std::vector<TerminalKeyMapEntry>>& KeyMap() const {
		return mKeyMap;
	}

	inline const std::unordered_map<Scancode, std::vector<TerminalKeyMapEntry>>&
	PlatformKeyMap() const {
		return mPlatformKeyMap;
	}

	inline const std::unordered_map<Keycode, std::vector<TerminalKeyMapShortcut>>&
	Shortcuts() const {
		return mShortcuts;
	}
};

constexpr int TerminalKeyModFlags_Any = 0xFFFFFFFF;

extern TerminalKeyMap terminalKeyMap;

static const Scancode asciiScancodeTable[] = {
	SCANCODE_A, SCANCODE_B, SCANCODE_C,			  SCANCODE_D,	  SCANCODE_E,			SCANCODE_F,
	SCANCODE_G, SCANCODE_H, SCANCODE_I,			  SCANCODE_J,	  SCANCODE_K,			SCANCODE_L,
	SCANCODE_M, SCANCODE_N, SCANCODE_O,			  SCANCODE_P,	  SCANCODE_Q,			SCANCODE_R,
	SCANCODE_S, SCANCODE_T, SCANCODE_U,			  SCANCODE_V,	  SCANCODE_W,			SCANCODE_X,
	SCANCODE_Y, SCANCODE_Z, SCANCODE_LEFTBRACKET, SCANCODE_SLASH, SCANCODE_RIGHTBRACKET };

class TerminalDisplay : public ITerminalDisplay {
  public:
	static std::shared_ptr<TerminalDisplay>
	create( EE::Window::Window* window, Font* font, const Float& fontSize, const Sizef& pixelsSize,
			std::shared_ptr<TerminalEmulator>&& terminalEmulator );

	static std::shared_ptr<TerminalDisplay>
	create( EE::Window::Window* window, Font* font, const Float& fontSize, const Sizef& pixelsSize,
			const std::string& program, const std::vector<std::string>& args,
			const std::string& workingDir, IProcessFactory* processFactory = nullptr );

	virtual void resetColors();
	virtual int resetColor( int index, const char* name );

	virtual void setTitle( const char* title );
	virtual void setIconTitle( const char* title );

	virtual void setClipboard( const char* text );
	virtual const char* getClipboard() const;

	virtual bool drawBegin( int columns, int rows );
	virtual void drawLine( Line line, int x1, int y, int x2 );
	virtual void drawCursor( int cx, int cy, TerminalGlyph g, int ox, int oy, TerminalGlyph og );
	virtual void drawEnd();

	virtual void update();

	void action( TerminalShortcutAction action );

	bool hasTerminated() const;

	void draw();

	virtual void onMouseDoubleClick( const Vector2i& pos, const Uint32& flags );

	virtual void onMouseMotion( const Vector2i& pos, const Uint32& flags );

	virtual void onMouseDown( const Vector2i& pos, const Uint32& flags );

	virtual void onMouseUp( const Vector2i& pos, const Uint32& flags );

	virtual void onTextInput( const Uint32& chr );

	virtual void onKeyDown( const Keycode& keyCode, const Uint32& chr, const Uint32& mod,
							const Scancode& scancode );

	Float getFontSize() const;

	void setFontSize( const Float& FontSize );

	const Vector2f& getPosition() const;

	void setPosition( const Vector2f& position );

	const Sizef& getSize() const;

	void setSize( const Sizef& size );

	bool isDirty() const { return mDirty; }

	void invalidate();

	bool hasFocus() const { return mFocus; }

	void setFocus( bool focus );

	bool isBlinkingCursor();

	const Sizef& getPadding() const;

	void setPadding( const Sizef& padding );

  protected:
	EE::Window::Window* mWindow;
	std::vector<TerminalGlyph> mBuffer;
	std::vector<std::pair<Color, std::string>> mColors;
	std::shared_ptr<TerminalEmulator> mTerminal;
	mutable std::string mClipboard;

	Font* mFont{ nullptr };
	Float mFontSize{ 12 };
	Sizef mPadding;
	Vector2f mPosition;
	Sizef mSize;
	int mColumns{ 0 };
	int mRows{ 0 };
	std::atomic<bool> mDirty{ true };
	std::atomic<bool> mDrawing{ false };
	Vector2i mCursor;
	TerminalGlyph mCursorGlyph;
	bool mUseColorEmoji{ true };
	bool mPasteNewlineFix{ true };
	bool mFocus{ true };
	Clock mClock;
	Clock mLastDoubleClick;

	TerminalDisplay( EE::Window::Window* window, Font* font, const Float& fontSize,
					 const Sizef& pixelsSize );

	void draw( const Vector2f& pos );

	Vector2i positionToGrid( const Vector2i& pos );

	void onSizeChange();
};

#endif // ETERMINALDISPLAY_HPP
