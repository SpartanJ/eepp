#include "eterminaldisplay.hpp"
#include "system/processfactory.hpp"
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/system/color.hpp>
#include <eepp/window.hpp>
#include <eepp/window/clipboard.hpp>

// This is the customizable colorscheme
const char* colornames[256] = { "#1e2127", "#e06c75", "#98c379", "#d19a66", "#61afef", "#c678dd",
								"#56b6c2", "#abb2bf", "#5c6370", "#e06c75", "#98c379", "#d19a66",
								"#61afef", "#c678dd", "#56b6c2", "#ffffff", "#1e2127", "#abb2bf" };

// This is the default Xterm palette
static const Color colormapped[256] = {
	Color( 0, 0, 0 ),		Color( 128, 0, 0 ),		Color( 0, 128, 0 ),
	Color( 128, 128, 0 ),	Color( 0, 0, 128 ),		Color( 128, 0, 128 ),
	Color( 0, 128, 128 ),	Color( 192, 192, 192 ), Color( 128, 128, 128 ),
	Color( 255, 0, 0 ),		Color( 0, 255, 0 ),		Color( 255, 255, 0 ),
	Color( 0, 0, 255 ),		Color( 255, 0, 255 ),	Color( 0, 255, 255 ),
	Color( 255, 255, 255 ), Color( 0, 0, 0 ),		Color( 0, 0, 95 ),
	Color( 0, 0, 135 ),		Color( 0, 0, 175 ),		Color( 0, 0, 215 ),
	Color( 0, 0, 255 ),		Color( 0, 95, 0 ),		Color( 0, 95, 95 ),
	Color( 0, 95, 135 ),	Color( 0, 95, 175 ),	Color( 0, 95, 215 ),
	Color( 0, 95, 255 ),	Color( 0, 135, 0 ),		Color( 0, 135, 95 ),
	Color( 0, 135, 135 ),	Color( 0, 135, 175 ),	Color( 0, 135, 215 ),
	Color( 0, 135, 255 ),	Color( 0, 175, 0 ),		Color( 0, 175, 95 ),
	Color( 0, 175, 135 ),	Color( 0, 175, 175 ),	Color( 0, 175, 215 ),
	Color( 0, 175, 255 ),	Color( 0, 215, 0 ),		Color( 0, 215, 95 ),
	Color( 0, 215, 135 ),	Color( 0, 215, 175 ),	Color( 0, 215, 215 ),
	Color( 0, 215, 255 ),	Color( 0, 255, 0 ),		Color( 0, 255, 95 ),
	Color( 0, 255, 135 ),	Color( 0, 255, 175 ),	Color( 0, 255, 215 ),
	Color( 0, 255, 255 ),	Color( 95, 0, 0 ),		Color( 95, 0, 95 ),
	Color( 95, 0, 135 ),	Color( 95, 0, 175 ),	Color( 95, 0, 215 ),
	Color( 95, 0, 255 ),	Color( 95, 95, 0 ),		Color( 95, 95, 95 ),
	Color( 95, 95, 135 ),	Color( 95, 95, 175 ),	Color( 95, 95, 215 ),
	Color( 95, 95, 255 ),	Color( 95, 135, 0 ),	Color( 95, 135, 95 ),
	Color( 95, 135, 135 ),	Color( 95, 135, 175 ),	Color( 95, 135, 215 ),
	Color( 95, 135, 255 ),	Color( 95, 175, 0 ),	Color( 95, 175, 95 ),
	Color( 95, 175, 135 ),	Color( 95, 175, 175 ),	Color( 95, 175, 215 ),
	Color( 95, 175, 255 ),	Color( 95, 215, 0 ),	Color( 95, 215, 95 ),
	Color( 95, 215, 135 ),	Color( 95, 215, 175 ),	Color( 95, 215, 215 ),
	Color( 95, 215, 255 ),	Color( 95, 255, 0 ),	Color( 95, 255, 95 ),
	Color( 95, 255, 135 ),	Color( 95, 255, 175 ),	Color( 95, 255, 215 ),
	Color( 95, 255, 255 ),	Color( 135, 0, 0 ),		Color( 135, 0, 95 ),
	Color( 135, 0, 135 ),	Color( 135, 0, 175 ),	Color( 135, 0, 215 ),
	Color( 135, 0, 255 ),	Color( 135, 95, 0 ),	Color( 135, 95, 95 ),
	Color( 135, 95, 135 ),	Color( 135, 95, 175 ),	Color( 135, 95, 215 ),
	Color( 135, 95, 255 ),	Color( 135, 135, 0 ),	Color( 135, 135, 95 ),
	Color( 135, 135, 135 ), Color( 135, 135, 175 ), Color( 135, 135, 215 ),
	Color( 135, 135, 255 ), Color( 135, 175, 0 ),	Color( 135, 175, 95 ),
	Color( 135, 175, 135 ), Color( 135, 175, 175 ), Color( 135, 175, 215 ),
	Color( 135, 175, 255 ), Color( 135, 215, 0 ),	Color( 135, 215, 95 ),
	Color( 135, 215, 135 ), Color( 135, 215, 175 ), Color( 135, 215, 215 ),
	Color( 135, 215, 255 ), Color( 135, 255, 0 ),	Color( 135, 255, 95 ),
	Color( 135, 255, 135 ), Color( 135, 255, 175 ), Color( 135, 255, 215 ),
	Color( 135, 255, 255 ), Color( 175, 0, 0 ),		Color( 175, 0, 95 ),
	Color( 175, 0, 135 ),	Color( 175, 0, 175 ),	Color( 175, 0, 215 ),
	Color( 175, 0, 255 ),	Color( 175, 95, 0 ),	Color( 175, 95, 95 ),
	Color( 175, 95, 135 ),	Color( 175, 95, 175 ),	Color( 175, 95, 215 ),
	Color( 175, 95, 255 ),	Color( 175, 135, 0 ),	Color( 175, 135, 95 ),
	Color( 175, 135, 135 ), Color( 175, 135, 175 ), Color( 175, 135, 215 ),
	Color( 175, 135, 255 ), Color( 175, 175, 0 ),	Color( 175, 175, 95 ),
	Color( 175, 175, 135 ), Color( 175, 175, 175 ), Color( 175, 175, 215 ),
	Color( 175, 175, 255 ), Color( 175, 215, 0 ),	Color( 175, 215, 95 ),
	Color( 175, 215, 135 ), Color( 175, 215, 175 ), Color( 175, 215, 215 ),
	Color( 175, 215, 255 ), Color( 175, 255, 0 ),	Color( 175, 255, 95 ),
	Color( 175, 255, 135 ), Color( 175, 255, 175 ), Color( 175, 255, 215 ),
	Color( 175, 255, 255 ), Color( 215, 0, 0 ),		Color( 215, 0, 95 ),
	Color( 215, 0, 135 ),	Color( 215, 0, 175 ),	Color( 215, 0, 215 ),
	Color( 215, 0, 255 ),	Color( 215, 95, 0 ),	Color( 215, 95, 95 ),
	Color( 215, 95, 135 ),	Color( 215, 95, 175 ),	Color( 215, 95, 215 ),
	Color( 215, 95, 255 ),	Color( 215, 135, 0 ),	Color( 215, 135, 95 ),
	Color( 215, 135, 135 ), Color( 215, 135, 175 ), Color( 215, 135, 215 ),
	Color( 215, 135, 255 ), Color( 215, 175, 0 ),	Color( 215, 175, 95 ),
	Color( 215, 175, 135 ), Color( 215, 175, 175 ), Color( 215, 175, 215 ),
	Color( 215, 175, 255 ), Color( 215, 215, 0 ),	Color( 215, 215, 95 ),
	Color( 215, 215, 135 ), Color( 215, 215, 175 ), Color( 215, 215, 215 ),
	Color( 215, 215, 255 ), Color( 215, 255, 0 ),	Color( 215, 255, 95 ),
	Color( 215, 255, 135 ), Color( 215, 255, 175 ), Color( 215, 255, 215 ),
	Color( 215, 255, 255 ), Color( 255, 0, 0 ),		Color( 255, 0, 95 ),
	Color( 255, 0, 135 ),	Color( 255, 0, 175 ),	Color( 255, 0, 215 ),
	Color( 255, 0, 255 ),	Color( 255, 95, 0 ),	Color( 255, 95, 95 ),
	Color( 255, 95, 135 ),	Color( 255, 95, 175 ),	Color( 255, 95, 215 ),
	Color( 255, 95, 255 ),	Color( 255, 135, 0 ),	Color( 255, 135, 95 ),
	Color( 255, 135, 135 ), Color( 255, 135, 175 ), Color( 255, 135, 215 ),
	Color( 255, 135, 255 ), Color( 255, 175, 0 ),	Color( 255, 175, 95 ),
	Color( 255, 175, 135 ), Color( 255, 175, 175 ), Color( 255, 175, 215 ),
	Color( 255, 175, 255 ), Color( 255, 215, 0 ),	Color( 255, 215, 95 ),
	Color( 255, 215, 135 ), Color( 255, 215, 175 ), Color( 255, 215, 215 ),
	Color( 255, 215, 255 ), Color( 255, 255, 0 ),	Color( 255, 255, 95 ),
	Color( 255, 255, 135 ), Color( 255, 255, 175 ), Color( 255, 255, 215 ),
	Color( 255, 255, 255 ), Color( 8, 8, 8 ),		Color( 18, 18, 18 ),
	Color( 28, 28, 28 ),	Color( 38, 38, 38 ),	Color( 48, 48, 48 ),
	Color( 58, 58, 58 ),	Color( 68, 68, 68 ),	Color( 78, 78, 78 ),
	Color( 88, 88, 88 ),	Color( 98, 98, 98 ),	Color( 108, 108, 108 ),
	Color( 118, 118, 118 ), Color( 128, 128, 128 ), Color( 138, 138, 138 ),
	Color( 148, 148, 148 ), Color( 158, 158, 158 ), Color( 168, 168, 168 ),
	Color( 178, 178, 178 ), Color( 188, 188, 188 ), Color( 198, 198, 198 ),
	Color( 208, 208, 208 ), Color( 218, 218, 218 ), Color( 228, 228, 228 ),
	Color( 238, 238, 238 ) };

