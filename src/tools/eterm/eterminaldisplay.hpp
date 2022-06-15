#ifndef ETERMINALDISPLAY_HPP
#define ETERMINALDISPLAY_HPP

#include "system/iprocessfactory.hpp"
#include "terminal/terminaldisplay.hpp"
#include "terminal/terminalemulator.hpp"
#include <eepp/config.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/window/window.hpp>
#include <memory>
#include <vector>

using namespace Hexe::Terminal;
using namespace EE;
using namespace EE::Window;
using namespace EE::System;

enum TerminalOptions {
	OPTION_NONE = 0,
	OPTION_COLOR_EMOJI = 1 << 0,
	OPTION_NO_BOXDRAWING = 1 << 1,
	OPTION_PASTE_CRLF = 1 << 2
};

struct TerminalConfig {
	int options;
};

class ETerminalDisplay : public TerminalDisplay {
  public:
	static std::shared_ptr<ETerminalDisplay>
	Create( EE::Window::Window* window, Font* font,
			std::shared_ptr<Hexe::Terminal::TerminalEmulator>&& terminalEmulator,
			TerminalConfig* config = 0 );

	static std::shared_ptr<ETerminalDisplay>
	Create( EE::Window::Window* window, Font* font, int columns, int rows,
			const std::string& program, const std::vector<std::string>& args,
			const std::string& workingDir, uint32_t options = 0,
			Hexe::System::IProcessFactory* processFactory = nullptr );

	virtual void ResetColors();
	virtual int ResetColor( int index, const char* name );

	virtual void SetTitle( const char* title );
	virtual void SetIconTitle( const char* title );

	virtual void SetClipboard( const char* text );
	virtual const char* GetClipboard() const;

	virtual bool DrawBegin( int columns, int rows );
	virtual void DrawLine( Line line, int x1, int y, int x2 );
	virtual void DrawCursor( int cx, int cy, Hexe::Terminal::Glyph g, int ox, int oy,
							 Hexe::Terminal::Glyph og );
	virtual void DrawEnd();

	virtual void Update();

	bool HasTerminated() const;

	void Draw( const Rectf& contentArea );

	void Draw( Vector2i pos, const Sizei& clip_rect, bool hasFocus );

  protected:
	EE::Window::Window* mWindow;
	std::vector<Hexe::Terminal::Glyph> m_buffer;
	std::vector<std::pair<Color, std::string>> m_colors;
	std::shared_ptr<Hexe::Terminal::TerminalEmulator> m_terminal;
	mutable std::string mClipboard;

	Font* mFont;
	int m_columns{ 0 };
	int m_rows{ 0 };
	bool m_dirty;
	bool m_checkDirty;
	int m_cursorx;
	int m_cursory;
	Hexe::Terminal::Glyph m_cursorg;
	bool m_useBoxDrawing;
	bool m_useColorEmoji;
	bool m_pasteNewlineFix;
	Clock mClock;

	ETerminalDisplay( EE::Window::Window* window, Font* font, int columns, int rows,
					  TerminalConfig* config );
};

#endif // ETERMINALDISPLAY_HPP