std::shared_ptr<ETerminalDisplay>
ETerminalDisplay::Create( EE::Window::Window* window, Font* font,
						  std::shared_ptr<Hexe::Terminal::TerminalEmulator>&& terminalEmulator,
						  TerminalConfig* config ) {
	std::shared_ptr<ETerminalDisplay> terminal = std::shared_ptr<ETerminalDisplay>(
		new ETerminalDisplay( window, font, terminalEmulator->GetNumColumns(),
							  terminalEmulator->GetNumRows(), config ) );
	terminal->m_terminal = std::move( terminalEmulator );
	return terminal;
}

static Hexe::System::IProcessFactory* g_processFactory = new Hexe::System::ProcessFactory();

std::shared_ptr<ETerminalDisplay>
ETerminalDisplay::Create( EE::Window::Window* window, Font* font, int columns, int rows,
						  const std::string& program, const std::vector<std::string>& args,
						  const std::string& workingDir, uint32_t options,
						  Hexe::System::IProcessFactory* processFactory ) {
	using namespace Hexe::System;

	TerminalConfig config{};
	config.options = options;

	if ( processFactory == nullptr ) {
		processFactory = g_processFactory;
	}

	std::unique_ptr<IPseudoTerminal> pseudoTerminal = nullptr;
	std::vector<std::string> argsV( args.begin(), args.end() );
	auto process = processFactory->CreateWithPseudoTerminal( program, argsV, workingDir, columns,
															 rows, pseudoTerminal );

	if ( !pseudoTerminal ) {
		fprintf( stderr, "Failed to create pseudo terminal\n" );
		return nullptr;
	}

	if ( !process ) {
		fprintf( stderr, "Failed to spawn process\n" );
		return nullptr;
	}

	std::shared_ptr<ETerminalDisplay> terminal = std::shared_ptr<ETerminalDisplay>(
		new ETerminalDisplay( window, font, columns, rows, &config ) );
	terminal->m_terminal =
		TerminalEmulator::Create( std::move( pseudoTerminal ), std::move( process ), terminal );
	return terminal;
}

ETerminalDisplay::ETerminalDisplay( EE::Window::Window* window, Font* font, int columns, int rows,
									TerminalConfig* config ) :
	TerminalDisplay(), mWindow( window ), mFont( font ), m_columns( columns ), m_rows( rows ) {
	if ( config != nullptr ) {
		if ( config->options & OPTION_COLOR_EMOJI )
			m_useColorEmoji = true;
		if ( config->options & OPTION_NO_BOXDRAWING )
			m_useBoxDrawing = false;
		if ( config->options & OPTION_PASTE_CRLF )
			m_pasteNewlineFix = true;
	}

	Hexe::Terminal::Glyph defaultGlyph;
	defaultGlyph.mode = ATTR_INVISIBLE;
	auto defaultColor = std::make_pair<Uint32, std::string>( 0U, "" );
	m_cursorg = defaultGlyph;
	m_colors.resize( eeARRAY_SIZE( colornames ), defaultColor );
	m_buffer.resize( m_columns * m_rows, defaultGlyph );
	( (int&)mMode ) |= MODE_FOCUSED;

	if ( config != nullptr ) {
		if ( config->options & OPTION_COLOR_EMOJI )
			m_useColorEmoji = true;
		if ( config->options & OPTION_NO_BOXDRAWING )
			m_useBoxDrawing = false;
		if ( config->options & OPTION_PASTE_CRLF )
			m_pasteNewlineFix = true;
	}
}

void ETerminalDisplay::ResetColors() {
	for ( Uint32 i = 0; i < eeARRAY_SIZE( colornames ); i++ ) {
		ResetColor( i, colornames[i] );
	}
}

int ETerminalDisplay::ResetColor( int index, const char* name ) {
	if ( !name ) {
		if ( index >= 0 && index < (int)m_colors.size() ) {
			Color col = 0x000000FF;

			if ( index < 256 )
				col = colormapped[index];

			m_colors[index].first = col;
			m_colors[index].second = "";
			return 0;
		}
	}

	if ( index >= 0 && index < (int)m_colors.size() ) {
		m_colors[index].first = Color::fromString( name );
		m_colors[index].second = name;
	}

	return 1;
}

void ETerminalDisplay::Update() {
	if ( m_terminal )
		m_terminal->Update();
}

bool ETerminalDisplay::HasTerminated() const {
	return m_terminal->HasExited();
}

void ETerminalDisplay::SetTitle( const char* title ) {}

void ETerminalDisplay::SetIconTitle( const char* title ) {}

void ETerminalDisplay::SetClipboard( const char* text ) {
	mClipboard = text;
	mWindow->getClipboard()->setText( mClipboard );
}

const char* ETerminalDisplay::GetClipboard() const {
	mClipboard = mWindow->getClipboard()->getText();
	return mClipboard.c_str();
}

bool ETerminalDisplay::DrawBegin( int columns, int rows ) {
	if ( columns != m_columns || rows != m_rows ) {
		Hexe::Terminal::Glyph defaultGlyph{};
		m_buffer.resize( columns * rows, defaultGlyph );
		m_columns = columns;
		m_rows = rows;
	}
	m_checkDirty = false;

	return ( ( mMode & MODE_VISIBLE ) != 0 );
}

void ETerminalDisplay::DrawLine( Line line, int x1, int y, int x2 ) {
	m_checkDirty = true;
	memcpy( &m_buffer[y * m_columns + x1], line, ( x2 - x1 ) * sizeof( Hexe::Terminal::Glyph ) );
	for ( int i = x1; i < x2; i++ ) {
		if ( m_terminal->selected( i, y ) ) {
			m_buffer[y * m_columns + i].mode |= ATTR_REVERSE;
		}
	}
}

void ETerminalDisplay::DrawCursor( int cx, int cy, Hexe::Terminal::Glyph g, int ox, int oy,
								   Hexe::Terminal::Glyph og ) {
	m_cursorx = cx;
	m_cursory = cy;
	m_cursorg = g;
}

void ETerminalDisplay::DrawEnd() {
	if ( m_checkDirty )
		m_dirty = true;
}

void ETerminalDisplay::Draw( const Rectf& contentArea ) {
	bool hasFocus = mWindow->hasFocus();

	bool modeFocus = mMode & MODE_FOCUSED;
	if ( hasFocus != modeFocus ) {
		if ( hasFocus ) {
			mMode |= MODE_FOCUSED | MODE_FOCUS;
		} else {
			mMode ^= MODE_FOCUS | MODE_FOCUSED;
		}
	} else {
		mMode ^= MODE_FOCUS;
	}

	Draw( Vector2i( contentArea.Left, contentArea.Top ), contentArea.getSize().asInt(), hasFocus );
}

static inline Color GetCol( unsigned int terminalColor,
							const std::vector<std::pair<Color, std::string>>& colors ) {
	if ( ( terminalColor & ( 1 << 24 ) ) == 0 ) {
		return colors[terminalColor & 0xFF].first;
	}
	return Color( ( terminalColor >> 16 ) & 0xFF, ( terminalColor >> 8 ) & 0xFF,
				  terminalColor & 0xFF, ( ~( ( terminalColor >> 25 ) & 0xFF ) ) & 0xFF );
}

#define MIN( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define MAX( a, b ) ( ( a ) < ( b ) ? ( b ) : ( a ) )
#define LEN( a ) ( sizeof( a ) / sizeof( a )[0] )
#define BETWEEN( x, a, b ) ( ( a ) <= ( x ) && ( x ) <= ( b ) )
#define IS_SET( flag ) ( ( mMode & ( flag ) ) != 0 )
#define DIV( n, d ) ( ( ( n ) + ( d ) / 2.0f ) / ( d ) )
#define DIVI( n, d ) ( ( ( n ) + ( d ) / 2 ) / ( d ) )
#define IM_COL32_R_SHIFT 24
#define IM_COL32_G_SHIFT 16
#define IM_COL32_B_SHIFT 8
#define IM_COL32_A_SHIFT 0

void ETerminalDisplay::Draw( Vector2i pos, const Sizei& clip_rect, bool hasFocus ) {
	if ( mClock.getElapsedTime().asSeconds() > 0.7 ) {
		mMode ^= MODE_BLINK;
		mClock.restart();
	}

	auto fontSize = (Float)mFont->getFontHeight( 12 );
	auto spaceCharAdvanceX = mFont->getGlyph( 'A', 12, false ).advance;

	auto width = m_columns * spaceCharAdvanceX;
	auto height = m_rows * fontSize;

	auto clipWidth = clip_rect.getWidth();
	auto clipHeight = clip_rect.getHeight();

	auto clipColumns = (int)std::floor( std::max( 1.0f, clipWidth / spaceCharAdvanceX ) );
	auto clipRows = (int)std::floor( std::max( 1.0f, clipHeight / fontSize ) );

	if ( width < clipWidth ) {
		pos.x = std::floor( pos.x + ( ( clipWidth - width ) / 2.0f ) );
	}
	if ( height < clipHeight ) {
		pos.y = std::floor( pos.y + ( ( clipHeight - height ) / 2.0f ) );
	}

	Primitives p;
	p.setColor( mEmulator->GetDefaultBackground() );
	p.drawRectangle( Rectf( pos.asFloat(), clip_rect.asFloat() ) );

	if ( clipColumns != m_terminal->GetNumColumns() || clipRows != m_terminal->GetNumRows() ) {
		m_terminal->Resize( clipColumns, clipRows );
	}

	float x = 0.0f;
	float y = std::floor( pos.y );
	const float line_height = fontSize;
	auto defaultFg = GetCol( mEmulator->GetDefaultForeground(), m_colors );
	auto defaultBg = GetCol( mEmulator->GetDefaultBackground(), m_colors );

	for ( int j = 0; j < m_rows; j++ ) {
		x = std::floor( pos.x );

		if ( pos.y + line_height * j > clip_rect.getWidth() )
			break;

		for ( int i = 0; i < m_columns; i++ ) {
			auto& glyph = m_buffer[j * m_columns + i];
			auto fg = GetCol( glyph.fg, m_colors );
			auto bg = GetCol( glyph.bg, m_colors );
			Color temp{ Color::Transparent };

			if ( ( glyph.mode & ATTR_BOLD_FAINT ) == ATTR_BOLD && BETWEEN( glyph.fg, 0, 7 ) )
				fg = GetCol( glyph.fg + 8, m_colors );

			if ( IS_SET( MODE_REVERSE ) ) {
				if ( fg == defaultFg ) {
					fg = defaultBg;
				} else {
					Uint32 red = ( fg.getValue() >> IM_COL32_R_SHIFT ) & 0xFF;
					Uint32 green = ( fg.getValue() >> IM_COL32_G_SHIFT ) & 0xFF;
					Uint32 blue = ( fg.getValue() >> IM_COL32_B_SHIFT ) & 0xFF;
					Uint32 alpha = ( fg.getValue() >> IM_COL32_A_SHIFT ) & 0xFF;
					fg = Color( ( ~red ) & 0xFF, ( ~green ) & 0xFF, ( ~blue ) & 0xFF, alpha );
				}

				if ( bg == defaultBg ) {
					bg = defaultFg;
				} else {
					Uint32 red = ( bg.getValue() >> IM_COL32_R_SHIFT ) & 0xFF;
					Uint32 green = ( bg.getValue() >> IM_COL32_G_SHIFT ) & 0xFF;
					Uint32 blue = ( bg.getValue() >> IM_COL32_B_SHIFT ) & 0xFF;
					Uint32 alpha = ( bg.getValue() >> IM_COL32_A_SHIFT ) & 0xFF;
					bg = Color( ( ~red ) & 0xFF, ( ~green ) & 0xFF, ( ~blue ) & 0xFF, alpha );
				}
			}

			if ( ( glyph.mode & ATTR_BOLD_FAINT ) == ATTR_FAINT ) {
				Uint32 red = ( fg.getValue() >> IM_COL32_R_SHIFT ) & 0xFF;
				Uint32 green = ( fg.getValue() >> IM_COL32_G_SHIFT ) & 0xFF;
				Uint32 blue = ( fg.getValue() >> IM_COL32_B_SHIFT ) & 0xFF;
				Uint32 alpha = ( fg.getValue() >> IM_COL32_A_SHIFT ) & 0xFF;
				red /= 2;
				green /= 2;
				blue /= 2;
				fg = Color( red, green, blue, alpha );
			}

			if ( glyph.mode & ATTR_REVERSE ) {
				temp = fg;
				fg = bg;
				bg = temp;
			}

			if ( glyph.mode & ATTR_BLINK && mMode & MODE_BLINK )
				fg = bg;

			if ( glyph.mode & ATTR_INVISIBLE )
				fg = bg;

			bool isWide = glyph.mode & ATTR_WIDE;

			auto advanceX = spaceCharAdvanceX * ( isWide ? 2.0f : 1.0f );

			if ( glyph.mode & ATTR_WDUMMY || glyph.u == 32 ) {
				continue;
			}

			Text t( mFont, 12 );
			t.setDisableCacheWidth( true );
			t.setBackgroundColor( bg );
			t.setColor( fg );
			t.setStyle( 0 | ( glyph.mode & ATTR_BOLD ? Text::Style::Bold : 0 ) |
						( glyph.mode & ATTR_ITALIC ? Text::Style::Italic : 0 ) |
						( glyph.mode & ATTR_UNDERLINE ? Text::Style::Underlined : 0 ) );
			String str;
			str.push_back( glyph.u );
			t.setString( str );
			t.draw( x, y );

			/*if ( glyph.mode & ATTR_BOXDRAW && m_useBoxDrawing ) {
				auto bd = boxdrawindex( &glyph );
				drawbox( x, y, advanceX, line_height, fg, bg, bd, vtx_write, idx_write,
						 vtx_current_idx, drawList->_Data->TexUvWhitePixel );
			}

			if ( glyph.mode & ATTR_STRUCK ) {
				ImVec2 a( x, y + 2 * ascent / 3 );
				ImVec2 c( x + advanceX, a.y + 1 );
				ImVec2 b( c.x, a.y ), d( a.x, c.y ), uv( drawList->_Data->TexUvWhitePixel );
				idx_write[0] = vtx_current_idx;
				idx_write[1] = (ImDrawIdx)( vtx_current_idx + 1 );
				idx_write[2] = (ImDrawIdx)( vtx_current_idx + 2 );
				idx_write[3] = vtx_current_idx;
				idx_write[4] = (ImDrawIdx)( vtx_current_idx + 2 );
				idx_write[5] = (ImDrawIdx)( vtx_current_idx + 3 );
				vtx_write[0].pos = a;
				vtx_write[0].uv = uv;
				vtx_write[0].col = fg;
				vtx_write[1].pos = b;
				vtx_write[1].uv = uv;
				vtx_write[1].col = fg;
				vtx_write[2].pos = c;
				vtx_write[2].uv = uv;
				vtx_write[2].col = fg;
				vtx_write[3].pos = d;
				vtx_write[3].uv = uv;
				vtx_write[3].col = fg;
				vtx_write += 4;
				vtx_current_idx += 4;
				idx_write += 6;
			}*/

			x += advanceX;
		}

		y += line_height;
	}

	if ( !IS_SET( MODE_HIDE ) ) {
		Color drawcol;

		m_cursorg.mode &=
			ATTR_BOLD | ATTR_ITALIC | ATTR_UNDERLINE | ATTR_STRUCK | ATTR_WIDE | ATTR_BOXDRAW;
		if ( IS_SET( MODE_REVERSE ) ) {
			m_cursorg.mode |= ATTR_REVERSE;
			m_cursorg.bg = mEmulator->GetDefaultForeground();
			if ( mEmulator->IsSelected( m_cursorx, m_cursory ) ) {
				drawcol = GetCol( mEmulator->GetDefaultCursorColor(), m_colors );
				m_cursorg.fg = mEmulator->GetDefaultReverseCursorColor();
			} else {
				drawcol = GetCol( mEmulator->GetDefaultReverseCursorColor(), m_colors );
				m_cursorg.fg = mEmulator->GetDefaultCursorColor();
			}
		} else {
			if ( mEmulator->IsSelected( m_cursorx, m_cursory ) ) {
				m_cursorg.fg = mEmulator->GetDefaultForeground();
				m_cursorg.bg = mEmulator->GetDefaultReverseCursorColor();
			} else {
				m_cursorg.fg = mEmulator->GetDefaultBackground();
				m_cursorg.bg = mEmulator->GetDefaultCursorColor();
			}
			drawcol = GetCol( m_cursorg.bg, m_colors );
		}

		Vector2f a{}, b{}, c{}, d{};

		auto borderpx = 1.f;
		auto cursorthickness = 2.f;

		p.setColor( drawcol );
		/* draw the new one */
		if ( IS_SET( MODE_FOCUSED ) ) {
			switch ( mCursorMode ) {
				case 7:					  /* st extension */
					m_cursorg.u = 0x2603; /* snowman (U+2603) */
										  /* FALLTHROUGH */
				case 0:					  /* Blinking Block */
				case 1:					  /* Blinking Block (Default) */
				case 2:					  /* Steady Block */
										  // TODO: Implement cursor glyph rendering
										  // xdrawglyph(g, cx, cy);
										  // break;
				case 3:					  /* Blinking Underline */
				case Hexe::Terminal::MAX_CURSOR:
				case 4: /* Steady Underline */
					p.drawRectangle( Rectf(
						{ pos.x + borderpx + m_cursorx * spaceCharAdvanceX,
						  pos.y + borderpx + ( m_cursory + 1 ) * line_height - cursorthickness },
						{ spaceCharAdvanceX, cursorthickness } ) );
					break;
				case 5: /* Blinking bar */
				case 6: /* Steady bar */
					p.drawRectangle( Rectf( { pos.x + borderpx + m_cursorx * spaceCharAdvanceX,
											  pos.y + m_cursory * line_height },
											{ spaceCharAdvanceX, line_height } ) );
					break;
			}
		} else {
			p.setFillMode( PrimitiveFillMode::DRAW_LINE );
			p.drawRectangle( Rectf( { pos.x + borderpx + m_cursorx * spaceCharAdvanceX,
									  pos.y + borderpx + m_cursory * line_height },
									{ spaceCharAdvanceX, line_height } ) );
		}
	}

	/*if ( hasFocus ) {
		auto mousePos = mWindow->getInput()->getMousePos();
		auto relPos = Vector2i{ mousePos.x - pos.x, mousePos.y - pos.y };
		int mouseX = 0;
		int mouseY = 0;

		if ( mousePos.x <= 0.0f || mousePos.y <= 0.0f ) {
			mouseX = 0;
			mouseY = 0;
		} else if ( relPos.x >= 0.0f && relPos.y >= 0.0f ) {
			mouseX = eeclamp( (int)std::floor( relPos.x / spaceCharAdvanceX ), 0, clipColumns );
			mouseY = eeclamp( (int)std::floor( relPos.y / fontSize ), 0, clipRows );
		}

		ProcessInput( mouseX, mouseY );
	}*/
}
