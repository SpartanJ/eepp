#include "utest.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <climits>
#include <cstdlib>
#include <deque>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/glyphdrawable.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/graphics/textlayout.hpp>
#include <eepp/system/base64.hpp>
#include <eepp/system/compression.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollview.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/cursormanager.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/inputevent.hpp>
#include <eepp/window/runtime.hpp>
#include <eterm/system/iprocess.hpp>
#include <eterm/system/processfactory.hpp>
#include <eterm/terminal/ipseudoterminal.hpp>
#include <eterm/terminal/iterminaldisplay.hpp>
#include <eterm/terminal/terminaldisplay.hpp>
#include <eterm/terminal/terminalemulator.hpp>
#include <eterm/terminal/terminalgraphics.hpp>
#include <eterm/terminal/terminalsearch.hpp>
#include <eterm/terminal/terminalsession.hpp>
#include <eterm/ui/uiterminal.hpp>
#include <limits>
#include <thread>

using namespace eterm::Terminal;
using namespace eterm::System;
using namespace EE::System;

class MockPty : public IPseudoTerminal {
  public:
	std::string mBuffer;
	std::string mWrites;
	bool mLoopWrites{ true };
	size_t mMaxRead{ std::numeric_limits<size_t>::max() };
	std::deque<size_t> mReadSizes;
	size_t mReadOffset{ 0 };
	std::atomic<size_t> mBytesRead{ 0 };
	int mCols = 80;
	int mRows = 24;
	int mPixelWidth = 0;
	int mPixelHeight = 0;
	int getNumColumns() const override { return mCols; }
	int getNumRows() const override { return mRows; }
	bool resize( int columns, int rows, int pixelWidth, int pixelHeight ) override {
		mCols = columns;
		mRows = rows;
		mPixelWidth = pixelWidth;
		mPixelHeight = pixelHeight;
		return true;
	}
	bool isTTY() const override { return true; }
	int write( const char* s, size_t n ) override {
		mWrites.append( s, n );
		if ( mLoopWrites )
			mBuffer.append( s, n );
		return n;
	}
	int read( char* buf, size_t n, bool ) override {
		if ( mReadOffset == mBuffer.size() )
			return 0;
		size_t readLimit = mMaxRead;
		if ( !mReadSizes.empty() ) {
			readLimit = mReadSizes.front();
			mReadSizes.pop_front();
		}
		size_t toRead = std::min( { n, mBuffer.size() - mReadOffset, readLimit } );
		memcpy( buf, mBuffer.data() + mReadOffset, toRead );
		mReadOffset += toRead;
		mBytesRead.fetch_add( toRead, std::memory_order_relaxed );
		return toRead;
	}
};

class MockProcess : public IProcess {
  public:
	std::atomic<bool> mExited{ false };
	void checkExitStatus() override {}
	bool hasExited() const override { return mExited.load(); }
	int getExitCode() const override { return 0; }
	void terminate() override {}
	void waitForExit() override {}
	int pid() override { return 123; }
};

class SizingProcessFactory : public ProcessFactory {
  public:
	int columns{ 0 };
	int rows{ 0 };

	std::unique_ptr<IProcess>
	createWithPseudoTerminal( const std::string&, const std::vector<std::string>&,
							  const std::string&, int numColumns, int numRows,
							  std::unique_ptr<IPseudoTerminal>& outPseudoTerminal,
							  const std::unordered_map<std::string, std::string>& ) override {
		columns = numColumns;
		rows = numRows;
		auto pty = std::make_unique<MockPty>();
		pty->mCols = columns;
		pty->mRows = rows;
		outPseudoTerminal = std::move( pty );
		return std::make_unique<MockProcess>();
	}
};

static std::shared_ptr<const TerminalSnapshot>
waitForSnapshot( const std::shared_ptr<TerminalSession>& session,
				 const std::function<bool( const TerminalSnapshot& )>& predicate,
				 std::chrono::milliseconds timeout = std::chrono::milliseconds( 1000 ) ) {
	const auto deadline = std::chrono::steady_clock::now() + timeout;
	while ( std::chrono::steady_clock::now() < deadline ) {
		auto snapshot = session->snapshot();
		if ( snapshot && predicate( *snapshot ) )
			return snapshot;
		std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
	}
	return nullptr;
}

UTEST( eterm_display, startup_uses_default_grid_before_layout ) {
	EE::UI::UIApplication app(
		WindowSettings( 640, 480, "eterm startup dimensions", WindowStyle::Hidden ),
		EE::UI::UIApplication::Settings( Sys::getProcessPath() + "../", 1 ) );
	auto* font = app.getUI()->getUIThemeManager()->getDefaultFont();
	SizingProcessFactory factory;
	for ( const Sizef& size :
		  { Sizef{ 0, 0 }, Sizef{ 16, 16 }, Sizef{ 640, 0 }, Sizef{ 0, 480 } } ) {
		auto display = TerminalDisplay::create( app.getWindow(), font, 12, size, "shell", {}, "",
												100, &factory, false );
		ASSERT_TRUE( display != nullptr );
		EXPECT_EQ( 80, factory.columns );
		EXPECT_EQ( 24, factory.rows );
		ASSERT_TRUE(
			waitForSnapshot( display->getSession(), []( const TerminalSnapshot& snapshot ) {
				return snapshot.columns == 80 && snapshot.rows == 24;
			} ) != nullptr );
		display->update( false );
		EXPECT_EQ( 24, display->rowCount() );
	}
}

UTEST( eterm_display, scene_fonts_fall_back_to_the_application_emoji_font ) {
	EE::UI::UIApplication app(
		WindowSettings( 320, 240, "eterm emoji fallback", WindowStyle::Hidden ),
		EE::UI::UIApplication::Settings( Sys::getProcessPath() + "../", 1 ) );
	// eterm loads its terminal font into the UI scene's scope; that scope's FontService must find
	// the emoji font UIApplication loads into the default scope, or emoji render blank.
	auto font = FontTrueType::New( "eterm-test-monospace", *app.getUI()->getResourceScope() );
	ASSERT_TRUE( font->loadFromFile( "assets/fonts/DejaVuSansMono.ttf" ) );
	ASSERT_TRUE( font->getFontService() != nullptr );
	EXPECT_TRUE( font->getFontService()->getColorEmojiFont() != nullptr ||
				 font->getFontService()->getEmojiFont() != nullptr );
	const Glyph emoji = font->getGlyph( 0x1F600, 24, false, false );
	EXPECT_TRUE( emoji.bounds.getWidth() > 0 && emoji.bounds.getHeight() > 0 );
}

UTEST( eterm_display, shaped_cluster_glyphs_share_the_terminal_font_texture ) {
	EE::UI::UIApplication app(
		WindowSettings( 320, 240, "eterm cluster shaping", WindowStyle::Hidden ),
		EE::UI::UIApplication::Settings( Sys::getProcessPath() + "../", 1 ) );
	auto font = FontTrueType::New( "eterm-test-clusters", *app.getUI()->getResourceScope() );
	ASSERT_TRUE( font->loadFromFile( "assets/fonts/DejaVuSansMono.ttf" ) );
	if ( !Text::TextShaperEnabled )
		UTEST_SKIP( "Built without the text shaper" );
	// The display draws every cluster glyph with the terminal font's texture bound, including
	// glyphs that shaping took from a fallback font such as the emoji font.
	const std::u32string clusters[] = { U"e\u0301", U"\u2764\uFE0F", U"\U0001F1E6\U0001F1F7" };
	bool usedFallbackFont = false;
	for ( const auto& cluster : clusters ) {
		auto layout = TextLayout::layout( String::View( cluster.data(), cluster.size() ),
										  font.get(), 24, Text::Regular );
		ASSERT_FALSE( layout->paragraphs.empty() );
		ASSERT_FALSE( layout->paragraphs.front().shapedGlyphs.empty() );
		for ( const ShapedGlyph& shaped : layout->paragraphs.front().shapedGlyphs ) {
			EXPECT_NE( 0u, shaped.glyphIndex );
			usedFallbackFont = usedFallbackFont || shaped.font != font.get();
			GlyphDrawable* drawable = font->getShapedGlyphDrawable( shaped, 24 );
			ASSERT_TRUE( drawable != nullptr );
			EXPECT_TRUE( drawable->getTexture().get() == font->getTexture( 24 ).get() );
		}
	}
	EXPECT_TRUE( usedFallbackFont );

	// An emoji sequence is shaped by the emoji font as a whole, even when the terminal font has
	// some of its codepoints (DejaVu has U+2764 and the digits), so it forms one glyph.
	if ( !font->getFontService()->getColorEmojiFont() )
		return;
	const std::u32string sequences[] = { U"\U0001F469\u200D\u2764\uFE0F\u200D\U0001F468",
										 U"1\uFE0F\u20E3", U"\u2764\uFE0F" };
	for ( const auto& sequence : sequences ) {
		auto layout = TextLayout::layout( String::View( sequence.data(), sequence.size() ),
										  font.get(), 24, Text::Regular );
		ASSERT_FALSE( layout->paragraphs.empty() );
		const auto& glyphs = layout->paragraphs.front().shapedGlyphs;
		ASSERT_EQ( static_cast<size_t>( 1 ), glyphs.size() );
		EXPECT_NE( 0u, glyphs[0].glyphIndex );
		EXPECT_TRUE( glyphs[0].font == font->getFontService()->getColorEmojiFont() );
	}
	// Text presentation keeps the terminal font.
	const std::u32string text = U"\u2764\uFE0E";
	auto layout = TextLayout::layout( String::View( text.data(), text.size() ), font.get(), 24, 0 );
	ASSERT_FALSE( layout->paragraphs.front().shapedGlyphs.empty() );
	EXPECT_TRUE( layout->paragraphs.front().shapedGlyphs[0].font == font.get() );
}

UTEST( eterm_display, startup_uses_grid_from_available_pixels ) {
	EE::UI::UIApplication app(
		WindowSettings( 640, 480, "eterm startup dimensions", WindowStyle::Hidden ),
		EE::UI::UIApplication::Settings( Sys::getProcessPath() + "../", 1 ) );
	auto* font = app.getUI()->getUIThemeManager()->getDefaultFont();
	const Float cellWidth = font->getGlyph( 'A', 12, false, false ).advance;
	const Float cellHeight = font->getFontHeight( 12 );
	SizingProcessFactory factory;
	auto display =
		TerminalDisplay::create( app.getWindow(), font, 12, { cellWidth * 100, cellHeight * 30 },
								 "shell", {}, "", 100, &factory, false );
	ASSERT_TRUE( display != nullptr );
	EXPECT_EQ( 100, factory.columns );
	EXPECT_EQ( 30, factory.rows );
}

UTEST( eterm_display, placeholder_grid_resizes_after_layout ) {
	EE::UI::UIApplication app(
		WindowSettings( 640, 480, "eterm layout dimensions", WindowStyle::Hidden ),
		EE::UI::UIApplication::Settings( Sys::getProcessPath() + "../", 1 ) );
	auto* font = app.getUI()->getUIThemeManager()->getDefaultFont();
	SizingProcessFactory factory;
	auto display = TerminalDisplay::create( app.getWindow(), font, 12, { 16, 16 }, "shell", {}, "",
											100, &factory, false );
	ASSERT_TRUE( display != nullptr );
	ASSERT_TRUE( waitForSnapshot( display->getSession(), []( const TerminalSnapshot& snapshot ) {
					 return snapshot.columns == 80 && snapshot.rows == 24;
				 } ) != nullptr );
	display->update( false );

	const Float cellWidth = font->getGlyph( 'A', 12, false, false ).advance;
	const Float cellHeight = font->getFontHeight( 12 );
	display->setSize( { cellWidth * 100, cellHeight * 30 } );
	ASSERT_TRUE( waitForSnapshot( display->getSession(), []( const TerminalSnapshot& snapshot ) {
					 return snapshot.columns == 100 && snapshot.rows == 30;
				 } ) != nullptr );
	display->update( false );
	EXPECT_EQ( 30, display->rowCount() );

	// Collapsed or not-yet-laid-out views must not send a one-row resize to the shell either.
	display->setSize( { 0, 0 } );
	ASSERT_TRUE( waitForSnapshot( display->getSession(), []( const TerminalSnapshot& snapshot ) {
					 return snapshot.columns == 80 && snapshot.rows == 24;
				 } ) != nullptr );
}

class SelectionTestDisplay : public TerminalDisplay {
  public:
	SelectionTestDisplay( EE::Window::Window* window, Font* font,
						  std::shared_ptr<TerminalSession> session ) :
		TerminalDisplay( window, font, 12, { 400, 200 }, false ) {
		mPadding = { 10, 10, 10, 10 };
		mSession = std::move( session );
		consumeSnapshot();
	}

	using TerminalDisplay::positionToGrid;
};

UTEST( eterm_ui, applicationWheelDoesNotScrollHistoryOrAncestor ) {
	EE::UI::UIApplication app(
		WindowSettings( 640, 480, "eterm wheel propagation", WindowStyle::Hidden ),
		EE::UI::UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(),
										 1 ) );
	for ( bool alternateScreen : { false, true } ) {
		auto pty = std::make_unique<MockPty>();
		for ( int line = 0; line < 80; ++line )
			pty->mBuffer += "wheel history\r\n";
		pty->mBuffer += alternateScreen ? "\033[?1049h" : "\033[?1000h";
		pty->mLoopWrites = false;
		auto session =
			TerminalSession::create( std::move( pty ), std::make_unique<MockProcess>(), 100 );
		ASSERT_TRUE(
			waitForSnapshot( session, [alternateScreen]( const TerminalSnapshot& snapshot ) {
				return alternateScreen ? snapshot.altScreen
									   : ( snapshot.windowMode & MODE_MOUSEBTN ) != 0;
			} ) != nullptr );
		auto display = std::make_shared<SelectionTestDisplay>(
			app.getWindow(), app.getUI()->getUIThemeManager()->getDefaultFont(), session );
		EXPECT_EQ( alternateScreen, display->isAltScr() );
		EXPECT_EQ( !alternateScreen, display->isAppCapturingMouse() );
		auto* scroll = EE::UI::UIScrollView::New();
		scroll->setPixelsSize( 500, 300 );
		scroll->setParent( app.getUI()->getRoot() );
		auto* content = EE::UI::UIWidget::New();
		content->setPixelsSize( 450, 900 );
		content->setParent( scroll );
		int propagatedWheelEvents = 0;
		content->on( EE::Scene::Event::MouseWheel,
					 [&]( const EE::Scene::Event* ) { ++propagatedWheelEvents; } );
		auto* terminal = eterm::UI::UITerminal::New( display );
		terminal->setPixelsSize( 400, 200 );
		terminal->setParent( content );
		app.getUI()->flushDirtyStyleAndLayout();
		ASSERT_TRUE( scroll->getVerticalScrollBar()->isEnabled() );
		auto* input = app.getWindow()->getInput();
		input->setMousePos( terminal->convertToWorldSpace( { 40.f, 40.f } ).asInt() );
		app.getUI()->update( Milliseconds( 16 ) );
		const Float previousValue = terminal->getVerticalScrollBar()->getValue();
		InputEvent wheel{};
		wheel.Type = InputEvent::MouseWheel;
		wheel.WinID = app.getWindow()->getWindowID();
		wheel.wheel = { 0, 0.f, 1.f, InputEvent::WheelEvent::Normal };
		input->processEventForWindow( &wheel );
		app.getUI()->update( Milliseconds( 16 ) );
		EXPECT_NEAR( previousValue, terminal->getVerticalScrollBar()->getValue(), 0.0001f );
		wheel.wheel.y = 0.f;
		wheel.wheel.x = 1.f;
		input->processEventForWindow( &wheel );
		app.getUI()->update( Milliseconds( 16 ) );
		EXPECT_EQ( 0, propagatedWheelEvents );
		EXPECT_NEAR( 0.f, scroll->getVerticalScrollBar()->getValue(), 0.0001f );
		eeDelete( scroll );
	}
}

UTEST( eterm_display, selection_clamps_each_axis_independently ) {
	EE::UI::UIApplication app(
		WindowSettings( 640, 480, "eterm selection bounds", WindowStyle::Hidden ),
		EE::UI::UIApplication::Settings( Sys::getProcessPath() + "../", 1 ) );
	auto session = TerminalSession::create( std::make_unique<MockPty>(),
											std::make_unique<MockProcess>(), 100 );
	ASSERT_TRUE( waitForSnapshot( session, []( const TerminalSnapshot& snapshot ) {
					 return snapshot.columns == 80 && snapshot.rows == 24;
				 } ) != nullptr );
	SelectionTestDisplay display( app.getWindow(),
								  app.getUI()->getUIThemeManager()->getDefaultFont(), session );
	display.setPosition( { 100, 100 } );
	const auto cell = display.getCellPixelSize();
	const Vector2i inside{ 110 + 3 * cell.getWidth(), 110 + 4 * cell.getHeight() };
	const auto grid = display.positionToGrid( inside );
	ASSERT_GT( grid.x, 0 );
	ASSERT_GT( grid.y, 0 );

	for ( int x : { -50, 0, 90, 10000 } ) {
		const auto outside = display.positionToGrid( { x, inside.y } );
		EXPECT_EQ( grid.y, outside.y );
		EXPECT_EQ( x < 110 ? 0 : 80, outside.x );
	}
	for ( int y : { -50, 0, 90, 10000 } ) {
		const auto outside = display.positionToGrid( { inside.x, y } );
		EXPECT_EQ( grid.x, outside.x );
		EXPECT_EQ( y < 110 ? 0 : 23, outside.y );
	}
}

UTEST( eterm_display, selection_auto_scroll_uses_only_vertical_widget_bounds ) {
	EE::UI::UIApplication app(
		WindowSettings( 640, 480, "eterm selection auto-scroll", WindowStyle::Hidden ),
		EE::UI::UIApplication::Settings( Sys::getProcessPath() + "../", 1 ) );
	auto pty = std::make_unique<MockPty>();
	for ( int line = 0; line < 80; ++line )
		pty->mBuffer += "selection history\r\n";
	auto session =
		TerminalSession::create( std::move( pty ), std::make_unique<MockProcess>(), 100 );
	ASSERT_TRUE( waitForSnapshot( session, []( const TerminalSnapshot& snapshot ) {
					 return snapshot.historyLength > 40;
				 } ) != nullptr );
	SelectionTestDisplay display( app.getWindow(),
								  app.getUI()->getUIThemeManager()->getDefaultFont(), session );
	display.setPosition( { 100, 100 } );
	display.setClickStep( 1 );
	auto* input = app.getWindow()->getInput();
	input->injectButtonPress( EE_BUTTON_LEFT );
	display.onMouseDown( { 150, 150 }, EE_BUTTON_LMASK );

	// Keep the auto-scroll timer eligible and leave the input position stale deliberately.
	std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
	for ( const Vector2i& point :
		  { Vector2i{ -50, 150 }, Vector2i{ 700, 150 }, Vector2i{ -50, 100 }, Vector2i{ 700, 105 },
			Vector2i{ -50, 295 }, Vector2i{ 700, 300 } } ) {
		display.onMouseMove( point, EE_BUTTON_LMASK );
		session->requestSelection(); // Wait for the worker to process the motion.
		EXPECT_EQ( 0, session->snapshot()->scrollPosition );
	}

	// Crossing the top and bottom must still scroll even when X is outside the widget.
	display.onMouseMove( { -50, 99 }, EE_BUTTON_LMASK );
	ASSERT_TRUE( waitForSnapshot( session, []( const TerminalSnapshot& snapshot ) {
					 return snapshot.scrollPosition == 1;
				 } ) != nullptr );
	std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
	display.onMouseMove( { 700, 301 }, EE_BUTTON_LMASK );
	ASSERT_TRUE( waitForSnapshot( session, []( const TerminalSnapshot& snapshot ) {
					 return snapshot.scrollPosition == 0;
				 } ) != nullptr );

	// Periodic updates must retain the unclamped Y coordinate outside the window.
	display.setPosition( { 100, 0 } );
	EE::Window::InputEvent motion( EE::Window::InputEvent::MouseMotion );
	motion.motion = {};
	motion.motion.x = -50;
	motion.motion.y = -1;
	input->processEvent( &motion );
	std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
	display.update( false );
	ASSERT_TRUE( waitForSnapshot( session, []( const TerminalSnapshot& snapshot ) {
					 return snapshot.scrollPosition == 1;
				 } ) != nullptr );
	input->injectButtonRelease( EE_BUTTON_LEFT );
	display.onMouseUp( { 150, 150 }, EE_BUTTON_LMASK );
}

UTEST( eterm_session, command_wakeup_and_snapshot_immutability ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto session = TerminalSession::create( std::move( pty ), std::move( process ), 100 );
	ASSERT_TRUE( session != nullptr );

	session->writeRaw( "ABC" );
	auto first = waitForSnapshot( session, []( const TerminalSnapshot& snapshot ) {
		return snapshot.cells.size() >= 3 && snapshot.cells[0].u == 'A' &&
			   snapshot.cells[1].u == 'B' && snapshot.cells[2].u == 'C';
	} );
	ASSERT_TRUE( first != nullptr );
	ASSERT_TRUE( first->graphics != nullptr );
	EXPECT_EQ( static_cast<Uint64>( 0 ), first->graphics->requiredUpdateSequence );
	const Uint64 firstGeneration = first->generation;

	session->writeRaw( "\rXYZ" );
	auto second = waitForSnapshot( session, [firstGeneration]( const TerminalSnapshot& snapshot ) {
		return snapshot.generation > firstGeneration && snapshot.cells[0].u == 'X';
	} );
	ASSERT_TRUE( second != nullptr );
	EXPECT_EQ( static_cast<Rune>( 'A' ), first->cells[0].u );
	EXPECT_TRUE( second->generation > first->generation );
}

UTEST( eterm_session, skipped_snapshot_generation_requires_full_redraw ) {
	TerminalSnapshot snapshot;
	snapshot.generation = 42;
	EXPECT_TRUE( snapshot.dirtyRowsFollow( 41 ) );
	EXPECT_FALSE( snapshot.dirtyRowsFollow( 40 ) );
}

UTEST( eterm_session, graphics_update_queue_preserves_order_and_payloads ) {
	TerminalGraphicsUpdateQueue queue;
	auto pixels = std::make_shared<const std::vector<Uint8>>( 16, 0x7F );
	TerminalGraphicsUpdate create;
	create.type = TerminalGraphicsUpdateType::CreateImage;
	create.imageId = 7;
	create.pixels = pixels;
	TerminalGraphicsUpdate patch;
	patch.type = TerminalGraphicsUpdateType::UpdateRegion;
	patch.imageId = 7;
	patch.pixels = pixels;

	EXPECT_EQ( static_cast<Uint64>( 1 ), queue.enqueue( std::move( create ) ) );
	EXPECT_EQ( static_cast<Uint64>( 2 ), queue.enqueue( std::move( patch ) ) );
	EXPECT_EQ( static_cast<size_t>( 32 ), queue.queuedBytes() );
	auto updates = queue.drain();
	ASSERT_EQ( static_cast<size_t>( 2 ), updates.size() );
	EXPECT_EQ( static_cast<Uint64>( 1 ), updates[0].sequence );
	EXPECT_EQ( static_cast<Uint64>( 2 ), updates[1].sequence );
	EXPECT_EQ( TerminalGraphicsUpdateType::CreateImage, updates[0].type );
	EXPECT_EQ( TerminalGraphicsUpdateType::UpdateRegion, updates[1].type );
	EXPECT_TRUE( pixels == updates[0].pixels );
}

UTEST( eterm_session, graphics_update_queue_overflow_requires_resync ) {
	TerminalGraphicsUpdateQueue queue( 2, 8 );
	TerminalGraphicsUpdate update;
	update.type = TerminalGraphicsUpdateType::UpdateRegion;
	update.pixels = std::make_shared<const std::vector<Uint8>>( 8, 0xFF );
	queue.enqueue( update );
	queue.enqueue( std::move( update ) );

	EXPECT_TRUE( queue.needsResync() );
	auto updates = queue.drain();
	ASSERT_EQ( static_cast<size_t>( 1 ), updates.size() );
	EXPECT_EQ( TerminalGraphicsUpdateType::Resync, updates[0].type );
	EXPECT_EQ( static_cast<Uint64>( 2 ), updates[0].sequence );
	EXPECT_EQ( static_cast<size_t>( 0 ), queue.queuedBytes() );
}

UTEST( eterm_session, graphics_update_queue_coalesces_superseded_video_frames ) {
	TerminalGraphicsUpdateQueue queue( 4, 8 );
	TerminalGraphicsUpdate create;
	create.type = TerminalGraphicsUpdateType::CreateImage;
	create.imageId = 7;
	create.pixels = std::make_shared<const std::vector<Uint8>>( 8, 1 );
	EXPECT_EQ( static_cast<Uint64>( 1 ), queue.enqueue( std::move( create ) ) );
	for ( Uint8 frame = 2; frame < 20; ++frame ) {
		TerminalGraphicsUpdate replacement;
		replacement.type = TerminalGraphicsUpdateType::ReplaceImage;
		replacement.imageId = 7;
		replacement.pixels = std::make_shared<const std::vector<Uint8>>( 8, frame );
		EXPECT_EQ( static_cast<Uint64>( 1 ), queue.enqueue( std::move( replacement ) ) );
	}
	EXPECT_FALSE( queue.needsResync() );
	EXPECT_EQ( static_cast<size_t>( 8 ), queue.queuedBytes() );
	auto updates = queue.drain();
	ASSERT_EQ( static_cast<size_t>( 1 ), updates.size() );
	EXPECT_EQ( TerminalGraphicsUpdateType::CreateImage, updates[0].type );
	EXPECT_EQ( static_cast<Uint8>( 19 ), updates[0].pixels->front() );
}

UTEST( eterm_session, ordered_selection_request ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "ordered selection";
	auto process = std::make_unique<MockProcess>();
	auto session = TerminalSession::create( std::move( pty ), std::move( process ), 100 );
	ASSERT_TRUE( waitForSnapshot( session, []( const TerminalSnapshot& snapshot ) {
					 return !snapshot.cells.empty() && snapshot.cells[0].u == 'o';
				 } ) != nullptr );

	session->selectionStart( 0, 0, 0 );
	session->selectionExtend( 6, 0, SEL_REGULAR, false );
	auto selection = session->requestSelection();
	ASSERT_TRUE( selection.has_value() );
	EXPECT_STDSTREQ( "ordered", *selection );
}

UTEST( eterm_session, loaded_command_latency_stays_bounded ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer.assign( 32 * 1024 * 1024, 'L' );
	pty->mMaxRead = 64;
	MockPty* ptyPtr = pty.get();
	auto process = std::make_unique<MockProcess>();
	auto session = TerminalSession::create( std::move( pty ), std::move( process ), 100 );
	const auto readDeadline = std::chrono::steady_clock::now() + std::chrono::milliseconds( 100 );
	while ( ptyPtr->mBytesRead.load( std::memory_order_relaxed ) == 0 &&
			std::chrono::steady_clock::now() < readDeadline )
		std::this_thread::yield();
	ASSERT_TRUE( ptyPtr->mBytesRead.load( std::memory_order_relaxed ) > 0 );

	const auto start = std::chrono::steady_clock::now();
	auto selection = session->requestSelection();
	const auto latency = std::chrono::steady_clock::now() - start;
	EXPECT_TRUE( selection.has_value() );
	EXPECT_TRUE( latency < std::chrono::milliseconds( 50 ) );
}

UTEST( eterm_session, resize_and_output_are_serialized ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto session = TerminalSession::create( std::move( pty ), std::move( process ), 100 );
	session->resize( 40, 12 );
	session->writeRaw( "after resize" );
	auto snapshot = waitForSnapshot( session, []( const TerminalSnapshot& value ) {
		return value.columns == 40 && value.rows == 12 && !value.cells.empty() &&
			   value.cells[0].u == 'a';
	} );
	ASSERT_TRUE( snapshot != nullptr );
	EXPECT_EQ( static_cast<size_t>( 40 * 12 ), snapshot->cells.size() );
}

UTEST( eterm_session, closing_a_tab_resize_keeps_surviving_session_alive ) {
	auto survivorPty = std::make_unique<MockPty>();
	survivorPty->mCols = 151;
	survivorPty->mRows = 52;
	for ( int line = 0; line < 200; ++line )
		survivorPty->mBuffer += "survivor history " + std::to_string( line ) + "\r\n";
	auto survivorProcess = std::make_unique<MockProcess>();
	auto survivor =
		TerminalSession::create( std::move( survivorPty ), std::move( survivorProcess ), 1000 );
	ASSERT_TRUE( waitForSnapshot( survivor, []( const TerminalSnapshot& snapshot ) {
					 return snapshot.columns == 151 && snapshot.rows == 52 &&
							snapshot.historyLength >= 100;
				 } ) != nullptr );

	auto temporaryPty = std::make_unique<MockPty>();
	temporaryPty->mCols = 151;
	temporaryPty->mRows = 52;
	auto temporaryProcess = std::make_unique<MockProcess>();
	auto temporary =
		TerminalSession::create( std::move( temporaryPty ), std::move( temporaryProcess ), 1000 );

	survivor->resize( 151, 50 );
	temporary->resize( 151, 50 );
	ASSERT_TRUE( waitForSnapshot( survivor, []( const TerminalSnapshot& snapshot ) {
					 return snapshot.columns == 151 && snapshot.rows == 50;
				 } ) != nullptr );
	ASSERT_TRUE( waitForSnapshot( temporary, []( const TerminalSnapshot& snapshot ) {
					 return snapshot.columns == 151 && snapshot.rows == 50;
				 } ) != nullptr );

	/* Queue the surviving resize before closing the temporary session so both workers can release
	 * and reallocate equal-sized terminal rows concurrently. */
	survivor->resize( 151, 52 );
	temporary.reset();
	ASSERT_TRUE( waitForSnapshot( survivor, []( const TerminalSnapshot& snapshot ) {
					 return snapshot.columns == 151 && snapshot.rows == 52 &&
							snapshot.historyLength >= 100;
				 } ) != nullptr );
}

UTEST( eterm_session, scroll_snapshots_acknowledge_the_latest_ordered_command ) {
	auto pty = std::make_unique<MockPty>();
	for ( int line = 0; line < 80; ++line )
		pty->mBuffer += "history " + std::to_string( line ) + "\r\n";
	auto process = std::make_unique<MockProcess>();
	auto session = TerminalSession::create( std::move( pty ), std::move( process ), 100 );
	ASSERT_TRUE( waitForSnapshot( session, []( const TerminalSnapshot& snapshot ) {
					 return snapshot.historyLength >= 25;
				 } ) != nullptr );

	const Uint64 firstCommand = session->scrollTo( 10 );
	const Uint64 secondCommand = session->scrollTo( 25 );
	EXPECT_EQ( firstCommand + 1, secondCommand );
	auto acknowledged =
		waitForSnapshot( session, [secondCommand]( const TerminalSnapshot& snapshot ) {
			return snapshot.lastAppliedScrollCommand == secondCommand;
		} );
	ASSERT_TRUE( acknowledged != nullptr );
	EXPECT_EQ( 25, acknowledged->scrollPosition );
}

UTEST( eterm_session, presentation_rate_is_applied_on_the_worker ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto session = TerminalSession::create( std::move( pty ), std::move( process ), 100 );
	session->setPresentationRate( 120 );
	ASSERT_TRUE( waitForSnapshot( session, []( const TerminalSnapshot& snapshot ) {
					 return snapshot.presentationRate == 120;
				 } ) != nullptr );
}

UTEST( eterm_session, focus_reporting_is_ordered_on_worker ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033[?1004h";
	for ( int line = 0; line < 40; ++line )
		pty->mBuffer += "Line " + std::to_string( line ) + "\r\n";
	pty->mLoopWrites = false;
	MockPty* ptyPtr = pty.get();
	auto process = std::make_unique<MockProcess>();
	auto session = TerminalSession::create( std::move( pty ), std::move( process ), 100 );
	auto enabled = waitForSnapshot( session, []( const TerminalSnapshot& snapshot ) {
		return snapshot.windowMode & MODE_FOCUS && snapshot.historyLength >= 5;
	} );
	ASSERT_TRUE( enabled != nullptr );
	const Uint64 scrollCommand = session->scrollTo( 5 );
	auto scrolled = waitForSnapshot( session, [scrollCommand]( const TerminalSnapshot& snapshot ) {
		return snapshot.lastAppliedScrollCommand == scrollCommand;
	} );
	ASSERT_TRUE( scrolled != nullptr );
	ASSERT_EQ( 5, scrolled->scrollPosition );

	session->setFocus( false );
	auto unfocused = waitForSnapshot( session, [scrolled]( const TerminalSnapshot& snapshot ) {
		return snapshot.generation > scrolled->generation &&
			   !( snapshot.windowMode & MODE_FOCUSED );
	} );
	ASSERT_TRUE( unfocused != nullptr );
	EXPECT_EQ( 5, unfocused->scrollPosition );
	ASSERT_TRUE( ptyPtr->mWrites.size() >= 3 );
	EXPECT_STDSTREQ( "\033[O", ptyPtr->mWrites.substr( ptyPtr->mWrites.size() - 3 ) );

	session->setFocus( true );
	auto refocused = waitForSnapshot( session, [unfocused]( const TerminalSnapshot& snapshot ) {
		return snapshot.generation > unfocused->generation && snapshot.windowMode & MODE_FOCUSED;
	} );
	ASSERT_TRUE( refocused != nullptr );
	EXPECT_EQ( 5, refocused->scrollPosition );
	ASSERT_TRUE( ptyPtr->mWrites.size() >= 3 );
	EXPECT_STDSTREQ( "\033[I", ptyPtr->mWrites.substr( ptyPtr->mWrites.size() - 3 ) );
}

UTEST( eterm_session, focus_commands_do_not_clear_selection ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "persistent selection";
	auto process = std::make_unique<MockProcess>();
	auto session = TerminalSession::create( std::move( pty ), std::move( process ), 100 );
	ASSERT_TRUE( waitForSnapshot( session, []( const TerminalSnapshot& snapshot ) {
					 return !snapshot.cells.empty() && snapshot.cells[0].u == 'p';
				 } ) != nullptr );

	session->selectionStart( 0, 0, 0 );
	session->selectionExtend( 9, 0, SEL_REGULAR, false );
	auto selected = session->requestSelection();
	ASSERT_TRUE( selected.has_value() );
	ASSERT_STDSTREQ( "persistent", *selected );

	session->setFocus( false );
	session->setFocus( true );
	auto afterFocusChange = session->requestSelection();
	ASSERT_TRUE( afterFocusChange.has_value() );
	EXPECT_STDSTREQ( "persistent", *afterFocusChange );
}

UTEST( eterm_session, replaceable_events_coalesce_without_losing_ordered_events ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto session = TerminalSession::create( std::move( pty ), std::move( process ), 100 );
	session->drainEvents();
	session->writeRaw( "\033]0;first\a\033]0;second\a" );
	ASSERT_TRUE( waitForSnapshot( session, []( const TerminalSnapshot& snapshot ) {
					 return snapshot.title == "second";
				 } ) != nullptr );

	int titleEvents = 0;
	std::string title;
	for ( auto& event : session->drainEvents() ) {
		if ( event.type == TerminalSession::EventType::Title ) {
			++titleEvents;
			title = std::move( event.data );
		}
	}
	EXPECT_EQ( 1, titleEvents );
	EXPECT_STDSTREQ( "second", title );
}

UTEST( eterm_session, ordered_events_are_coalescing_barriers ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto session = TerminalSession::create( std::move( pty ), std::move( process ), 100 );
	session->drainEvents();
	session->writeRaw( "\033]0;before\a\a\033]0;after\a" );
	ASSERT_TRUE( waitForSnapshot( session, []( const TerminalSnapshot& snapshot ) {
					 return snapshot.title == "after";
				 } ) != nullptr );

	std::vector<TerminalSession::EventType> semanticEvents;
	for ( const auto& event : session->drainEvents() ) {
		if ( event.type == TerminalSession::EventType::Title ||
			 event.type == TerminalSession::EventType::Bell )
			semanticEvents.emplace_back( event.type );
	}
	ASSERT_EQ( static_cast<size_t>( 3 ), semanticEvents.size() );
	EXPECT_EQ( TerminalSession::EventType::Title, semanticEvents[0] );
	EXPECT_EQ( TerminalSession::EventType::Bell, semanticEvents[1] );
	EXPECT_EQ( TerminalSession::EventType::Title, semanticEvents[2] );
}

UTEST( eterm_session, reset_is_ordered_and_publishes_immediately ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "content";
	auto process = std::make_unique<MockProcess>();
	auto session = TerminalSession::create( std::move( pty ), std::move( process ), 100 );
	auto populated = waitForSnapshot( session, []( const TerminalSnapshot& snapshot ) {
		return !snapshot.cells.empty() && snapshot.cells[0].u == 'c';
	} );
	ASSERT_TRUE( populated != nullptr );
	session->reset();
	auto reset = waitForSnapshot( session, [populated]( const TerminalSnapshot& snapshot ) {
		return snapshot.generation > populated->generation && !snapshot.cells.empty() &&
			   snapshot.cells[0].u == ' ';
	} );
	ASSERT_TRUE( reset != nullptr );
}

UTEST( eterm_session, process_exit_follows_buffered_output_and_final_snapshot ) {
	auto pty = std::make_unique<MockPty>();
	pty->mMaxRead = 1;
	MockPty* ptyPtr = pty.get();
	auto process = std::make_unique<MockProcess>();
	MockProcess* processPtr = process.get();
	auto session = TerminalSession::create( std::move( pty ), std::move( process ), 100 );
	session->setDataEventsEnabled( true );
	session->writeRaw( std::string( 3 * 1024, 'Q' ) );
	const auto readDeadline = std::chrono::steady_clock::now() + std::chrono::milliseconds( 100 );
	while ( ptyPtr->mBytesRead.load( std::memory_order_relaxed ) == 0 &&
			std::chrono::steady_clock::now() < readDeadline )
		std::this_thread::yield();
	ASSERT_TRUE( ptyPtr->mBytesRead.load( std::memory_order_relaxed ) > 0 );
	processPtr->mExited.store( true );

	auto finalSnapshot = waitForSnapshot(
		session, []( const TerminalSnapshot& snapshot ) { return snapshot.processExited; } );
	ASSERT_TRUE( finalSnapshot != nullptr );
	EXPECT_EQ( 0, finalSnapshot->exitCode );

	size_t bytesRead = 0;
	bool sawExit = false;
	for ( auto& event : session->drainEvents() ) {
		if ( event.type == TerminalSession::EventType::Data )
			bytesRead += event.data.size();
		else if ( event.type == TerminalSession::EventType::ProcessExit )
			sawExit = true;
	}
	EXPECT_EQ( static_cast<size_t>( 3 * 1024 ), bytesRead );
	EXPECT_TRUE( sawExit );
}

UTEST( eterm_session, repeated_create_destroy_and_concurrent_workers ) {
	for ( int iteration = 0; iteration < 16; ++iteration ) {
		std::vector<std::shared_ptr<TerminalSession>> sessions;
		for ( int terminal = 0; terminal < 4; ++terminal ) {
			auto pty = std::make_unique<MockPty>();
			auto process = std::make_unique<MockProcess>();
			auto session = TerminalSession::create( std::move( pty ), std::move( process ), 100 );
			session->writeRaw( "worker" + std::to_string( terminal ) );
			sessions.emplace_back( std::move( session ) );
		}
		for ( const auto& session : sessions ) {
			EXPECT_TRUE( waitForSnapshot( session, []( const TerminalSnapshot& snapshot ) {
							 return !snapshot.cells.empty() && snapshot.cells[0].u == 'w';
						 } ) != nullptr );
		}
	}
}

UTEST( eterm_session, concurrent_shutdown_is_idempotent ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto session = TerminalSession::create( std::move( pty ), std::move( process ), 100 );
	std::vector<std::thread> shutdownThreads;
	for ( int thread = 0; thread < 4; ++thread )
		shutdownThreads.emplace_back( [session] { session->shutdown(); } );
	for ( auto& thread : shutdownThreads )
		thread.join();
	session->shutdown();
}

class MockDisplay : public ITerminalDisplay {
  public:
	std::vector<std::string> mClipboardWrites;

	void setClipboard( const char* text ) override { mClipboardWrites.emplace_back( text ); }

	std::vector<TerminalGlyph> mCells;
	int mColumns{ 0 };
	int mDrawLines{ 0 };
	int mDrawEnds{ 0 };
	std::vector<int> mPublishedScrollPositions;
	std::string mTitle;
	uint32_t mFirstMode{ 0 };
	uint32_t mSecondMode{ 0 };
	TerminalGlyph mFirstGlyph;
	TerminalGlyph mSecondGlyph;
	std::vector<Uint32> mResetColorIndices;
	int mResetColorsCount{ 0 };
	Uint32 mBackground{ 0x101010FF };
	std::shared_ptr<TerminalGraphicsPresentation> mGraphics;
	bool drawBegin( Uint32 columns, Uint32 rows ) override {
		mColumns = columns;
		mCells.resize( columns * rows );
		return true;
	}
	void drawLine( Line line, int x1, int y, int x2 ) override {
		std::copy( line + x1, line + x2, mCells.begin() + y * mColumns + x1 );
		++mDrawLines;
		if ( y == 0 ) {
			mFirstMode = line[0].mode;
			mSecondMode = line[1].mode;
			mFirstGlyph = line[0];
			mSecondGlyph = line[1];
		}
	}
	void drawCursor( int, int, TerminalGlyph, int, int, TerminalGlyph ) override {}
	void drawEnd() override {
		++mDrawEnds;
		if ( mEmulator )
			mPublishedScrollPositions.emplace_back( mEmulator->scrollPos() );
	}
	void setTitle( const char* title ) override { mTitle = title ? title : ""; }
	void resetColors() override { ++mResetColorsCount; }
	void drawGraphics( std::shared_ptr<TerminalGraphicsPresentation> presentation,
					   std::vector<TerminalGraphicsUpdate> ) override {
		mGraphics = std::move( presentation );
	}
	int resetColor( const Uint32& index, const char* ) override {
		mResetColorIndices.emplace_back( index );
		return 0;
	}
	bool getColor( const Uint32& index, unsigned char* r, unsigned char* g,
				   unsigned char* b ) override {
		if ( index == 259 ) {
			*r = static_cast<unsigned char>( mBackground >> 24 );
			*g = static_cast<unsigned char>( mBackground >> 16 );
			*b = static_cast<unsigned char>( mBackground >> 8 );
			return true;
		}
		if ( index > 1 )
			return false;
		*r = static_cast<unsigned char>( 1 + index * 3 );
		*g = static_cast<unsigned char>( 2 + index * 3 );
		*b = static_cast<unsigned char>( 3 + index * 3 );
		return true;
	}
};

UTEST( eterm, kitty_graphics_parser_preserves_payload_and_types_action ) {
	auto result = KittyGraphicsProtocol::parse(
		"a=T,f=32,s=2,v=1,i=7,p=9,q=1,C=1,z=-3,future=value;AAAA;BBBB" );
	ASSERT_TRUE( result.command.has_value() );
	auto* transmit = std::get_if<KittyTransmitCommand>( &*result.command );
	ASSERT_TRUE( transmit != nullptr );
	EXPECT_TRUE( transmit->display );
	EXPECT_EQ( static_cast<Uint32>( 32 ), *transmit->data.format );
	EXPECT_EQ( static_cast<Uint32>( 2 ), *transmit->data.width );
	EXPECT_EQ( static_cast<Uint32>( 1 ), *transmit->data.height );
	EXPECT_EQ( static_cast<Int32>( -3 ), *transmit->data.zIndex );
	EXPECT_STDSTREQ( "AAAA;BBBB", std::string( transmit->data.payload ) );
}

UTEST( eterm, kitty_graphics_parser_rejects_invalid_control_data ) {
	EXPECT_EQ( KittyGraphicsError::InvalidArgument,
			   KittyGraphicsProtocol::parse( "a=T,m=2;AAAA" ).error );
	EXPECT_EQ( KittyGraphicsError::InvalidArgument,
			   KittyGraphicsProtocol::parse( "a=T,i=1,I=2;AAAA" ).error );
	EXPECT_EQ( KittyGraphicsError::InvalidArgument,
			   KittyGraphicsProtocol::parse( "a=T,s=4294967296;AAAA" ).error );
	EXPECT_EQ( KittyGraphicsError::InvalidArgument,
			   KittyGraphicsProtocol::parse( "a=unknown;AAAA" ).error );
}

UTEST( eterm, kitty_graphics_parser_fuzz_corpus_is_bounded_and_total ) {
	Uint32 state = 0xC0FFEEu;
	for ( size_t iteration = 0; iteration < 5000; ++iteration ) {
		state ^= state << 13;
		state ^= state >> 17;
		state ^= state << 5;
		const size_t length = state % 512;
		std::string input( length, '\0' );
		for ( char& character : input ) {
			state ^= state << 13;
			state ^= state >> 17;
			state ^= state << 5;
			character = static_cast<char>( state & 0x7F );
		}
		const auto result = KittyGraphicsProtocol::parse( input );
		EXPECT_TRUE( result.command.has_value() || result.error != KittyGraphicsError::None );
	}
}

UTEST( eterm, kitty_graphics_direct_rgba_chunks_create_worker_image ) {
	KittyGraphicsProtocol protocol;
	auto first = protocol.handle( "a=t,f=32,s=1,v=1,i=7,m=1;AQID" );
	EXPECT_EQ( KittyGraphicsError::None, first.error );
	EXPECT_FALSE( first.changed );
	auto final = protocol.handle( "m=0;BA==" );
	EXPECT_EQ( KittyGraphicsError::None, final.error );
	EXPECT_TRUE( final.changed );
	EXPECT_STDSTREQ( "\033_Gi=7;OK\033\\", final.response );

	auto pixels = protocol.imagePixels( 7 );
	ASSERT_TRUE( pixels != nullptr );
	ASSERT_EQ( static_cast<size_t>( 4 ), pixels->size() );
	EXPECT_EQ( static_cast<Uint8>( 1 ), ( *pixels )[0] );
	EXPECT_EQ( static_cast<Uint8>( 4 ), ( *pixels )[3] );
	auto updates = protocol.takeUpdates();
	ASSERT_EQ( static_cast<size_t>( 1 ), updates.size() );
	EXPECT_EQ( TerminalGraphicsUpdateType::CreateImage, updates[0].type );
}

UTEST( eterm, kitty_graphics_chunk_continuations_reject_metadata_and_wrong_action ) {
	KittyGraphicsProtocol protocol;
	EXPECT_EQ( KittyGraphicsError::None, protocol.handle( "a=t,f=32,s=1,v=1,i=8,m=1;AQID" ).error );
	EXPECT_EQ( KittyGraphicsError::InvalidArgument, protocol.handle( "m=0,s=1;BA==" ).error );
	EXPECT_EQ( static_cast<size_t>( 0 ), protocol.imageCount() );
	EXPECT_EQ( KittyGraphicsError::None, protocol.handle( "a=t,f=32,s=1,v=1,i=8,m=1;AQID" ).error );
	EXPECT_EQ( KittyGraphicsError::InvalidArgument, protocol.handle( "a=p,i=8" ).error );
	EXPECT_EQ( static_cast<size_t>( 0 ), protocol.imageCount() );

	protocol.handle( "a=t,f=32,s=1,v=1,i=8;AQIDBA==" );
	EXPECT_EQ( KittyGraphicsError::None, protocol.handle( "a=f,i=8,f=32,s=1,v=1,m=1;AQID" ).error );
	EXPECT_EQ( KittyGraphicsError::InvalidArgument, protocol.handle( "m=0;BA==" ).error );
}

UTEST( eterm, kitty_graphics_frame_chunks_repeat_frame_action ) {
	KittyGraphicsProtocol protocol;
	ASSERT_EQ( KittyGraphicsError::None, protocol.handle( "a=t,f=32,s=1,v=1,i=8;AQIDBA==" ).error );
	protocol.takeUpdates();
	EXPECT_EQ( KittyGraphicsError::None,
			   protocol.handle( "a=f,i=8,r=1,f=32,s=1,v=1,X=1,m=1;BQYH" ).error );
	const auto final = protocol.handle( "a=f,m=0;CA==" );
	EXPECT_EQ( KittyGraphicsError::None, final.error );
	EXPECT_TRUE( final.changed );
	const std::vector<Uint8> expected{ 5, 6, 7, 8 };
	ASSERT_TRUE( protocol.imagePixels( 8 ) != nullptr );
	EXPECT_TRUE( expected == *protocol.imagePixels( 8 ) );
}

UTEST( eterm, kitty_graphics_image_number_allocates_id_and_echoes_number ) {
	KittyGraphicsProtocol protocol;
	auto created = protocol.handle( "a=t,f=32,s=1,v=1,I=77;AQIDBA==" );
	EXPECT_TRUE( created.changed );
	EXPECT_TRUE( created.response.find( ",I=77;OK" ) != std::string::npos );
	auto placed = protocol.handle( "a=p,I=77,p=3" );
	EXPECT_EQ( KittyGraphicsError::None, placed.error );
	ASSERT_EQ( static_cast<size_t>( 1 ), protocol.takePresentation()->placements.size() );
}

UTEST( eterm, kitty_graphics_rgb_and_zlib_preserve_rgb24 ) {
	const std::vector<Uint8> rgb{ 10, 20, 30, 40, 50, 60 };
	std::vector<Uint8> compressed( Compression::getMaxCompressedBufferSize( rgb.size() ) );
	IOStreamMemory source( reinterpret_cast<const char*>( rgb.data() ), rgb.size() );
	IOStreamMemory destination( reinterpret_cast<char*>( compressed.data() ), compressed.size() );
	ASSERT_EQ( Compression::OK, Compression::compress( destination, source ) );
	compressed.resize( destination.tell() );
	std::string encoded;
	ASSERT_TRUE( Base64::encode(
		std::string_view( reinterpret_cast<const char*>( compressed.data() ), compressed.size() ),
		encoded ) );

	KittyGraphicsProtocol protocol;
	auto result = protocol.handle( "a=t,f=24,s=2,v=1,i=9,o=z;" + encoded );
	EXPECT_TRUE( result.changed );
	auto pixels = protocol.imagePixels( 9 );
	ASSERT_TRUE( pixels != nullptr );
	const std::vector<Uint8> expected{ 10, 20, 30, 40, 50, 60 };
	EXPECT_TRUE( expected == *pixels );
	auto updates = protocol.takeUpdates();
	ASSERT_EQ( static_cast<size_t>( 1 ), updates.size() );
	EXPECT_EQ( static_cast<Uint8>( 3 ), updates.front().channels );
	EXPECT_TRUE( updates.front().pixels && expected == *updates.front().pixels );
}

UTEST( eterm, memory_stream_supports_multichunk_compression_output ) {
	std::vector<Uint8> sourcePixels( 128 * 1024 );
	Uint32 state = 0x12345678;
	for ( Uint8& pixel : sourcePixels ) {
		state ^= state << 13;
		state ^= state >> 17;
		state ^= state << 5;
		pixel = static_cast<Uint8>( state );
	}
	std::vector<Uint8> compressed( Compression::getMaxCompressedBufferSize( sourcePixels.size() ) );
	IOStreamMemory source( reinterpret_cast<const char*>( sourcePixels.data() ),
						   sourcePixels.size() );
	IOStreamMemory destination( reinterpret_cast<char*>( compressed.data() ), compressed.size() );
	EXPECT_EQ( Compression::OK, Compression::compress( destination, source ) );
	EXPECT_TRUE( destination.tell() > 16 * 1024 );
}

UTEST( eterm, kitty_graphics_png_decodes_to_rgba ) {
	KittyGraphicsProtocol protocol;
	const auto result = protocol.handle( "a=t,f=100,i=10;"
										 "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42"
										 "mNk+A8AAQUBAScY42YAAAAASUVORK5CYII=" );
	EXPECT_EQ( KittyGraphicsError::None, result.error );
	EXPECT_TRUE( result.changed );
	const auto* pixels = protocol.imagePixels( 10 );
	ASSERT_TRUE( pixels != nullptr );
	EXPECT_EQ( static_cast<size_t>( 4 ), pixels->size() );
}

UTEST( eterm, kitty_graphics_placement_uses_final_cursor_and_geometry ) {
	KittyGraphicsProtocol protocol;
	protocol.handle( "a=T,f=32,s=1,v=1,i=21,p=4,c=2,r=3,C=1,x=0,y=0,w=1,h=1;AQIDBA==",
					 Vector2i( 5, 6 ) );
	auto presentation = protocol.takePresentation();
	ASSERT_EQ( static_cast<size_t>( 1 ), presentation->placements.size() );
	const auto& placement = presentation->placements.front();
	EXPECT_EQ( static_cast<KittyImageId>( 21 ), placement.imageId );
	EXPECT_EQ( static_cast<KittyPlacementId>( 4 ), placement.placementId );
	EXPECT_EQ( 5, placement.visibleAnchorCell.x );
	EXPECT_EQ( 6, placement.visibleAnchorCell.y );
	EXPECT_EQ( static_cast<Uint32>( 2 ), placement.columns );
	EXPECT_EQ( static_cast<Uint32>( 3 ), placement.rows );

	auto put = protocol.handle( "a=p,i=21,p=5,c=4,r=2", Vector2i( 1, 2 ) );
	EXPECT_EQ( 4, put.cursorMovement.x );
	EXPECT_EQ( 2, put.cursorMovement.y );
	presentation = protocol.takePresentation();
	ASSERT_EQ( static_cast<size_t>( 2 ), presentation->placements.size() );
}

UTEST( eterm, kitty_graphics_placement_derives_missing_cell_geometry ) {
	KittyGraphicsProtocol protocol;
	protocol.setCellPixelSize( 10, 20 );
	protocol.handle( "a=t,f=32,s=2,v=2,i=22;AAAAAAAAAAAAAAAAAAAAAA==" );
	auto result = protocol.handle( "a=p,i=22,X=9,Y=19,C=1" );
	EXPECT_TRUE( result.changed );
	auto presentation = protocol.takePresentation();
	ASSERT_EQ( static_cast<size_t>( 1 ), presentation->placements.size() );
	EXPECT_EQ( static_cast<Uint32>( 2 ), presentation->placements[0].columns );
	EXPECT_EQ( static_cast<Uint32>( 2 ), presentation->placements[0].rows );
}

UTEST( eterm, kitty_graphics_retransmit_removes_old_placements_and_crop_intersects ) {
	KittyGraphicsProtocol protocol;
	protocol.handle( "a=T,f=32,s=2,v=2,i=23,p=4;AAAAAAAAAAAAAAAAAAAAAA==" );
	ASSERT_EQ( static_cast<size_t>( 1 ), protocol.takePresentation()->placements.size() );
	protocol.handle( "a=t,f=32,s=1,v=1,i=23;AQIDBA==" );
	EXPECT_TRUE( protocol.takePresentation()->placements.empty() );
	protocol.handle( "a=t,f=32,s=2,v=2,i=24;AAAAAAAAAAAAAAAAAAAAAA==" );
	auto placed = protocol.handle( "a=p,i=24,p=5,x=1,y=1,w=99,h=99" );
	EXPECT_STDSTREQ( "\033_Gi=24,p=5;OK\033\\", placed.response );
	auto presentation = protocol.takePresentation();
	ASSERT_EQ( static_cast<size_t>( 1 ), presentation->placements.size() );
	EXPECT_EQ( 2, presentation->placements[0].sourcePixels.Right );
	EXPECT_EQ( 2, presentation->placements[0].sourcePixels.Bottom );
}

UTEST( eterm, kitty_graphics_resync_republishes_authoritative_images ) {
	KittyGraphicsProtocol protocol;
	protocol.handle( "a=t,f=32,s=1,v=1,i=27;AQIDBA==" );
	protocol.takeUpdates();
	protocol.resync();
	auto updates = protocol.takeUpdates();
	ASSERT_EQ( static_cast<size_t>( 2 ), updates.size() );
	EXPECT_EQ( TerminalGraphicsUpdateType::ResetAll, updates[0].type );
	EXPECT_EQ( TerminalGraphicsUpdateType::CreateImage, updates[1].type );
	EXPECT_EQ( static_cast<KittyImageId>( 27 ), updates[1].imageId );
	ASSERT_TRUE( updates[1].pixels != nullptr );
	EXPECT_EQ( static_cast<size_t>( 4 ), updates[1].pixels->size() );
}

UTEST( eterm, kitty_graphics_root_frame_patch_publishes_only_changed_rectangle ) {
	KittyGraphicsProtocol protocol;
	protocol.handle( "a=t,f=32,s=2,v=1,i=29;AQIDBAUGBwg=" );
	protocol.takeUpdates();
	auto result = protocol.handle( "a=f,i=29,r=1,f=32,s=1,v=1,x=1,y=0,X=1;CQoLDA==" );
	EXPECT_TRUE( result.changed );
	const auto* pixels = protocol.imagePixels( 29 );
	ASSERT_TRUE( pixels != nullptr );
	const std::vector<Uint8> expected{ 1, 2, 3, 4, 9, 10, 11, 12 };
	EXPECT_TRUE( expected == *pixels );
	auto updates = protocol.takeUpdates();
	ASSERT_EQ( static_cast<size_t>( 1 ), updates.size() );
	EXPECT_EQ( TerminalGraphicsUpdateType::UpdateRegion, updates[0].type );
	EXPECT_EQ( 1, updates[0].region.Left );
	EXPECT_EQ( 0, updates[0].region.Top );
	EXPECT_EQ( 2, updates[0].region.Right );
	EXPECT_EQ( 1, updates[0].region.Bottom );
	ASSERT_TRUE( updates[0].pixels != nullptr );
	EXPECT_EQ( static_cast<size_t>( 4 ), updates[0].pixels->size() );
}

UTEST( eterm, kitty_graphics_rgb24_root_converts_only_when_mutated ) {
	KittyGraphicsProtocol protocol;
	ASSERT_EQ( KittyGraphicsError::None,
			   protocol.handle( "a=t,f=24,s=2,v=1,i=92;AQIDBAUG" ).error );
	auto updates = protocol.takeUpdates();
	ASSERT_EQ( static_cast<size_t>( 1 ), updates.size() );
	EXPECT_EQ( static_cast<Uint8>( 3 ), updates[0].channels );

	ASSERT_EQ( KittyGraphicsError::None,
			   protocol.handle( "a=f,i=92,r=1,f=32,s=1,v=1,x=1,y=0,X=1;BwgJCg==" ).error );
	const std::vector<Uint8> expected{ 1, 2, 3, 255, 7, 8, 9, 10 };
	ASSERT_TRUE( protocol.imagePixels( 92 ) != nullptr );
	EXPECT_TRUE( expected == *protocol.imagePixels( 92 ) );
	updates = protocol.takeUpdates();
	ASSERT_EQ( static_cast<size_t>( 2 ), updates.size() );
	EXPECT_EQ( TerminalGraphicsUpdateType::ReplaceImage, updates[0].type );
	EXPECT_EQ( static_cast<Uint8>( 4 ), updates[0].channels );
	EXPECT_EQ( TerminalGraphicsUpdateType::UpdateRegion, updates[1].type );
}

UTEST( eterm, kitty_graphics_animation_frame_create_control_and_compose ) {
	KittyGraphicsProtocol protocol;
	protocol.handle( "a=T,f=32,s=2,v=1,i=30;AQIDBAUGBwg=" );
	protocol.takeUpdates();
	protocol.takePresentation();
	auto frame = protocol.handle( "a=f,i=30,c=1,f=32,s=1,v=1,x=1,y=0,X=1,z=25;CQoLDA==" );
	EXPECT_EQ( KittyGraphicsError::None, frame.error );
	auto updates = protocol.takeUpdates();
	ASSERT_EQ( static_cast<size_t>( 1 ), updates.size() );
	EXPECT_EQ( TerminalGraphicsUpdateType::CreateFrame, updates[0].type );
	EXPECT_EQ( static_cast<Uint32>( 2 ), updates[0].frameNumber );
	ASSERT_TRUE( updates[0].pixels != nullptr );
	const std::vector<Uint8> expectedFrame{ 1, 2, 3, 4, 9, 10, 11, 12 };
	EXPECT_TRUE( expectedFrame == *updates[0].pixels );

	auto control = protocol.handle( "a=a,i=30,c=2" );
	EXPECT_TRUE( control.changed );
	auto presentation = protocol.takePresentation();
	ASSERT_EQ( static_cast<size_t>( 1 ), presentation->placements.size() );
	EXPECT_EQ( static_cast<Uint32>( 2 ), presentation->placements[0].frameNumber );

	auto compose = protocol.handle( "a=c,i=30,r=1,c=2,X=1,Y=0,x=0,y=0,w=1,h=1,C=1" );
	EXPECT_EQ( KittyGraphicsError::None, compose.error );
	updates = protocol.takeUpdates();
	ASSERT_EQ( static_cast<size_t>( 1 ), updates.size() );
	EXPECT_EQ( TerminalGraphicsUpdateType::UpdateFrameRegion, updates[0].type );
	ASSERT_TRUE( updates[0].pixels != nullptr );
	const std::vector<Uint8> expectedPatch{ 5, 6, 7, 8 };
	EXPECT_TRUE( expectedPatch == *updates[0].pixels );
	EXPECT_TRUE( protocol.handle( "a=a,i=30,c=1,r=1,z=-1,s=3" ).changed );
	EXPECT_TRUE( protocol.updateAnimations() );
	presentation = protocol.takePresentation();
	ASSERT_EQ( static_cast<size_t>( 1 ), presentation->placements.size() );
	EXPECT_EQ( static_cast<Uint32>( 2 ), presentation->placements[0].frameNumber );
	EXPECT_TRUE( protocol.handle( "a=d,d=f,i=30" ).changed );
	updates = protocol.takeUpdates();
	ASSERT_EQ( static_cast<size_t>( 1 ), updates.size() );
	EXPECT_EQ( TerminalGraphicsUpdateType::DeleteFrame, updates[0].type );
}

UTEST( eterm, kitty_graphics_delete_placements_and_uppercase_frees_data ) {
	KittyGraphicsProtocol protocol;
	protocol.handle( "a=T,f=32,s=1,v=1,i=31,p=1,c=2,r=2;AQIDBA==", Vector2i( 3, 4 ) );
	protocol.handle( "a=p,i=31,p=2,c=1,r=1", Vector2i( 8, 9 ) );
	protocol.takeUpdates();

	auto result = protocol.handle( "a=d,d=c", Vector2i( 4, 5 ) );
	EXPECT_TRUE( result.changed );
	auto presentation = protocol.takePresentation();
	ASSERT_EQ( static_cast<size_t>( 1 ), presentation->placements.size() );
	EXPECT_EQ( static_cast<KittyPlacementId>( 2 ), presentation->placements[0].placementId );
	EXPECT_TRUE( protocol.imagePixels( 31 ) != nullptr );

	result = protocol.handle( "a=d,d=I,i=31,p=2" );
	EXPECT_TRUE( result.changed );
	EXPECT_TRUE( protocol.imagePixels( 31 ) == nullptr );
	EXPECT_TRUE( protocol.takePresentation()->placements.empty() );
	auto updates = protocol.takeUpdates();
	ASSERT_EQ( static_cast<size_t>( 1 ), updates.size() );
	EXPECT_EQ( TerminalGraphicsUpdateType::DeleteImage, updates[0].type );
	EXPECT_EQ( static_cast<KittyImageId>( 31 ), updates[0].imageId );
}

UTEST( eterm, kitty_graphics_storage_quota_evicts_oldest_unplaced_image ) {
	KittyGraphicsProtocol protocol( 8, 2, 2 );
	EXPECT_TRUE( protocol.handle( "a=t,f=32,s=1,v=1,i=41;AQIDBA==" ).changed );
	EXPECT_TRUE( protocol.handle( "a=t,f=32,s=1,v=1,i=42,N=1;BQYHCA==" ).changed );
	protocol.takeUpdates();
	EXPECT_TRUE( protocol.handle( "a=t,f=32,s=1,v=1,i=43;CQoLDA==" ).changed );
	EXPECT_TRUE( protocol.imagePixels( 41 ) != nullptr );
	EXPECT_TRUE( protocol.imagePixels( 42 ) == nullptr );
	EXPECT_TRUE( protocol.imagePixels( 43 ) != nullptr );
	auto updates = protocol.takeUpdates();
	ASSERT_EQ( static_cast<size_t>( 2 ), updates.size() );
	EXPECT_EQ( TerminalGraphicsUpdateType::DeleteImage, updates[0].type );
	EXPECT_EQ( static_cast<KittyImageId>( 42 ), updates[0].imageId );
	EXPECT_EQ( TerminalGraphicsUpdateType::CreateImage, updates[1].type );
}

UTEST( eterm, kitty_graphics_screen_lifecycle_restores_primary_and_resets_gpu ) {
	KittyGraphicsProtocol protocol;
	protocol.handle( "a=T,f=32,s=1,v=1,i=51;AQIDBA==" );
	protocol.takeUpdates();
	protocol.takePresentation();
	protocol.setAlternateScreen( true );
	EXPECT_TRUE( protocol.takePresentation()->placements.empty() );
	protocol.handle( "a=p,i=51,p=2" );
	ASSERT_EQ( static_cast<size_t>( 1 ), protocol.takePresentation()->placements.size() );
	protocol.setAlternateScreen( false );
	auto primary = protocol.takePresentation();
	ASSERT_EQ( static_cast<size_t>( 1 ), primary->placements.size() );
	EXPECT_EQ( static_cast<KittyPlacementId>( 0 ), primary->placements[0].placementId );
	protocol.clearScreen();
	EXPECT_TRUE( protocol.takePresentation()->placements.empty() );
	EXPECT_TRUE( protocol.imagePixels( 51 ) != nullptr );
	protocol.reset();
	EXPECT_TRUE( protocol.imagePixels( 51 ) == nullptr );
	auto updates = protocol.takeUpdates();
	ASSERT_TRUE( !updates.empty() );
	EXPECT_EQ( TerminalGraphicsUpdateType::ResetAll, updates.back().type );
}

UTEST( eterm, kitty_graphics_scrolling_tracks_history_and_scrollback ) {
	KittyGraphicsProtocol protocol;
	protocol.setViewport( 0, 0, 4 );
	protocol.handle( "a=T,f=32,s=1,v=1,i=61,r=1;AQIDBA==", Vector2i( 0, 0 ) );
	protocol.takePresentation();
	protocol.scrollScreen( 0, 3, -1, true );
	protocol.setViewport( 0, 1, 4 );
	EXPECT_TRUE( protocol.takePresentation()->placements.empty() );
	protocol.setViewport( 1, 1, 4 );
	auto history = protocol.takePresentation();
	ASSERT_EQ( static_cast<size_t>( 1 ), history->placements.size() );
	EXPECT_EQ( 0, history->placements[0].visibleAnchorCell.y );
	protocol.setViewport( 0, 0, 4 );
	EXPECT_TRUE( protocol.takePresentation()->placements.empty() );
}

UTEST( eterm, kitty_graphics_margin_scroll_discards_only_scrolled_out_placements ) {
	KittyGraphicsProtocol protocol;
	protocol.setViewport( 0, 0, 6 );
	protocol.handle( "a=T,f=32,s=1,v=1,i=62,p=1;AQIDBA==", Vector2i( 0, 0 ) );
	protocol.handle( "a=p,i=62,p=2", Vector2i( 0, 2 ) );
	protocol.handle( "a=p,i=62,p=3", Vector2i( 0, 5 ) );
	protocol.scrollScreen( 1, 4, -2, false );
	auto presentation = protocol.takePresentation();
	ASSERT_EQ( static_cast<size_t>( 2 ), presentation->placements.size() );
	EXPECT_EQ( static_cast<KittyPlacementId>( 1 ), presentation->placements[0].placementId );
	EXPECT_EQ( static_cast<KittyPlacementId>( 3 ), presentation->placements[1].placementId );
}

UTEST( eterm, kitty_graphics_margin_scroll_clips_partially_visible_placements ) {
	KittyGraphicsProtocol protocol;
	protocol.setViewport( 0, 0, 6 );
	protocol.handle( "a=T,f=32,s=1,v=4,i=63,p=1,c=1,r=4;AAAAAAAAAAAAAAAAAAAAAA==",
					 Vector2i( 0, 1 ) );
	protocol.scrollScreen( 1, 4, -2, false );
	auto presentation = protocol.takePresentation();
	ASSERT_EQ( static_cast<size_t>( 1 ), presentation->placements.size() );
	EXPECT_EQ( 1, presentation->placements[0].visibleAnchorCell.y );
	EXPECT_EQ( static_cast<Uint32>( 2 ), presentation->placements[0].rows );
	EXPECT_EQ( 2, presentation->placements[0].sourcePixels.Top );
	EXPECT_EQ( 4, presentation->placements[0].sourcePixels.Bottom );
}

UTEST( eterm, kitty_graphics_virtual_and_relative_placements_follow_protocol_rules ) {
	KittyGraphicsProtocol protocol;
	protocol.handle( "a=t,f=32,s=1,v=1,i=71;AQIDBA==" );
	auto virtualPlacement = protocol.handle( "a=p,i=71,p=10,U=1,c=2,r=2", Vector2i( 2, 3 ) );
	EXPECT_TRUE( virtualPlacement.changed );
	EXPECT_EQ( 0, virtualPlacement.cursorMovement.x );
	EXPECT_TRUE( protocol.takePresentation()->placements.empty() );
	auto relative = protocol.handle( "a=p,i=71,p=11,P=71,Q=10,H=4,V=-1,c=1,r=1" );
	EXPECT_TRUE( relative.changed );
	EXPECT_EQ( 0, relative.cursorMovement.x );
	protocol.setPlaceholderCells( { { 71, 10, Vector2i( 2, 3 ), 0, 0 } } );
	auto presentation = protocol.takePresentation();
	ASSERT_EQ( static_cast<size_t>( 2 ), presentation->placements.size() );
	auto child = std::find_if(
		presentation->placements.begin(), presentation->placements.end(),
		[]( const TerminalVisiblePlacement& placement ) { return placement.placementId == 11; } );
	ASSERT_TRUE( child != presentation->placements.end() );
	EXPECT_EQ( 6, child->visibleAnchorCell.x );
	EXPECT_EQ( 2, child->visibleAnchorCell.y );
	protocol.handle( "a=d,d=a" );
	ASSERT_EQ( static_cast<size_t>( 1 ), protocol.takePresentation()->placements.size() );
	auto replacement = protocol.handle( "a=p,i=71,p=11,P=71,Q=10,H=1,V=1" );
	EXPECT_TRUE( replacement.changed );
}

UTEST( eterm, kitty_graphics_relative_placements_report_missing_parents_and_cycles ) {
	KittyGraphicsProtocol protocol;
	protocol.handle( "a=t,f=32,s=1,v=1,i=72;AQIDBA==" );
	EXPECT_EQ( KittyGraphicsError::NoParent, protocol.handle( "a=p,i=72,p=2,P=72,Q=99" ).error );
	EXPECT_TRUE( protocol.handle( "a=p,i=72,p=1", Vector2i( 1, 1 ) ).changed );
	EXPECT_TRUE( protocol.handle( "a=p,i=72,p=2,P=72,Q=1,H=1,V=0" ).changed );
	EXPECT_EQ( KittyGraphicsError::Cycle,
			   protocol.handle( "a=p,i=72,p=1,P=72,Q=2,H=1,V=0" ).error );
}

UTEST( eterm, kitty_graphics_independent_client_namespaces_coexist ) {
	KittyGraphicsProtocol protocol;
	auto first = protocol.handle( "a=T,f=32,s=1,v=1,I=101,p=1;AQIDBA==" );
	auto second = protocol.handle( "a=T,f=32,s=1,v=1,I=202,p=1;BQYHCA==", Vector2i( 2, 0 ) );
	EXPECT_EQ( KittyGraphicsError::None, first.error );
	EXPECT_EQ( KittyGraphicsError::None, second.error );
	ASSERT_EQ( static_cast<size_t>( 2 ), protocol.imageCount() );
	auto presentation = protocol.takePresentation();
	ASSERT_EQ( static_cast<size_t>( 2 ), presentation->placements.size() );
	EXPECT_NE( presentation->placements[0].imageId, presentation->placements[1].imageId );
	EXPECT_TRUE( protocol.handle( "a=d,d=n,I=101" ).changed );
	ASSERT_EQ( static_cast<size_t>( 1 ), protocol.takePresentation()->placements.size() );
	EXPECT_EQ( static_cast<size_t>( 2 ), protocol.imageCount() );
}

UTEST( eterm_session, kitty_graphics_update_and_metadata_cross_worker_boundary ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033_Ga=t,f=32,s=1,v=1,i=13;AQIDBA==\033\\";
	pty->mLoopWrites = false;
	MockPty* ptyPtr = pty.get();
	auto process = std::make_unique<MockProcess>();
	auto session = TerminalSession::create( std::move( pty ), std::move( process ), 100 );
	auto snapshot = waitForSnapshot( session, []( const TerminalSnapshot& value ) {
		return value.graphics && value.graphics->requiredUpdateSequence > 0;
	} );
	ASSERT_TRUE( snapshot != nullptr );
	EXPECT_EQ( static_cast<Uint64>( 1 ), snapshot->graphics->requiredUpdateSequence );
	auto updates = session->drainGraphicsUpdates();
	ASSERT_EQ( static_cast<size_t>( 1 ), updates.size() );
	EXPECT_EQ( static_cast<Uint64>( 1 ), updates[0].sequence );
	EXPECT_EQ( static_cast<KittyImageId>( 13 ), updates[0].imageId );
	EXPECT_STDSTREQ( "\033_Gi=13;OK\033\\", ptyPtr->mWrites );
}

UTEST( eterm, kitty_keyboard_state_stack_and_modes ) {
	KittyKeyboardState state;
	state.push( 1 );
	EXPECT_EQ( static_cast<Uint32>( 1 ), state.flags );
	state.push( 7 );
	EXPECT_EQ( static_cast<Uint32>( 7 ), state.flags );
	state.pop();
	EXPECT_EQ( static_cast<Uint32>( 1 ), state.flags );
	state.set( 8, 2 );
	EXPECT_EQ( static_cast<Uint32>( 9 ), state.flags );
	state.set( 1, 3 );
	EXPECT_EQ( static_cast<Uint32>( 8 ), state.flags );
	state.set( 8, 1 );
	EXPECT_EQ( static_cast<Uint32>( 8 ), state.flags );
	state.set( 1, 99 );
	EXPECT_EQ( static_cast<Uint32>( 8 ), state.flags );
	state.pop( 1000000 );
	EXPECT_EQ( static_cast<Uint32>( 0 ), state.flags );
	EXPECT_TRUE( state.stack.empty() );
}

UTEST( eterm, kitty_keyboard_encoder_modifiers_and_enter ) {
	EXPECT_EQ( static_cast<Uint32>( 1 ), KittyKeyboardEncoder::encodeModifiers( KEYMOD_NONE ) );
	EXPECT_EQ( static_cast<Uint32>( 2 ), KittyKeyboardEncoder::encodeModifiers( KEYMOD_SHIFT ) );
	EXPECT_EQ( static_cast<Uint32>( 3 ), KittyKeyboardEncoder::encodeModifiers( KEYMOD_ALT ) );
	EXPECT_EQ( static_cast<Uint32>( 5 ), KittyKeyboardEncoder::encodeModifiers( KEYMOD_CTRL ) );
	EXPECT_EQ( static_cast<Uint32>( 6 ),
			   KittyKeyboardEncoder::encodeModifiers( KEYMOD_CTRL | KEYMOD_SHIFT ) );

	KittyKeyEvent enter{ KEY_RETURN, SCANCODE_RETURN, '\r', KEYMOD_CTRL, KittyKeyEventType::Press };
	EXPECT_FALSE( KittyKeyboardEncoder::encode( enter, 1 ).handled );
	EXPECT_STDSTREQ( "\033[13;5u", KittyKeyboardEncoder::encode( enter, 8 ).bytes );
	EXPECT_STDSTREQ( "\033[13;5:1u", KittyKeyboardEncoder::encode( enter, 10 ).bytes );
	enter.type = KittyKeyEventType::Repeat;
	EXPECT_STDSTREQ( "\033[13;5:2u", KittyKeyboardEncoder::encode( enter, 10 ).bytes );
	enter.type = KittyKeyEventType::Release;
	EXPECT_STDSTREQ( "\033[13;5:3u", KittyKeyboardEncoder::encode( enter, 10 ).bytes );
	EXPECT_STDSTREQ( "\033[97;5u",
					 KittyKeyboardEncoder::encode(
						 { KEY_A, SCANCODE_A, 0, KEYMOD_CTRL, KittyKeyEventType::Press }, 1 )
						 .bytes );
	EXPECT_FALSE( KittyKeyboardEncoder::encode(
					  { KEY_A, SCANCODE_A, 0, KEYMOD_SHIFT, KittyKeyEventType::Press }, 1 )
					  .handled );
	EXPECT_FALSE( KittyKeyboardEncoder::encode(
					  { KEY_1, SCANCODE_1, 0, KEYMOD_SHIFT, KittyKeyEventType::Press }, 7 )
					  .handled );
	EXPECT_STDSTREQ( "\033[97:65;2u",
					 KittyKeyboardEncoder::encode(
						 { KEY_A, SCANCODE_A, 'A', KEYMOD_SHIFT, KittyKeyEventType::Press }, 12 )
						 .bytes );
	EXPECT_STDSTREQ( "\033[13;1:1~",
					 KittyKeyboardEncoder::encode(
						 { KEY_F3, SCANCODE_F3, 0, KEYMOD_NONE, KittyKeyEventType::Press }, 31 )
						 .bytes );
	EXPECT_STDSTREQ(
		"\033[57414;1:1u",
		KittyKeyboardEncoder::encode(
			{ KEY_KP_ENTER, SCANCODE_KP_ENTER, 0, KEYMOD_NONE, KittyKeyEventType::Press }, 31 )
			.bytes );
	EXPECT_STDSTREQ(
		"\033[57442;5:1u",
		KittyKeyboardEncoder::encode(
			{ KEY_LCTRL, SCANCODE_LCTRL, 0, KEYMOD_CTRL, KittyKeyEventType::Press }, 31 )
			.bytes );
}

UTEST( eterm, kitty_keyboard_protocol_keeps_screen_state_independent ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033[=1u\033[?1049h\033[?u\033[=3u\033[?1049l\033[?u";
	pty->mLoopWrites = false;
	MockPty* ptyPtr = pty.get();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );
	term->update();
	EXPECT_STDSTREQ( "\033[?0u\033[?1u", ptyPtr->mWrites );
}

UTEST( eterm, kitty_keyboard_protocol_negotiates_and_reports_active_state ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033[?u\033[>7u\033[?u\033[<u\033[?u";
	pty->mLoopWrites = false;
	MockPty* ptyPtr = pty.get();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	term->update();

	EXPECT_STDSTREQ( "\033[?0u\033[?7u\033[?0u", ptyPtr->mWrites );
}

UTEST( eterm, kitty_keyboard_protocol_encodes_worker_key_without_duplicate_text ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033[>8u";
	pty->mLoopWrites = false;
	MockPty* ptyPtr = pty.get();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );
	term->update();
	term->keyEvent( { KEY_A, SCANCODE_A, 0, KEYMOD_NONE, KittyKeyEventType::Press } );
	term->textInput( 'a' );
	term->keyEvent( { KEY_RETURN, SCANCODE_RETURN, '\r', KEYMOD_CTRL, KittyKeyEventType::Press } );
	EXPECT_STDSTREQ( "\033[97;1u\033[13;5u", ptyPtr->mWrites );
}

UTEST( eterm, kitty_modifier_key_does_not_scroll_to_bottom ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033[>8u";
	MockPty* ptyPtr = pty.get();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );
	term->update();

	for ( int i = 0; i < 40; ++i ) {
		std::string line = "Line " + std::to_string( i ) + "\r\n";
		term->write( line.c_str(), line.size() );
		term->update();
	}
	ptyPtr->mLoopWrites = false;

	TerminalArg scroll( 5 );
	term->kscrollup( &scroll );
	ptyPtr->mWrites.clear();

	term->keyEvent( { KEY_LCTRL, SCANCODE_LCTRL, 0, KEYMOD_LCTRL, KittyKeyEventType::Press } );
	EXPECT_FALSE( ptyPtr->mWrites.empty() );
	EXPECT_EQ( 5, term->scrollPos() );

	term->keyEvent( { KEY_A, SCANCODE_A, 0, KEYMOD_LCTRL, KittyKeyEventType::Press } );
	EXPECT_EQ( 0, term->scrollPos() );
}

UTEST( eterm, tab_and_backtab_preserve_physical_modifiers ) {
	const Uint32 modifierCases[] = { KEYMOD_NONE, KEYMOD_LSHIFT, KEYMOD_RSHIFT, KEYMOD_SHIFT };
	for ( Uint32 flags : { 0u, 1u, 3u, 7u, 8u, 11u } ) {
		for ( Uint32 modifiers : modifierCases ) {
			auto pty = std::make_unique<MockPty>();
			pty->mBuffer = "\033[>" + std::to_string( flags ) + "u";
			pty->mLoopWrites = false;
			MockPty* ptyPtr = pty.get();
			auto term = TerminalEmulator::create( std::move( pty ), std::make_unique<MockProcess>(),
												  std::make_shared<MockDisplay>(), 100 );
			term->update();

			const bool shifted = modifiers & KEYMOD_SHIFT;
			const bool reportAll = flags & 8;
			const bool reportEvents = reportAll && ( flags & 2 );
			const std::string prefix = shifted ? "\033[9;2" : "\033[9;1";
			const std::string press = reportAll ? prefix + ( reportEvents ? ":1u" : "u" )
									  : shifted ? "\033[Z"
												: "\t";
			const std::string release = reportEvents ? prefix + ":3u" : "";
			KittyKeyEvent event{ KEY_TAB, SCANCODE_TAB, 0, modifiers, KittyKeyEventType::Press };
			term->keyEvent( event );
			EXPECT_STDSTREQ( press, ptyPtr->mWrites );
			event.type = KittyKeyEventType::Release;
			term->keyEvent( event );
			EXPECT_STDSTREQ( press + release, ptyPtr->mWrites );
			// Lock bits must not prevent a legacy keymap match.
			if ( !reportAll ) {
				ptyPtr->mWrites.clear();
				event.type = KittyKeyEventType::Repeat;
				term->keyEvent( event );
				EXPECT_STDSTREQ( press, ptyPtr->mWrites );
				ptyPtr->mWrites.clear();
				event.type = KittyKeyEventType::Press;
				event.modifiers |= KEYMOD_CAPS | KEYMOD_NUM;
				term->keyEvent( event );
				EXPECT_STDSTREQ( press, ptyPtr->mWrites );
			}
		}
	}
}

UTEST( eterm, legacy_keymap_normalizes_sides_and_preserves_altgr ) {
	auto pty = std::make_unique<MockPty>();
	pty->mLoopWrites = false;
	MockPty* ptyPtr = pty.get();
	auto term = TerminalEmulator::create( std::move( pty ), std::make_unique<MockProcess>(),
										  std::make_shared<MockDisplay>(), 100 );
	const struct {
		Keycode key;
		Scancode scancode;
		Uint32 modifiers;
		const char* bytes;
	} cases[] = {
		{ KEY_UP, SCANCODE_UP, KEYMOD_LSHIFT, "\033[1;2A" },
		{ KEY_UP, SCANCODE_UP, KEYMOD_RSHIFT, "\033[1;2A" },
		{ KEY_UP, SCANCODE_UP, KEYMOD_LCTRL, "\033[1;5A" },
		{ KEY_UP, SCANCODE_UP, KEYMOD_RCTRL, "\033[1;5A" },
		{ KEY_UP, SCANCODE_UP, KEYMOD_RCTRL | KEYMOD_RSHIFT | KEYMOD_LALT, "\033[1;8A" },
		{ KEY_F1, SCANCODE_F1, KEYMOD_LMETA, "\033[1;6P" },
		{ KEY_F1, SCANCODE_F1, KEYMOD_RMETA, "\033[1;6P" },
		{ KEY_F1, SCANCODE_F1, KEYMOD_LALT, "\033[1;3P" },
		{ KEY_F1, SCANCODE_F1, KEYMOD_RALT, "\033[1;4P" },
	};
	for ( const auto& entry : cases ) {
		ptyPtr->mWrites.clear();
		term->keyEvent(
			{ entry.key, entry.scancode, 0, entry.modifiers, KittyKeyEventType::Press } );
		EXPECT_STDSTREQ( entry.bytes, ptyPtr->mWrites );
	}
}

UTEST( eterm, application_cursor_keys_are_written_to_pty ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033[?1h";
	pty->mLoopWrites = false;
	MockPty* ptyPtr = pty.get();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );
	term->update();

	term->keyEvent( { KEY_UP, SCANCODE_UP, 0, KEYMOD_NONE, KittyKeyEventType::Press } );
	term->keyEvent( { KEY_DOWN, SCANCODE_DOWN, 0, KEYMOD_NONE, KittyKeyEventType::Press } );

	EXPECT_STDSTREQ( "\033OA\033OB", ptyPtr->mWrites );
}

UTEST( eterm, kitty_keyboard_protocol_preserves_altgr_text ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033[>15u";
	pty->mLoopWrites = false;
	MockPty* ptyPtr = pty.get();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );
	term->update();

	// Spanish AltGr+2 produces '@'. Windows may include a synthetic Ctrl modifier for AltGr.
	term->keyEvent(
		{ KEY_2, SCANCODE_2, '@', KEYMOD_RALT | KEYMOD_LCTRL, KittyKeyEventType::Press } );
	term->textInput( '@' );

	EXPECT_STDSTREQ( "\033[64::50;1:1u", ptyPtr->mWrites );
	EXPECT_FALSE( KittyKeyboardEncoder::encode(
					  { KEY_2, SCANCODE_2, 0, KEYMOD_RALT, KittyKeyEventType::Press }, 7 )
					  .handled );
}

UTEST( eterm, kitty_graphics_unicode_placeholder_uses_color_and_diacritics ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033_Ga=t,f=32,s=2,v=2,i=72,q=2;AAAAAAAAAAAAAAAAAAAAAA==\033\\"
				   "\033_Ga=p,i=72,p=3,U=1,c=2,r=2,q=2\033\\"
				   "\033[38;5;72m\033[58;5;3m\xF4\x8E\xBB\xAE\xCC\x85\xCC\x85";
	pty->mLoopWrites = false;
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );
	term->update();
	ASSERT_TRUE( display->mGraphics != nullptr );
	ASSERT_EQ( static_cast<size_t>( 1 ), display->mGraphics->placements.size() );
	const auto& placement = display->mGraphics->placements[0];
	EXPECT_EQ( static_cast<KittyImageId>( 72 ), placement.imageId );
	EXPECT_EQ( static_cast<KittyPlacementId>( 3 ), placement.placementId );
	EXPECT_EQ( 0, placement.visibleAnchorCell.x );
	EXPECT_EQ( 0, placement.visibleAnchorCell.y );
	EXPECT_EQ( 0, placement.sourcePixels.Left );
	EXPECT_EQ( 0, placement.sourcePixels.Top );
	EXPECT_EQ( 1, placement.sourcePixels.Right );
	EXPECT_EQ( 1, placement.sourcePixels.Bottom );
	term->resize( 100, 30, 1000, 600 );
	ASSERT_TRUE( display->mGraphics != nullptr );
	ASSERT_EQ( static_cast<size_t>( 1 ), display->mGraphics->placements.size() );
	EXPECT_EQ( static_cast<KittyImageId>( 72 ), display->mGraphics->placements[0].imageId );
}

UTEST( eterm, printable_runs_replace_kitty_placeholders ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033_Ga=t,f=32,s=2,v=2,i=72,q=2;AAAAAAAAAAAAAAAAAAAAAA==\033\\"
				   "\033_Ga=p,i=72,p=3,U=1,c=2,r=2,q=2\033\\"
				   "\033[38;5;72m\033[58;5;3m\xF4\x8E\xBB\xAE\xCC\x85\xCC\x85";
	pty->mLoopWrites = false;
	MockPty* input = pty.get();
	auto display = std::make_shared<MockDisplay>();
	auto term =
		TerminalEmulator::create( std::move( pty ), std::make_unique<MockProcess>(), display, 100 );
	term->update();
	ASSERT_TRUE( display->mGraphics != nullptr );
	ASSERT_EQ( static_cast<size_t>( 1 ), display->mGraphics->placements.size() );

	input->mBuffer += "\033[m\rXY";
	term->update();
	ASSERT_TRUE( display->mGraphics != nullptr );
	EXPECT_EQ( static_cast<size_t>( 0 ), display->mGraphics->placements.size() );
	EXPECT_EQ( static_cast<Rune>( 'X' ), display->mFirstGlyph.u );
	EXPECT_EQ( static_cast<Rune>( 'Y' ), display->mSecondGlyph.u );
}

UTEST( eterm, kitty_graphics_apc_is_fragmentation_safe_and_not_terminal_text ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033_Ga=q,i=31,f=32,s=1,v=1;AAAAAA==\033\\OK";
	pty->mLoopWrites = false;
	pty->mMaxRead = 1;
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	term->update();
	while ( !term->update() ) {
	}

	EXPECT_EQ( static_cast<Rune>( 'O' ), display->mFirstGlyph.u );
	EXPECT_EQ( static_cast<Rune>( 'K' ), display->mSecondGlyph.u );
}

UTEST( eterm, kitty_graphics_bulk_apc_accepts_every_input_split_boundary ) {
	const std::string stream = "\033_Ga=T,f=32,s=1,v=1,q=2;AQIDBA==\033\\";
	for ( size_t split = 1; split < stream.size(); ++split ) {
		auto pty = std::make_unique<MockPty>();
		pty->mBuffer = stream;
		pty->mLoopWrites = false;
		pty->mReadSizes = { split, stream.size() - split };
		auto process = std::make_unique<MockProcess>();
		auto display = std::make_shared<MockDisplay>();
		auto term =
			TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );
		term->update();
		while ( !term->update() ) {
		}
		ASSERT_TRUE( display->mGraphics != nullptr );
		ASSERT_EQ( static_cast<size_t>( 1 ), display->mGraphics->placements.size() );
	}
}

UTEST( eterm, kitty_graphics_strict_base64_rejects_invalid_payload_bytes ) {
	KittyGraphicsProtocol protocol;
	EXPECT_EQ( KittyGraphicsError::InvalidData,
			   protocol.handle( "a=t,f=32,s=1,v=1;AQI BA==" ).error );
	EXPECT_EQ( KittyGraphicsError::InvalidData,
			   protocol.handle( "a=t,f=32,s=1,v=1;AQIDBA=$" ).error );
	EXPECT_EQ( KittyGraphicsError::InvalidData,
			   protocol.handle( "a=t,f=32,s=1,v=1;AQ=DBA==" ).error );
	EXPECT_EQ( KittyGraphicsError::InvalidData, protocol.handle( "a=t,f=32,s=1,v=1;AB==" ).error );
	EXPECT_EQ( KittyGraphicsError::InvalidData,
			   protocol.handle( "a=t,f=32,s=1,v=1,m=1;AQ==" ).error );
	EXPECT_EQ( KittyGraphicsError::InvalidData,
			   protocol.handle( "a=t,f=32,s=1,v=1,m=1;AQI!" ).error );
}

UTEST( eterm, kitty_graphics_strict_base64_validates_final_quantum ) {
	auto decode = []( std::string_view encoded ) {
		std::vector<Uint8> output( Base64::decodeSafeOutLen( encoded.size() ) );
		return Base64::decode( encoded.size(), encoded.data(), output.size(), output.data(),
							   Base64::DecodeMode::NoWhitespaceStrict );
	};
	EXPECT_EQ( static_cast<size_t>( 1 ), decode( "AA" ) );
	EXPECT_EQ( static_cast<size_t>( 2 ), decode( "AAA" ) );
	EXPECT_EQ( static_cast<size_t>( 1 ), decode( "AA==" ) );
	EXPECT_EQ( static_cast<size_t>( 2 ), decode( "AAA=" ) );
	for ( std::string_view invalid :
		  { "A", "AB", "AAB", "AB==", "AAB=", "====", "A===", "AA=A", "AAAA=", "AAAA====" } )
		EXPECT_EQ( static_cast<size_t>( -1 ), decode( invalid ) );
}

UTEST( eterm, kitty_graphics_strict_base64_decodes_boundaries_and_large_payloads ) {
	for ( size_t length = 1; length <= 257; ++length ) {
		std::vector<Uint8> boundarySource( length );
		for ( size_t i = 0; i < boundarySource.size(); ++i )
			boundarySource[i] = static_cast<Uint8>( ( i * 197 + length ) & 0xFF );
		std::string boundaryEncoded;
		ASSERT_TRUE( Base64::encode(
			std::string_view( reinterpret_cast<const char*>( boundarySource.data() ),
							  boundarySource.size() ),
			boundaryEncoded ) );
		std::vector<Uint8> boundaryDecoded( Base64::decodeSafeOutLen( boundaryEncoded.size() ) );
		const size_t boundaryDecodedSize =
			Base64::decode( boundaryEncoded.size(), boundaryEncoded.data(), boundaryDecoded.size(),
							boundaryDecoded.data(), Base64::DecodeMode::NoWhitespaceStrict );
		ASSERT_EQ( boundarySource.size(), boundaryDecodedSize );
		boundaryDecoded.resize( boundaryDecodedSize );
		EXPECT_TRUE( boundarySource == boundaryDecoded );
	}

	std::vector<Uint8> source( 65537 );
	for ( size_t i = 0; i < source.size(); ++i )
		source[i] = static_cast<Uint8>( ( i * 131 + i / 7 ) & 0xFF );
	std::string encoded;
	ASSERT_TRUE( Base64::encode(
		std::string_view( reinterpret_cast<const char*>( source.data() ), source.size() ),
		encoded ) );
	std::vector<Uint8> decoded( Base64::decodeSafeOutLen( encoded.size() ) );
	const size_t decodedSize =
		Base64::decode( encoded.size(), encoded.data(), decoded.size(), decoded.data(),
						Base64::DecodeMode::NoWhitespaceStrict );
	ASSERT_EQ( source.size(), decodedSize );
	decoded.resize( decodedSize );
	EXPECT_TRUE( source == decoded );

	encoded[encoded.size() / 2] = '!';
	EXPECT_EQ( static_cast<size_t>( -1 ),
			   Base64::decode( encoded.size(), encoded.data(), decoded.size(), decoded.data(),
							   Base64::DecodeMode::NoWhitespaceStrict ) );
}

UTEST( eterm, kitty_graphics_reuses_unreferenced_replacement_pixel_storage ) {
	KittyGraphicsProtocol protocol;
	ASSERT_EQ( KittyGraphicsError::None,
			   protocol.handle( "a=t,f=24,s=2,v=1,i=91,q=2;AQIDBAUG" ).error );
	auto updates = protocol.takeUpdates();
	ASSERT_EQ( static_cast<size_t>( 1 ), updates.size() );
	updates.clear();
	const auto* storage = protocol.imagePixels( 91 );
	ASSERT_TRUE( storage != nullptr );
	const auto* firstAllocation = storage->data();

	ASSERT_EQ( KittyGraphicsError::None,
			   protocol.handle( "a=t,f=24,s=2,v=1,i=91,q=2;BwgJCgsM" ).error );
	EXPECT_TRUE( storage == protocol.imagePixels( 91 ) );
	const std::vector<Uint8> expected{ 7, 8, 9, 10, 11, 12 };
	EXPECT_TRUE( expected == *protocol.imagePixels( 91 ) );
	ASSERT_TRUE( firstAllocation != protocol.imagePixels( 91 )->data() );
	updates = protocol.takeUpdates();
	ASSERT_EQ( static_cast<size_t>( 1 ), updates.size() );
	updates.clear();

	ASSERT_EQ( KittyGraphicsError::None,
			   protocol.handle( "a=t,f=24,s=2,v=1,i=91,q=2;DQ4PEBES" ).error );
	EXPECT_TRUE( storage == protocol.imagePixels( 91 ) );
	const std::vector<Uint8> recycledExpected{ 13, 14, 15, 16, 17, 18 };
	EXPECT_TRUE( recycledExpected == *protocol.imagePixels( 91 ) );
	EXPECT_TRUE( firstAllocation == protocol.imagePixels( 91 )->data() );
}

UTEST( eterm, kitty_graphics_rejects_shared_memory_transmission ) {
	KittyGraphicsProtocol protocol;
	EXPECT_EQ( KittyGraphicsError::Unsupported,
			   protocol.handle( "a=T,t=s,f=24,s=1,v=1,q=2;L2VlcHAtc2ht" ).error );
}

UTEST( eterm, kitty_graphics_accepts_unchunked_direct_image_larger_than_eight_kibibytes ) {
	std::vector<Uint8> rgb( 64 * 64 * 3 );
	for ( size_t offset = 0; offset < rgb.size(); offset += 3 )
		rgb[offset] = 255;
	std::string encoded;
	ASSERT_TRUE( Base64::encode(
		std::string_view( reinterpret_cast<const char*>( rgb.data() ), rgb.size() ), encoded ) );
	const std::string command = "a=T,f=24,s=64,v=64,c=10,r=5,C=1;" + encoded;
	ASSERT_TRUE( KittyGraphicsProtocol::parse( command ).command.has_value() );
	KittyGraphicsProtocol directProtocol;
	EXPECT_EQ( KittyGraphicsError::None, directProtocol.handle( command ).error );

	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033_G" + command + "\033\\";
	pty->mLoopWrites = false;
	MockPty* ptyPtr = pty.get();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	term->update();
	while ( !term->update() ) {
	}

	EXPECT_TRUE( ptyPtr->mWrites.empty() );
	ASSERT_TRUE( display->mGraphics != nullptr );
	ASSERT_EQ( static_cast<size_t>( 1 ), display->mGraphics->placements.size() );
	EXPECT_EQ( static_cast<Uint32>( 10 ), display->mGraphics->placements[0].columns );
	EXPECT_EQ( static_cast<Uint32>( 5 ), display->mGraphics->placements[0].rows );
}

UTEST( eterm, kitty_graphics_anonymous_video_frames_replace_at_same_anchor ) {
	KittyGraphicsProtocol protocol( 8, 8, 8 );
	for ( int frame = 0; frame < 20; ++frame ) {
		const char* pixels = frame % 2 == 0 ? "AQIDBA==" : "BQYHCA==";
		auto result =
			protocol.handle( std::string( "a=T,f=32,s=1,v=1,q=2;" ) + pixels, Vector2i( 3, 4 ) );
		EXPECT_EQ( KittyGraphicsError::None, result.error );
		EXPECT_TRUE( result.changed );
		EXPECT_EQ( static_cast<size_t>( 1 ), protocol.imageCount() );
		ASSERT_EQ( static_cast<size_t>( 1 ), protocol.takePresentation()->placements.size() );
	}
	const auto* pixels = protocol.imagePixels( 1 );
	ASSERT_TRUE( pixels != nullptr );
	EXPECT_EQ( static_cast<Uint8>( 5 ), ( *pixels )[0] );
}

UTEST( eterm, oversized_kitty_graphics_apc_is_discarded_until_terminator ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033_Ga=t;" + std::string( MAX_KITTY_GRAPHICS_APC_SIZE, 'A' ) + "\033\\OK";
	pty->mLoopWrites = false;
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	term->update();
	while ( !term->update() ) {
	}

	EXPECT_EQ( static_cast<Rune>( 'O' ), display->mFirstGlyph.u );
	EXPECT_EQ( static_cast<Rune>( 'K' ), display->mSecondGlyph.u );
}

UTEST( eterm, string_sequence_bodies_survive_fragmented_reads_and_terminators ) {
	std::string clipboard( 150000, '\0' );
	for ( size_t i = 0; i < clipboard.size(); ++i )
		clipboard[i] = static_cast<char>( 'a' + i % 26 );
	std::string encoded;
	ASSERT_TRUE( Base64::encode( clipboard, encoded ) );
	// The title ends with U+009C (ST), a C1 control that only exists after UTF-8 decoding;
	// missing it would swallow the trailing text into the title.
	const std::string prefix = "\033]52;c;" + encoded + "\033";
	const std::string stream = prefix + "\\\033]2;h\xC3\xA9llo\xC2\x9COK";

	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = stream;
	pty->mLoopWrites = false;
	// Irregular reads, with one boundary between the ESC and the backslash of ST.
	for ( size_t offset = 0; offset < prefix.size(); offset += 997 )
		pty->mReadSizes.emplace_back( std::min<size_t>( 997, prefix.size() - offset ) );
	for ( size_t offset = prefix.size(); offset < stream.size(); offset += 3 )
		pty->mReadSizes.emplace_back( 3 );
	MockPty* input = pty.get();
	auto display = std::make_shared<MockDisplay>();
	auto term =
		TerminalEmulator::create( std::move( pty ), std::make_unique<MockProcess>(), display );
	while ( input->mReadOffset < input->mBuffer.size() )
		term->update();
	while ( !term->update() ) {
	}

	ASSERT_EQ( static_cast<size_t>( 1 ), display->mClipboardWrites.size() );
	EXPECT_TRUE( clipboard == display->mClipboardWrites[0] );
	EXPECT_STDSTREQ( "h\xC3\xA9llo", display->mTitle );
	EXPECT_EQ( static_cast<Rune>( 'O' ), display->mFirstGlyph.u );
	EXPECT_EQ( static_cast<Rune>( 'K' ), display->mSecondGlyph.u );
}

UTEST( eterm, oversized_osc_hyperlink_clears_the_active_link ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033]8;;https://example.com/\aX\033]8;;https://example.com/" +
				   std::string( MAX_GENERIC_STRING_SEQUENCE_SIZE, 'x' ) + "\aY";
	pty->mLoopWrites = false;
	MockPty* input = pty.get();
	auto display = std::make_shared<MockDisplay>();
	auto term =
		TerminalEmulator::create( std::move( pty ), std::make_unique<MockProcess>(), display );
	while ( input->mReadOffset < input->mBuffer.size() )
		term->update();
	while ( !term->update() ) {
	}

	EXPECT_EQ( static_cast<Rune>( 'X' ), display->mFirstGlyph.u );
	EXPECT_NE( 0, glyphLinkId( display->mFirstGlyph ) );
	EXPECT_EQ( static_cast<Rune>( 'Y' ), display->mSecondGlyph.u );
	EXPECT_EQ( 0, glyphLinkId( display->mSecondGlyph ) );
}

UTEST( eterm, cursor_style_and_xterm_version_queries ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033[0 q\033[6 q\033[>0q";
	pty->mLoopWrites = false;
	MockPty* ptyPtr = pty.get();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	term->update();

	EXPECT_EQ( TerminalCursorMode::SteadyBar, display->getCursorMode() );
	EXPECT_EQ( static_cast<size_t>( 0 ), ptyPtr->mWrites.find( "\033P>|eterm " ) );
	ASSERT_TRUE( ptyPtr->mWrites.size() >= 2 );
	EXPECT_STDSTREQ( "\033\\", ptyPtr->mWrites.substr( ptyPtr->mWrites.size() - 2 ) );
}

UTEST( eterm, pixel_geometry_queries_use_latest_worker_resize ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033[14t\033[16t";
	pty->mLoopWrites = false;
	MockPty* ptyPtr = pty.get();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );
	term->resize( 40, 20, 400, 300 );

	term->update();

	EXPECT_TRUE( ptyPtr->mWrites.find( "\033[4;300;400t" ) != std::string::npos );
	EXPECT_TRUE( ptyPtr->mWrites.find( "\033[6;15;10t" ) != std::string::npos );
}

UTEST( eterm, sgr_pixel_mouse_mode_uses_grid_relative_pixels ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033[?1000h\033[?1016h";
	pty->mLoopWrites = false;
	MockPty* ptyPtr = pty.get();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );
	term->update();
	term->mousereport( TerminalMouseEventType::MouseButtonDown, { 2, 3 }, { 20, 30 },
					   EE_BUTTON_LMASK, 0 );
	term->mousereport( TerminalMouseEventType::MouseButtonDown, { 2, 3 }, { 20, 30 },
					   EE_BUTTON_RMASK, 0 );
	term->mousereport( TerminalMouseEventType::MouseButtonRelease, { 2, 3 }, { 20, 30 },
					   EE_BUTTON_RMASK, 0 );

	EXPECT_STDSTREQ( "\033[<0;21;31M\033[<2;21;31M\033[<2;21;31m", ptyPtr->mWrites );
}

UTEST( eterm, cursor_style_zero_uses_blinking_configured_shape ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033[2 q\033[0 q";
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );
	term->setDefaultCursorMode( TerminalCursorMode::SteadyUnderline );

	term->update();

	EXPECT_EQ( TerminalCursorMode::BlinkUnderline, display->getCursorMode() );
}

UTEST( eterm, osc_hyperlink_markers_are_recognized ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033]8;id=codex;https://example.com/\033\\OK\033]8;;\033\\";
	pty->mLoopWrites = false;
	MockPty* ptyPtr = pty.get();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	term->update();

	EXPECT_EQ( static_cast<Rune>( 'O' ), display->mFirstGlyph.u );
	EXPECT_EQ( static_cast<Rune>( 'K' ), display->mSecondGlyph.u );
	EXPECT_TRUE( ptyPtr->mWrites.empty() );
	const auto id = glyphLinkId( display->mFirstGlyph );
	ASSERT_NE( 0, id );
	EXPECT_EQ( id, glyphLinkId( display->mSecondGlyph ) );
	ASSERT_TRUE( term->getHyperlink( id ) != nullptr );
	EXPECT_STDSTREQ( "https://example.com/", *term->getHyperlink( id )->uri );
	EXPECT_STDSTREQ( "codex", term->getHyperlink( id )->externalId );
}

UTEST( eterm, packed_hyperlinks_preserve_glyph_layout_and_attributes ) {
	EXPECT_EQ( static_cast<size_t>( 16 ), sizeof( TerminalGlyph ) );
	TerminalGlyph glyph{ 'x', ATTR_BOLD | ATTR_SEARCH_ACTIVE | ATTR_WIDE, 42, 7 };
	for ( uint16_t id : { uint16_t{ 1 }, uint16_t{ GLYPH_LINK_MAX }, uint16_t{ 0 } } ) {
		setGlyphLinkId( glyph, id );
		EXPECT_EQ( id, glyphLinkId( glyph ) );
		EXPECT_EQ( static_cast<uint32_t>( ATTR_BOLD | ATTR_SEARCH_ACTIVE | ATTR_WIDE ),
				   glyph.mode & GLYPH_ATTRIBUTE_MASK );
		EXPECT_EQ( static_cast<uint32_t>( 42 ), glyph.fg );
		EXPECT_EQ( static_cast<uint32_t>( 7 ), glyph.bg );
	}
}

UTEST( eterm, osc_hyperlinks_parse_fragments_parameters_and_uri_remainder ) {
	auto pty = std::make_unique<MockPty>();
	pty->mMaxRead = 1;
	pty->mBuffer = "a\033]8;other=yes:id=one:future=x;https://example.com/a;b?q=%20é\a"
				   "B\033[0mC\033]8;;\033\\d"
				   "\033]8;id=two;https://example.com/\033\\E"
				   "\033]8;id=two;https://example.com/\aF"
				   "\033]8;;https://example.com/\aG"
				   "\033]8;;https://example.com/\aH\033]8;;\aI";
	MockPty* input = pty.get();
	auto display = std::make_shared<MockDisplay>();
	auto term =
		TerminalEmulator::create( std::move( pty ), std::make_unique<MockProcess>(), display );
	while ( input->mReadOffset < input->mBuffer.size() )
		term->update();
	term->redraw();
	const auto& cells = display->mCells;
	EXPECT_EQ( 0, glyphLinkId( cells[0] ) );
	const auto first = glyphLinkId( cells[1] );
	ASSERT_NE( 0, first );
	EXPECT_EQ( first, glyphLinkId( cells[2] ) ); // SGR 0 does not close OSC 8.
	EXPECT_EQ( 0, glyphLinkId( cells[3] ) );
	ASSERT_TRUE( term->getHyperlink( first ) != nullptr );
	EXPECT_STDSTREQ( "https://example.com/a;b?q=%20é", *term->getHyperlink( first )->uri );
	EXPECT_STDSTREQ( "one", term->getHyperlink( first )->externalId );
	EXPECT_EQ( glyphLinkId( cells[4] ), glyphLinkId( cells[5] ) );
	EXPECT_NE( glyphLinkId( cells[5] ), glyphLinkId( cells[6] ) );
	EXPECT_NE( glyphLinkId( cells[6] ), glyphLinkId( cells[7] ) );
	EXPECT_EQ( 0, glyphLinkId( cells[8] ) );
}

UTEST( eterm, osc_hyperlinks_replace_invalid_runs_without_leaking_to_text ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033]8;;https://old.example/\aA\033]8;malformed\aB"
				   "\033]8;;relative/path\aC\033]8;;\aD\033]8;;https://example.com/" +
				   std::string( 8192, 'x' ) + "\aE";
	auto display = std::make_shared<MockDisplay>();
	auto term =
		TerminalEmulator::create( std::move( pty ), std::make_unique<MockProcess>(), display );
	term->update();
	ASSERT_NE( 0, glyphLinkId( display->mCells[0] ) );
	for ( int x = 1; x <= 4; ++x )
		EXPECT_EQ( 0, glyphLinkId( display->mCells[x] ) );
}

UTEST( eterm, osc_hyperlinks_follow_edits_and_clear_wide_cell_remnants ) {
	auto pty = std::make_unique<MockPty>();
	MockPty* input = pty.get();
	input->mBuffer = "\033]8;;https://example.com/\aAB界\033]8;;\a";
	auto display = std::make_shared<MockDisplay>();
	auto term =
		TerminalEmulator::create( std::move( pty ), std::make_unique<MockProcess>(), display );
	term->update();
	const auto id = glyphLinkId( display->mCells[0] );
	ASSERT_NE( 0, id );
	EXPECT_EQ( id, glyphLinkId( display->mCells[2] ) );
	EXPECT_EQ( id, glyphLinkId( display->mCells[3] ) );
	input->mBuffer += "\033[H\033[@";
	term->update();
	EXPECT_EQ( 0, glyphLinkId( display->mCells[0] ) );
	EXPECT_EQ( id, glyphLinkId( display->mCells[1] ) );
	input->mBuffer += "\033[P\033[1;4HX"; // Delete the inserted blank, overwrite the wide dummy.
	term->update();
	EXPECT_EQ( id, glyphLinkId( display->mCells[0] ) );
	EXPECT_EQ( 0, glyphLinkId( display->mCells[2] ) );
	EXPECT_EQ( 0, glyphLinkId( display->mCells[3] ) );
	input->mBuffer += "\033[1;3H\033]8;;https://example.com/\a界\033]8;;\a\033[1;3H\033[X";
	term->update();
	EXPECT_EQ( 0, glyphLinkId( display->mCells[2] ) );
	EXPECT_EQ( 0, glyphLinkId( display->mCells[3] ) );
	input->mBuffer += "\033[2J";
	term->update();
	for ( const auto& glyph : display->mCells )
		EXPECT_EQ( 0, glyphLinkId( glyph ) );
}

UTEST( eterm, osc_hyperlinks_survive_history_reflow_and_registry_churn ) {
	auto pty = std::make_unique<MockPty>();
	MockPty* input = pty.get();
	input->mCols = 20;
	input->mRows = 4;
	input->mBuffer = "\033]8;id=pinned;https://pinned.example/\a";
	for ( int line = 0; line < 10020; ++line )
		input->mBuffer += "linked history  \r\n";
	input->mBuffer += "\033]8;;\a";
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::make_unique<MockProcess>(),
										  display, 10000 );
	while ( input->mReadOffset < input->mBuffer.size() )
		term->update();
	term->redraw();
	const auto pinned = glyphLinkId( display->mCells[0] );
	ASSERT_NE( 0, pinned );
	input->mBuffer += "\033[?1049h";
	for ( int link = 0; link < 34000; ++link ) {
		input->mBuffer +=
			"\033[H\033]8;;https://churn.example/" + std::to_string( link ) + "\aX\033]8;;\a";
	}
	input->mBuffer += "\033[?1049l";
	while ( input->mReadOffset < input->mBuffer.size() )
		term->update();
	ASSERT_TRUE( term->getHyperlink( pinned ) != nullptr );
	EXPECT_STDSTREQ( "https://pinned.example/", *term->getHyperlink( pinned )->uri );
	for ( int columns : { 10, 40, 20 } ) {
		term->resize( columns, 4 );
		TerminalArg amount( term->scrollSize() );
		term->kscrollup( &amount );
		term->redraw();
		ASSERT_NE( 0, glyphLinkId( display->mCells[0] ) );
		for ( const auto& glyph : display->mCells ) {
			if ( auto id = glyphLinkId( glyph ) ) {
				ASSERT_TRUE( term->getHyperlink( id ) != nullptr );
				EXPECT_STDSTREQ( "https://pinned.example/", *term->getHyperlink( id )->uri );
			}
		}
	}
	term->reset(); // Scrollback remains live across RIS.
	ASSERT_TRUE( term->getHyperlink( pinned ) != nullptr );
	term->clearHistory();
	EXPECT_FALSE( term->hasHyperlinks() );
}

UTEST( eterm, osc_hyperlink_collection_scans_history_at_its_narrowed_width ) {
	auto pty = std::make_unique<MockPty>();
	MockPty* input = pty.get();
	input->mCols = 40;
	input->mRows = 4;
	input->mBuffer = "\033]8;;https://short.example/\aab\033]8;;\a\r\n";
	for ( int line = 0; line < 8; ++line )
		input->mBuffer += "cd\r\n";
	auto display = std::make_shared<MockDisplay>();
	auto term =
		TerminalEmulator::create( std::move( pty ), std::make_unique<MockProcess>(), display, 100 );
	while ( input->mReadOffset < input->mBuffer.size() )
		term->update();
	// Short history narrows without reflow: each line shrinks to 20 columns while the history
	// capacity remains 40. Collection must not read the released tail (ASan regression).
	term->resize( 20, 4 );
	term->reset();
	TerminalArg amount( term->scrollSize() );
	term->kscrollup( &amount );
	term->redraw();
	const auto id = glyphLinkId( display->mCells[0] );
	ASSERT_NE( 0, id );
	ASSERT_TRUE( term->getHyperlink( id ) != nullptr );
	EXPECT_STDSTREQ( "https://short.example/", *term->getHyperlink( id )->uri );
}

UTEST( eterm_session, osc_hyperlinks_are_owned_by_each_snapshot_after_id_reuse ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033]8;;https://original.example/\aOLD\033]8;;\a";
	auto session =
		TerminalSession::create( std::move( pty ), std::make_unique<MockProcess>(), 100 );
	auto original = waitForSnapshot( session, []( const TerminalSnapshot& snapshot ) {
		return !snapshot.visibleHyperlinks.empty() && snapshot.cells[0].u == 'O';
	} );
	ASSERT_TRUE( original != nullptr );
	session->reset();
	session->writeRaw( "\033]8;;https://replacement.example/\aNEW\033]8;;\a" );
	auto replacement = waitForSnapshot( session, []( const TerminalSnapshot& snapshot ) {
		return !snapshot.visibleHyperlinks.empty() && snapshot.cells[0].u == 'N';
	} );
	ASSERT_TRUE( replacement != nullptr );
	EXPECT_EQ( glyphLinkId( original->cells[0] ), glyphLinkId( replacement->cells[0] ) );
	EXPECT_STDSTREQ( "https://original.example/", *original->visibleHyperlinks[0].uri );
	EXPECT_STDSTREQ( "https://replacement.example/", *replacement->visibleHyperlinks[0].uri );
}

UTEST( eterm, linked_trailing_spaces_survive_narrow_and_wide_reflow ) {
	auto pty = std::make_unique<MockPty>();
	pty->mCols = 20;
	pty->mRows = 4;
	pty->mBuffer = "\033]8;;https://example.com/\aAB            \033]8;;\a";
	auto display = std::make_shared<MockDisplay>();
	auto term =
		TerminalEmulator::create( std::move( pty ), std::make_unique<MockProcess>(), display );
	term->update();
	for ( int columns : { 10, 30, 20 } ) {
		term->resize( columns, 4 );
		term->redraw();
		EXPECT_EQ( static_cast<std::ptrdiff_t>( 14 ),
				   std::count_if( display->mCells.begin(), display->mCells.end(),
								  []( const auto& glyph ) { return glyphLinkId( glyph ) != 0; } ) );
	}
}

UTEST( eterm, full_hyperlink_id_pool_omits_new_links_without_aliasing_live_cells ) {
	auto pty = std::make_unique<MockPty>();
	auto* input = pty.get();
	pty->mCols = 256;
	pty->mRows = 128;
	for ( uint32_t id = 0; id <= GLYPH_LINK_MAX; ++id )
		pty->mBuffer += "\033]8;;https://example.com/" + std::to_string( id ) + "\aX";
	pty->mBuffer += "\033]8;;\a";
	auto display = std::make_shared<MockDisplay>();
	auto term =
		TerminalEmulator::create( std::move( pty ), std::make_unique<MockProcess>(), display, 0 );
	while ( input->mReadOffset < input->mBuffer.size() )
		term->update();
	term->redraw();
	ASSERT_TRUE( term->getHyperlink( glyphLinkId( display->mCells.front() ) ) != nullptr );
	EXPECT_STDSTREQ( "https://example.com/0",
					 *term->getHyperlink( glyphLinkId( display->mCells.front() ) )->uri );
	EXPECT_EQ( 0, glyphLinkId( display->mCells.back() ) );
	EXPECT_EQ( static_cast<Rune>( 'X' ), display->mCells.back().u );
	EXPECT_EQ( static_cast<std::ptrdiff_t>( GLYPH_LINK_MAX ),
			   std::count_if( display->mCells.begin(), display->mCells.end(),
							  []( const auto& glyph ) { return glyphLinkId( glyph ) != 0; } ) );
	term->reset();
	EXPECT_FALSE( term->hasHyperlinks() );
}

UTEST( eterm_session, osc8_policy_disables_existing_links_and_future_output ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033]8;;https://example.com/\aA\033]8;;\a";
	auto session = TerminalSession::create( std::move( pty ), std::make_unique<MockProcess>(), 100,
											{}, TerminalSessionOptions{ true, false } );
	auto disabled = waitForSnapshot( session, []( const TerminalSnapshot& value ) {
		return !value.cells.empty() && value.cells[0].u == 'A';
	} );
	ASSERT_TRUE( disabled != nullptr );
	EXPECT_TRUE( disabled->visibleHyperlinks.empty() );
	EXPECT_EQ( 0, glyphLinkId( disabled->cells[0] ) );
	session->setOsc8HyperlinksEnabled( true );
	session->writeRaw( "\033]8;;https://example.com/\aB\033]8;;\a" );
	auto enabled = waitForSnapshot( session, []( const TerminalSnapshot& value ) {
		return !value.visibleHyperlinks.empty() && value.cells[1].u == 'B';
	} );
	ASSERT_TRUE( enabled != nullptr );
	session->setOsc8HyperlinksEnabled( false );
	session->writeRaw( "\033]8;;https://example.com/\aC\033]8;;\a" );
	auto again = waitForSnapshot( session, []( const TerminalSnapshot& value ) {
		return !value.cells.empty() && value.cells[2].u == 'C';
	} );
	ASSERT_TRUE( again != nullptr );
	EXPECT_TRUE( again->visibleHyperlinks.empty() );
	EXPECT_EQ( 0, glyphLinkId( again->cells[2] ) );
	EXPECT_FALSE( enabled->visibleHyperlinks.empty() ); // Retained snapshots remain immutable.
}

UTEST( eterm_display, osc_hyperlink_lookup_rejects_padding_and_grid_edges ) {
	EE::UI::UIApplication app(
		WindowSettings( 640, 480, "eterm hyperlink bounds", WindowStyle::Hidden ),
		EE::UI::UIApplication::Settings( Sys::getProcessPath() + "../", 1 ) );
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033]8;;https://example.com/\a界\033]8;;\a";
	auto session =
		TerminalSession::create( std::move( pty ), std::make_unique<MockProcess>(), 100 );
	ASSERT_TRUE( waitForSnapshot( session, []( const TerminalSnapshot& snapshot ) {
					 return !snapshot.visibleHyperlinks.empty();
				 } ) != nullptr );
	SelectionTestDisplay display( app.getWindow(),
								  app.getUI()->getUIThemeManager()->getDefaultFont(), session );
	display.setPosition( { 100, 100 } );
	const auto cell = display.getCellPixelSize();
	EXPECT_TRUE( display.getHyperlinkAt( { 111, 111 } ) == "https://example.com/" );
	EXPECT_TRUE( display.getHyperlinkAt( { 111, 111 } ) ==
				 display.getHyperlinkAt( { 111 + cell.getWidth(), 111 } ) );
	for ( const Vector2i& point :
		  { Vector2i{ 109, 111 }, Vector2i{ 111, 109 }, Vector2i{ 110 + 80 * cell.getWidth(), 111 },
			Vector2i{ 111, 110 + 24 * cell.getHeight() } } )
		EXPECT_FALSE( display.getHyperlinkAt( point ).has_value() );
	const auto generation = session->snapshot()->generation;
	session->writeRaw( "\033[H\033[P" ); // Delete only the leading half of the wide glyph.
	ASSERT_TRUE( waitForSnapshot( session, [generation]( const TerminalSnapshot& value ) {
					 return value.generation > generation;
				 } ) != nullptr );
	display.update( false );
	EXPECT_FALSE( display.getHyperlinkAt( { 111, 111 } ).has_value() );
}

UTEST( eterm_display, osc_hyperlinks_only_allow_explicit_safe_uri_schemes ) {
	for ( const auto* uri :
		  { "https://example.com/a;b?q=%20", "HTTP://example.com/", "mailto:user@example.com" } )
		EXPECT_TRUE( TerminalDisplay::isHyperlinkAllowed( uri ) );
	for ( const auto* uri :
		  { "file:///tmp/file", "ssh://host/", "javascript:alert(1)", "https:///",
			"https://example.com/%", "https://example.com/%GG", "https://example.com/\n" } )
		EXPECT_FALSE( TerminalDisplay::isHyperlinkAllowed( uri ) );
	EXPECT_FALSE(
		TerminalDisplay::isHyperlinkAllowed( std::string( "https://example.com/\0x", 22 ) ) );
}

UTEST( eterm_display, hyperlink_hover_tracks_snapshot_policy_and_pointer_bounds ) {
	EE::UI::UIApplication app(
		WindowSettings( 640, 480, "eterm hyperlink hover", WindowStyle::Hidden ),
		EE::UI::UIApplication::Settings( Sys::getProcessPath() + "../", 1 ) );
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033]8;id=group;https://example.com/target\a\033[4mFirst\033[0m"
				   "\033]8;;\a plain\r\n\033]8;id=group;https://example.com/target\a界"
				   "\033]8;;\a\033]8;;file:///tmp/file\aF\033]8;;\a";
	auto session =
		TerminalSession::create( std::move( pty ), std::make_unique<MockProcess>(), 100 );
	ASSERT_TRUE( waitForSnapshot( session, []( const TerminalSnapshot& snapshot ) {
					 return snapshot.visibleHyperlinks.size() == 2 && snapshot.cells[82].u == 'F';
				 } ) != nullptr );
	SelectionTestDisplay display( app.getWindow(),
								  app.getUI()->getUIThemeManager()->getDefaultFont(), session );
	display.setPosition( { 100, 100 } );
	const auto cell = display.getCellPixelSize();
	const Vector2i first{ 111, 111 };
	display.updateHyperlinkHover( first, false );
	EXPECT_TRUE( display.getHoveredHyperlink().empty() );
	display.updateHyperlinkHover( first, true );
	EXPECT_TRUE( display.getHoveredHyperlink() == "https://example.com/target" );
	// Moving within the same logical link retains the owned URI/cache rather than copying per
	// event.
	const char* target = display.getHoveredHyperlink().data();
	display.updateHyperlinkHover( { first.x + cell.getWidth(), first.y }, true );
	EXPECT_TRUE( target == display.getHoveredHyperlink().data() );
	display.updateHyperlinkHover( { first.x + cell.getWidth(), first.y + cell.getHeight() }, true );
	EXPECT_TRUE( display.getHoveredHyperlink() == "https://example.com/target" );
	for ( const Vector2i& point :
		  { Vector2i{ 109, 111 }, Vector2i{ 111, 109 }, Vector2i{ 111 + 6 * cell.getWidth(), 111 },
			Vector2i{ 111 + 2 * cell.getWidth(), 111 + cell.getHeight() } } ) {
		display.updateHyperlinkHover( point, true );
		EXPECT_TRUE( display.getHoveredHyperlink().empty() );
	}
	display.updateHyperlinkHover( first, true );
	display.setOsc8HyperlinksEnabled( false );
	EXPECT_TRUE( display.getHoveredHyperlink().empty() );
	display.updateHyperlinkHover( first, true );
	EXPECT_TRUE( display.getHoveredHyperlink().empty() );
	display.setOsc8HyperlinksEnabled( true );
	session->reset();
	ASSERT_TRUE( waitForSnapshot( session, []( const TerminalSnapshot& snapshot ) {
					 return snapshot.visibleHyperlinks.empty();
				 } ) != nullptr );
	display.update( false );
	display.updateHyperlinkHover( first, true );
	EXPECT_TRUE( display.getHoveredHyperlink().empty() );
}

class HyperlinkTestWidget : public eterm::UI::UITerminal {
  public:
	explicit HyperlinkTestWidget( const std::shared_ptr<TerminalDisplay>& display ) :
		UITerminal( display ) {}

	using UITerminal::isHyperlinkModifierPressed;
	using UITerminal::onCreateContextMenu;
	using UITerminal::onMouseDown;
	using UITerminal::onMouseMove;
	using UITerminal::onMouseUp;
	using UITerminal::updateHyperlinkHover;

	const std::string& contextHyperlink() const { return mContextHyperlink; }

	void destroyMenu() {
		auto* menu = mCurrentMenu;
		eeDelete( menu );
	}
};

UTEST( eterm_ui, hyperlink_hover_updates_when_modifiers_change_over_a_stationary_pointer ) {
	EE::UI::UIApplication app(
		WindowSettings( 640, 480, "eterm stationary hover", WindowStyle::Hidden ),
		EE::UI::UIApplication::Settings( Sys::getProcessPath() + "../", 1 ) );
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033]8;;https://example.com/\aLink\033]8;;\a";
	auto session =
		TerminalSession::create( std::move( pty ), std::make_unique<MockProcess>(), 100 );
	ASSERT_TRUE( waitForSnapshot( session, []( const TerminalSnapshot& snapshot ) {
					 return !snapshot.visibleHyperlinks.empty();
				 } ) != nullptr );
	auto display = std::make_shared<SelectionTestDisplay>(
		app.getWindow(), app.getUI()->getUIThemeManager()->getDefaultFont(), session );
	auto* terminal = eeNew( HyperlinkTestWidget, ( display ) );
	terminal->setParent( app.getUI()->getRoot() );
	terminal->setPixelsSize( 400, 200 );
	app.getUI()->flushDirtyStyleAndLayout();
	const auto point = terminal->convertToWorldSpace( { 5, 5 } ).asInt();
	ASSERT_TRUE( display->getHyperlinkAt( point ).has_value() );
	terminal->getEventDispatcher()->setMouseOverNode( terminal );
	auto* input = app.getWindow()->getInput();
	InputEvent key( InputEvent::KeyDown );
	key.key = {};
	const auto defaultModifier = KeyMod::getDefaultModifier();
	const auto modifierKeys = KeyMod::getKeyCodesFromModifier( defaultModifier );
	ASSERT_FALSE( modifierKeys.empty() );
	key.key.keysym.sym = modifierKeys.front();
	key.key.keysym.scancode = input->getScancodeFromKey( modifierKeys.front() );
	key.key.keysym.mod = defaultModifier;
	input->processEvent( &key );
	terminal->updateHyperlinkHover( point );
	// Hidden native hosts have no input focus. Offscreen inspection supports logical input;
	// validate both behaviors with native and headless focused runs.
	const bool active = app.getWindow()->hasInputFocus() || Runtime::isOffscreen();
	EXPECT_EQ( active, !display->getHoveredHyperlink().empty() );
	if ( active )
		EXPECT_EQ( Cursor::SysHand, app.getWindow()->getCursorManager()->getCurrentSysCursor() );
	key.Type = InputEvent::KeyUp;
	key.key.keysym.mod = 0;
	input->processEvent( &key );
	terminal->updateHyperlinkHover( point );
	EXPECT_TRUE( display->getHoveredHyperlink().empty() );
	EXPECT_NE( Cursor::SysHand, app.getWindow()->getCursorManager()->getCurrentSysCursor() );
	key.Type = InputEvent::KeyDown;
	key.key.keysym.mod = defaultModifier;
	input->processEvent( &key );
	terminal->updateHyperlinkHover( point );
	terminal->updateHyperlinkHover( { -1, -1 } );
	EXPECT_TRUE( display->getHoveredHyperlink().empty() );
	EXPECT_NE( Cursor::SysHand, app.getWindow()->getCursorManager()->getCurrentSysCursor() );
	key.Type = InputEvent::KeyUp;
	key.key.keysym.mod = 0;
	input->processEvent( &key );
	eeDelete( terminal );
}

UTEST( eterm_ui, hyperlink_gestures_balance_mouse_reports_and_retain_menu_context ) {
	EE::UI::UIApplication app(
		WindowSettings( 640, 480, "eterm hyperlink input", WindowStyle::Hidden ),
		EE::UI::UIApplication::Settings( Sys::getProcessPath() + "../", 1 ) );
	auto pty = std::make_unique<MockPty>();
	auto* inputPty = pty.get();
	pty->mLoopWrites = false;
	pty->mBuffer = "\033[?1000h\033[?1006h\033]8;;https://example.com/\a" + std::string( 40, 'X' ) +
				   "\033]8;;\a";
	auto session =
		TerminalSession::create( std::move( pty ), std::make_unique<MockProcess>(), 100 );
	ASSERT_TRUE( waitForSnapshot( session, []( const TerminalSnapshot& value ) {
					 return !value.visibleHyperlinks.empty();
				 } ) != nullptr );
	auto display = std::make_shared<SelectionTestDisplay>(
		app.getWindow(), app.getUI()->getUIThemeManager()->getDefaultFont(), session );
	auto* terminal = eeNew( HyperlinkTestWidget, ( display ) );
	terminal->setParent( app.getUI()->getRoot() );
	terminal->setPixelsSize( 400, 200 );
	app.getUI()->flushDirtyStyleAndLayout();
	const auto point = terminal->convertToWorldSpace( { 20, 5 } ).asInt();
	ASSERT_TRUE( display->getHyperlinkAt( point ).has_value() );
	auto* input = app.getWindow()->getInput();
	InputEvent key( InputEvent::KeyDown );
	key.key = {};
	const auto defaultModifier = KeyMod::getDefaultModifier();
	const auto modifierKeys = KeyMod::getKeyCodesFromModifier( defaultModifier );
	ASSERT_FALSE( modifierKeys.empty() );
	key.key.keysym.sym = modifierKeys.front();
	key.key.keysym.scancode = input->getScancodeFromKey( modifierKeys.front() );
	key.key.keysym.mod = defaultModifier;
	input->processEvent( &key );
	EXPECT_TRUE( terminal->isHyperlinkModifierPressed() );
	key.key.keysym.mod = defaultModifier | KeyMod::getDefaultSecondaryModifier();
	input->processEvent( &key );
	EXPECT_FALSE( terminal->isHyperlinkModifierPressed() );
	key.key.keysym.mod = KeyMod::getDefaultSecondaryModifier();
	input->processEvent( &key );
	EXPECT_FALSE( terminal->isHyperlinkModifierPressed() );
	key.key.keysym.mod = defaultModifier;
	input->processEvent( &key );
	terminal->onMouseDown( point, EE_BUTTON_LMASK );
	terminal->onMouseMove( point, EE_BUTTON_LMASK );
	key.Type = InputEvent::KeyUp;
	key.key.keysym.mod = 0;
	input->processEvent( &key );
	terminal->onMouseUp( { -100, -100 },
						 EE_BUTTON_LMASK ); // Cancel navigation; never launch a browser.
	session->requestSelection();			// Ordered barrier after mouse commands.
	EXPECT_TRUE( inputPty->mWrites.empty() );
	// The button really is held: a forwarded press is forgotten once Input reports a release.
	InputEvent button;
	button.Type = InputEvent::MouseButtonDown;
	button.button.button = EE_BUTTON_LEFT;
	input->processEvent( &button );
	terminal->onMouseDown( point, EE_BUTTON_LMASK );
	key.Type = InputEvent::KeyDown;
	key.key.keysym.mod = defaultModifier;
	input->processEvent( &key );
	// Held buttons can produce repeated downs. Adding the modifier must not turn an existing app
	// press into a hyperlink gesture and steal its release.
	terminal->onMouseDown( point, EE_BUTTON_LMASK );
	button.Type = InputEvent::MouseButtonUp;
	input->processEvent( &button );
	terminal->onMouseUp( { -100, -100 }, EE_BUTTON_LMASK );
	session->requestSelection();
	EXPECT_EQ( static_cast<std::ptrdiff_t>( 2 ),
			   std::count( inputPty->mWrites.begin(), inputPty->mWrites.end(), '\033' ) );
	EXPECT_NE( std::string::npos, inputPty->mWrites.find( 'M' ) );
	EXPECT_NE( std::string::npos, inputPty->mWrites.find( 'm' ) );
	ASSERT_TRUE( terminal->onCreateContextMenu( point, EE_BUTTON_RMASK ) );
	EXPECT_STDSTREQ( "https://example.com/", terminal->contextHyperlink() );
	session->reset();
	ASSERT_TRUE( waitForSnapshot( session, []( const TerminalSnapshot& value ) {
					 return value.visibleHyperlinks.empty();
				 } ) != nullptr );
	display->update( false );
	EXPECT_FALSE( display->getHyperlinkAt( point ).has_value() );
	EXPECT_STDSTREQ( "https://example.com/", terminal->contextHyperlink() );
	key.Type = InputEvent::KeyUp;
	key.key.keysym.mod = 0;
	input->processEvent( &key );
	terminal->destroyMenu();
	eeDelete( terminal );
}

UTEST( eterm, osc7_only_publishes_valid_local_directories ) {
	auto pty = std::make_unique<MockPty>();
	auto* input = pty.get();
	input->mMaxRead = 1;
	auto display = std::make_shared<MockDisplay>();
	auto term =
		TerminalEmulator::create( std::move( pty ), std::make_unique<MockProcess>(), display );
	const auto path = Sys::getTempPath() + "eterm spaces;é";
	auto uri = URI::fromFSPath( path );
	uri.setHost( "localhost" );
	input->mBuffer = "\033]7;" + uri.toString() + "\033\\";
	while ( input->mReadOffset < input->mBuffer.size() )
		term->update();
	EXPECT_STDSTREQ( uri.getFSPath(), term->getCurrentWorkingDirectory() );
	for ( const auto* value :
		  { "file://remote.invalid/tmp/", "file://localhost/abc%00def", "file://localhost/abc%GG",
			"file://localhost/abc%", "https://localhost/tmp/", "file:relative", "file://localhost",
			"file://localhost/tmp/?query", "file://localhost/tmp/#fragment",
			"file://localhost:123/tmp/", "file://user@localhost/tmp/" } ) {
		input->mBuffer += "\033]7;" + uri.toString() + "\a\033]7;" + value + "\a";
		while ( input->mReadOffset < input->mBuffer.size() )
			term->update();
		EXPECT_TRUE( term->getCurrentWorkingDirectory().empty() );
	}
	uri.setHost( "" );
	input->mBuffer += "\033]7;" + uri.toString() + "\a";
	while ( input->mReadOffset < input->mBuffer.size() )
		term->update();
	EXPECT_STDSTREQ( uri.getFSPath(), term->getCurrentWorkingDirectory() );
	term->reset();
	EXPECT_TRUE( term->getCurrentWorkingDirectory().empty() );
}

UTEST( eterm, device_status_reports_operating_status_and_cursor_position ) {
	auto pty = std::make_unique<MockPty>();
	// Benchmarks such as `kitten __benchmark__` wait for DSR 5 replies as a parse barrier.
	pty->mBuffer = "ab\033[5n\033[6n\033[5n";
	pty->mLoopWrites = false;
	MockPty* ptyPtr = pty.get();
	auto term = TerminalEmulator::create( std::move( pty ), std::make_unique<MockProcess>(),
										  std::make_shared<MockDisplay>(), 100 );
	term->update();
	EXPECT_STDSTREQ( "\033[0n\033[1;3R\033[0n", ptyPtr->mWrites );
}

UTEST( eterm, osc52_validates_payload_selection_and_terminators ) {
	auto pty = std::make_unique<MockPty>();
	auto* input = pty.get();
	input->mMaxRead = 1;
	input->mLoopWrites = false;
	input->mBuffer = "\033]52;c;SGVsbG8=\a\033]52;;V29ybGQ=\033\\\033]52;c;\a";
	auto display = std::make_shared<MockDisplay>();
	auto term =
		TerminalEmulator::create( std::move( pty ), std::make_unique<MockProcess>(), display );
	while ( input->mReadOffset < input->mBuffer.size() )
		term->update();
	ASSERT_EQ( static_cast<size_t>( 3 ), display->mClipboardWrites.size() );
	EXPECT_STDSTREQ( "Hello", display->mClipboardWrites[0] );
	EXPECT_STDSTREQ( "World", display->mClipboardWrites[1] );
	EXPECT_TRUE( display->mClipboardWrites[2].empty() );
	for ( const auto* value : { "c;?", "x;SGVsbG8=", "cx;SGVsbG8=", "c;!!!!", "c;SGVsbG8=;ignored",
								"c;SGVs bG8=", "c;SGVsbG8===", "c;AA==", "c;SGVsbG8=AA" } )
		input->mBuffer += std::string( "\033]52;" ) + value + "\a";
	while ( input->mReadOffset < input->mBuffer.size() )
		term->update();
	EXPECT_EQ( static_cast<size_t>( 3 ), display->mClipboardWrites.size() );
	// Every xterm target writes the single system clipboard (Neovim sends `s` for "*).
	for ( const auto* value : { "p;cA==", "s;cw==", "pc;cGM=", "0;MA==" } )
		input->mBuffer += std::string( "\033]52;" ) + value + "\a";
	while ( input->mReadOffset < input->mBuffer.size() )
		term->update();
	ASSERT_EQ( static_cast<size_t>( 7 ), display->mClipboardWrites.size() );
	EXPECT_STDSTREQ( "p", display->mClipboardWrites[3] );
	EXPECT_STDSTREQ( "s", display->mClipboardWrites[4] );
	EXPECT_STDSTREQ( "pc", display->mClipboardWrites[5] );
	EXPECT_STDSTREQ( "0", display->mClipboardWrites[6] );
	EXPECT_TRUE( input->mWrites.empty() ); // Queries cannot disclose clipboard contents.
	input->mMaxRead = std::numeric_limits<size_t>::max();
	input->mBuffer += "\033]52;c;" + std::string( 350000, 'A' ) + "\a";
	while ( input->mReadOffset < input->mBuffer.size() )
		term->update();
	EXPECT_EQ( static_cast<size_t>( 7 ), display->mClipboardWrites.size() );
}

UTEST( eterm_session, osc7_reports_publish_without_following_printable_output ) {
	auto pty = std::make_unique<MockPty>();
	const auto uri = URI::fromFSPath( Sys::getTempPath() );
	pty->mBuffer = "\033]7;" + uri.toString() + "\a";
	auto session =
		TerminalSession::create( std::move( pty ), std::make_unique<MockProcess>(), 100 );
	auto local = waitForSnapshot( session, []( const TerminalSnapshot& value ) {
		return !value.currentWorkingDirectory.empty();
	} );
	ASSERT_TRUE( local != nullptr );
	EXPECT_STDSTREQ( uri.getFSPath(), local->currentWorkingDirectory );
	session->writeRaw( "\033]7;file://remote.invalid/tmp/\a" );
	auto remote = waitForSnapshot(
		session, [generation = local->generation]( const TerminalSnapshot& value ) {
			return value.generation > generation && value.currentWorkingDirectory.empty();
		} );
	ASSERT_TRUE( remote != nullptr );
}

UTEST( eterm_session, osc52_policy_applies_before_startup_and_hot_updates_on_worker ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033]52;c;SGVsbG8=\aA";
	auto session = TerminalSession::create( std::move( pty ), std::make_unique<MockProcess>(), 100,
											{}, TerminalSessionOptions{ false, true } );
	ASSERT_TRUE( waitForSnapshot( session, []( const TerminalSnapshot& value ) {
					 return !value.cells.empty() && value.cells[0].u == 'A';
				 } ) != nullptr );
	for ( const auto& event : session->drainEvents() )
		EXPECT_NE( TerminalSession::EventType::Clipboard, event.type );
	session->setAllowOsc52ClipboardWrite( true );
	session->writeRaw( "\033]52;c;SGVsbG8=\033\\B" );
	ASSERT_TRUE( waitForSnapshot( session, []( const TerminalSnapshot& value ) {
					 return !value.cells.empty() && value.cells[1].u == 'B';
				 } ) != nullptr );
	int clipboardEvents = 0;
	for ( const auto& event : session->drainEvents() ) {
		if ( event.type == TerminalSession::EventType::Clipboard ) {
			++clipboardEvents;
			EXPECT_STDSTREQ( "Hello", event.data );
		}
	}
	EXPECT_EQ( 1, clipboardEvents );
	session->setAllowOsc52ClipboardWrite( false );
	session->reset();
	session->writeRaw( "\033]52;c;SGVsbG8=\aC" );
	ASSERT_TRUE( waitForSnapshot( session, []( const TerminalSnapshot& value ) {
					 return !value.cells.empty() && value.cells[0].u == 'C';
				 } ) != nullptr );
	for ( const auto& event : session->drainEvents() )
		EXPECT_NE( TerminalSession::EventType::Clipboard, event.type );
}

UTEST( eterm, alternate_scroll_mode_controls_wheel_key_translation ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033[?1049h\033[?1007l";
	pty->mLoopWrites = false;
	MockPty* ptyPtr = pty.get();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	term->update();
	term->mousereport( TerminalMouseEventType::MouseButtonDown, { 0, 0 }, { 0, 0 },
					   EE_BUTTON_WUMASK, 0 );
	EXPECT_TRUE( ptyPtr->mWrites.empty() );

	ptyPtr->mBuffer += "\033[?1007h";
	term->update();
	term->mousereport( TerminalMouseEventType::MouseButtonDown, { 0, 0 }, { 0, 0 },
					   EE_BUTTON_WUMASK, 0 );
	EXPECT_STDSTREQ( "\033[A", ptyPtr->mWrites );
}

UTEST( eterm, color_scheme_query_and_change_notification ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033[?996n\033[?2031h";
	pty->mLoopWrites = false;
	MockPty* ptyPtr = pty.get();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	term->update();
	EXPECT_STDSTREQ( "\033[?997;1n", ptyPtr->mWrites );

	display->mBackground = 0xF0F0F0FF;
	term->notifyColorSchemeChanged();
	EXPECT_STDSTREQ( "\033[?997;1n\033[?997;2n", ptyPtr->mWrites );

	ptyPtr->mBuffer += "\033[?2031l";
	term->update();
	display->mBackground = 0x101010FF;
	term->notifyColorSchemeChanged();
	EXPECT_STDSTREQ( "\033[?997;1n\033[?997;2n", ptyPtr->mWrites );
}

UTEST( eterm, basic_write ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	term->write( "ABC", 3 );
	term->update();

	term->selstart( 0, 0, 0 );
	term->selextend( 2, 0, 1, 0 );
	EXPECT_TRUE( term->hasSelection() );
	EXPECT_STDSTREQ( "ABC", term->getSelection() );
}

UTEST( eterm, printable_runs_clear_only_overwritten_selections ) {
	auto pty = std::make_unique<MockPty>();
	auto display = std::make_shared<MockDisplay>();
	auto term =
		TerminalEmulator::create( std::move( pty ), std::make_unique<MockProcess>(), display, 100 );
	term->write( "ABCDEFGH", 8 );
	term->update();

	term->selstart( 5, 0, 0 );
	term->selextend( 7, 0, SEL_REGULAR, false );
	term->write( "\r12", 3 );
	term->update();
	EXPECT_STDSTREQ( "FGH", term->getSelection() );

	term->write( "\r\033[4Cxyz", 8 );
	term->update();
	EXPECT_FALSE( term->hasSelection() );

	// A full row leaves the cursor pending a wrap on the last column; the next character tests
	// the selection there before wrapping, as tputc() does.
	const std::string row = "\r\n" + std::string( 80, 'a' );
	term->write( row.data(), row.size() );
	term->update();
	term->selstart( 79, 1, 0 );
	term->selextend( 79, 1, SEL_REGULAR, false );
	EXPECT_TRUE( term->hasSelection() );
	term->write( "b", 1 );
	term->update();
	EXPECT_FALSE( term->hasSelection() );
	EXPECT_EQ( static_cast<Rune>( 'b' ), display->mCells[2 * display->mColumns].u );
}

UTEST( eterm, unicode_runs_keep_wide_cell_and_wrap_semantics ) {
	auto pty = std::make_unique<MockPty>();
	auto display = std::make_shared<MockDisplay>();
	auto term =
		TerminalEmulator::create( std::move( pty ), std::make_unique<MockProcess>(), display, 100 );
	// 79 narrow cells leave one column: the wide character wraps and marks the row as wrapped.
	const std::string text = std::string( 79, 'a' ) + "\xE7\x95\x8C\xF0\x9F\x98\x80\xE2\x94\x80"
													  "b";
	term->write( text.data(), text.size() );
	term->update();
	const auto cell = [&]( int x, int y ) { return display->mCells[y * display->mColumns + x]; };
	EXPECT_TRUE( ( cell( 79, 0 ).mode & ATTR_WRAP ) != 0 );
	EXPECT_EQ( static_cast<Rune>( 0x754C ), cell( 0, 1 ).u );
	EXPECT_TRUE( ( cell( 0, 1 ).mode & ATTR_WIDE ) != 0 );
	EXPECT_TRUE( ( cell( 1, 1 ).mode & ATTR_WDUMMY ) != 0 );
	EXPECT_EQ( static_cast<Rune>( 0x1F600 ), cell( 2, 1 ).u );
	EXPECT_TRUE( ( cell( 2, 1 ).mode & ( ATTR_WIDE | ATTR_EMOJI ) ) == ( ATTR_WIDE | ATTR_EMOJI ) );
	EXPECT_EQ( static_cast<Rune>( 0x2500 ), cell( 4, 1 ).u );
	EXPECT_TRUE( ( cell( 4, 1 ).mode & ATTR_BOXDRAW ) != 0 );
	EXPECT_EQ( static_cast<Rune>( 'b' ), cell( 5, 1 ).u );

	// Overwriting the second half of a wide character clears its first half.
	term->write( "\033[2;2Hx", 7 );
	term->update();
	EXPECT_EQ( static_cast<Rune>( ' ' ), cell( 0, 1 ).u );
	EXPECT_TRUE( ( cell( 0, 1 ).mode & ATTR_WIDE ) == 0 );
	EXPECT_EQ( static_cast<Rune>( 'x' ), cell( 1, 1 ).u );
}

namespace {

struct ClusterTerminal {
	MockPty* input{ nullptr };
	std::shared_ptr<MockDisplay> display{ std::make_shared<MockDisplay>() };
	std::shared_ptr<TerminalEmulator> term;

	ClusterTerminal() {
		auto pty = std::make_unique<MockPty>();
		pty->mLoopWrites = false;
		input = pty.get();
		term = TerminalEmulator::create( std::move( pty ), std::make_unique<MockProcess>(), display,
										 100 );
	}

	void feed( const std::string& text ) {
		input->mBuffer += text;
		while ( input->mReadOffset < input->mBuffer.size() )
			term->update();
		term->update();
	}

	const TerminalGlyph& cell( int x, int y = 0 ) const {
		return display->mCells[y * display->mColumns + x];
	}

	std::u32string text( int x, int y = 0 ) const {
		std::u32string codepoints;
		term->visitGlyphCodepoints( cell( x, y ), [&]( Rune codepoint ) {
			codepoints += static_cast<char32_t>( codepoint );
		} );
		return codepoints;
	}
};

} // namespace

UTEST( eterm, zero_width_codepoints_attach_to_the_previous_cell ) {
	ClusterTerminal t;
	// The wcwidth() widths used with grapheme clustering turned off.
	t.term->setGraphemeClusteringEnabled( false );
	// e + COMBINING ACUTE ACCENT, x; then a mark with nothing before it on a new row.
	t.feed( "e\xCC\x81x\r\n\xCC\x81"
			"a" );
	EXPECT_TRUE( isGlyphCluster( t.cell( 0 ) ) );
	EXPECT_TRUE( t.text( 0 ) == U"é" );
	EXPECT_EQ( static_cast<Rune>( 'x' ), t.cell( 1 ).u );
	EXPECT_EQ( static_cast<Rune>( 'a' ), t.cell( 0, 1 ).u );
	t.term->selstart( 0, 0, 0 );
	t.term->selextend( 1, 0, SEL_REGULAR, false );
	EXPECT_STDSTREQ( "e\xCC\x81x", t.term->getSelection() );

	// Without mode 2027 a variation selector attaches only to a pictograph and changes no width.
	t.feed( "\033[2J\033[Ha\xEF\xB8\x8F"
			"b\xE2\x9D\xA4\xEF\xB8\x8F"
			"c" );
	EXPECT_EQ( static_cast<Rune>( 'a' ), t.cell( 0 ).u );
	EXPECT_EQ( static_cast<Rune>( 'b' ), t.cell( 1 ).u );
	EXPECT_TRUE( t.text( 2 ) == U"❤️" );
	EXPECT_EQ( static_cast<Rune>( 'c' ), t.cell( 3 ).u );

	// The selector is checked against the character's base, not its last mark.
	t.feed( "\r\n\xE2\x9D\xA4\xCC\x81\xEF\xB8\x8F" );
	EXPECT_TRUE( t.text( 0, 1 ) == U"❤́️" );
}

UTEST( eterm, mode_2027_clusters_change_widths ) {
	ClusterTerminal t;
	t.feed( "\033[?2027h" );
	// HEAVY BLACK HEART + VS16 becomes wide; GRINNING FACE + VS15 becomes narrow.
	t.feed( "\xE2\x9D\xA4\xEF\xB8\x8F"
			"a\xF0\x9F\x98\x80\xEF\xB8\x8E"
			"b" );
	EXPECT_TRUE( t.text( 0 ) == U"❤️" );
	EXPECT_TRUE( ( t.cell( 0 ).mode & ATTR_WIDE ) != 0 );
	EXPECT_TRUE( ( t.cell( 1 ).mode & ATTR_WDUMMY ) != 0 );
	EXPECT_EQ( static_cast<Rune>( 'a' ), t.cell( 2 ).u );
	EXPECT_TRUE( t.text( 3 ) == U"\U0001F600︎" );
	EXPECT_TRUE( ( t.cell( 3 ).mode & ATTR_WIDE ) == 0 );
	EXPECT_EQ( static_cast<Rune>( 'b' ), t.cell( 4 ).u );

	// A ZWJ family and a flag are one wide cluster each.
	t.feed( "\r\n\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7"
			"c\xF0\x9F\x87\xA6\xF0\x9F\x87\xB7"
			"d" );
	EXPECT_TRUE( t.text( 0, 1 ) == U"\U0001F468‍\U0001F469‍\U0001F467" );
	EXPECT_EQ( static_cast<Rune>( 'c' ), t.cell( 2, 1 ).u );
	EXPECT_TRUE( t.text( 3, 1 ) == U"\U0001F1E6\U0001F1F7" );
	EXPECT_EQ( static_cast<Rune>( 'd' ), t.cell( 5, 1 ).u );

	// A cluster that becomes wide in the last column moves to the next row.
	t.feed( "\033[3;80H\xE2\x9D\xA4\xEF\xB8\x8F"
			"e" );
	EXPECT_TRUE( ( t.cell( 79, 2 ).mode & ATTR_WRAP ) != 0 );
	EXPECT_EQ( static_cast<Rune>( ' ' ), t.cell( 79, 2 ).u );
	EXPECT_TRUE( t.text( 0, 3 ) == U"❤️" );
	EXPECT_EQ( static_cast<Rune>( 'e' ), t.cell( 2, 3 ).u );

	// An Indic conjunct (GB9c: consonant + virama + consonant) is one cluster, even though the
	// second consonant arrives as the start of a new run.
	t.feed( "\033[2J\033[H\xE0\xA4\x95\xE0\xA5\x8D\xE0\xA4\x95"
			"f" );
	EXPECT_TRUE( t.text( 0 ) == U"क्क" );
	EXPECT_EQ( static_cast<Rune>( 'f' ), t.cell( 2 ).u );

	// A full cluster drops only its own extensions: a codepoint that starts a new cluster prints.
	std::string full = "\r\ne";
	for ( int mark = 0; mark < 31; ++mark )
		full += "\xCC\x81";
	t.feed( full + "\xCC\x81\xF0\x9F\x87\xA6" );
	EXPECT_EQ( static_cast<size_t>( 32 ), t.text( 0, 1 ).size() );
	EXPECT_TRUE( t.text( 1, 1 ) == U"\U0001F1E6" );

	// Without the mode the same ZWJ sequence keeps per-codepoint widths.
	t.feed( "\033[?2027l\033[2J\033[H\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9" );
	EXPECT_TRUE( t.text( 0 ) == U"\U0001F468‍" );
	EXPECT_TRUE( t.text( 2 ) == U"\U0001F469" );
}

UTEST( eterm, decrqm_reports_mode_2027_and_ansi_modes ) {
	ClusterTerminal t;
	// Grapheme clustering is on by default.
	t.feed( "\033[?2027$p\033[?2027l\033[?2027$p\033[4$p\033[?9999$p" );
	EXPECT_STDSTREQ( "\033[?2027;1$y\033[?2027;2$y\033[4;2$y\033[?9999;0$y", t.input->mWrites );
}

UTEST( eterm, grapheme_clustering_setting_is_the_mode_after_reset ) {
	ClusterTerminal t;
	// A reset restores the configured mode, not whatever the last program left.
	t.feed( "\033[?2027l\033c\033[?2027$p" );
	EXPECT_STDSTREQ( "\033[?2027;1$y", t.input->mWrites );
	t.input->mWrites.clear();

	// Turning the setting off applies to the current mode and to resets.
	t.term->setGraphemeClusteringEnabled( false );
	t.feed( "\033[?2027$p\033[?2027h\033c\033[?2027$p" );
	EXPECT_STDSTREQ( "\033[?2027;2$y\033[?2027;2$y", t.input->mWrites );
	t.feed( "\033[2J\033[H\xF0\x9F\x87\xA6\xF0\x9F\x87\xB7" );
	EXPECT_TRUE( t.text( 0 ) == U"\U0001F1E6" );
	EXPECT_TRUE( t.text( 1 ) == U"\U0001F1F7" );

	// Turned back on, a flag is one wide cluster.
	t.term->setGraphemeClusteringEnabled( true );
	t.feed( "\033[2J\033[H\xF0\x9F\x87\xA6\xF0\x9F\x87\xB7" );
	EXPECT_TRUE( t.text( 0 ) == U"\U0001F1E6\U0001F1F7" );
	EXPECT_TRUE( ( t.cell( 0 ).mode & ATTR_WIDE ) != 0 );
}

UTEST( eterm, clusters_are_reclaimed_and_published_in_snapshots ) {
	ClusterTerminal t;
	t.feed( "e\xCC\x81" );
	EXPECT_TRUE( t.term->hasClusters() );
	t.feed( "\033c" );
	EXPECT_FALSE( t.term->hasClusters() );

	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "o\xCC\x88";
	auto session =
		TerminalSession::create( std::move( pty ), std::make_unique<MockProcess>(), 100 );
	auto snapshot = waitForSnapshot( session, []( const TerminalSnapshot& snapshot ) {
		return !snapshot.cells.empty() && isGlyphCluster( snapshot.cells[0] );
	} );
	ASSERT_TRUE( snapshot != nullptr );
	const auto* cluster = snapshot->findCluster( glyphClusterId( snapshot->cells[0] ) );
	ASSERT_TRUE( cluster != nullptr );
	EXPECT_TRUE( *cluster == U"ö" );
	EXPECT_EQ( static_cast<Rune>( 'o' ), snapshot->glyphBaseCodepoint( snapshot->cells[0] ) );
}

UTEST( eterm, rep_ignores_negative_counts_and_defaults_zero ) {
	auto pty = std::make_unique<MockPty>();
	auto display = std::make_shared<MockDisplay>();
	auto term =
		TerminalEmulator::create( std::move( pty ), std::make_unique<MockProcess>(), display, 100 );
	term->write( "A\033[-1bB\033[0bC", 13 );
	term->update();
	const std::string expected = "ABBC ";
	for ( size_t x = 0; x < expected.size(); ++x )
		EXPECT_EQ( static_cast<Rune>( expected[x] ), display->mCells[x].u );
}

UTEST( eterm, csi_numbers_parse_like_strtol_for_this_platforms_long ) {
	auto pty = std::make_unique<MockPty>();
	auto display = std::make_shared<MockDisplay>();
	auto term =
		TerminalEmulator::create( std::move( pty ), std::make_unique<MockProcess>(), display, 100 );
	// Values around 9 digits (32-bit long, as on Windows) and 18 digits (64-bit long), where the
	// hand-written parser hands over to strtol(). CHA moves to the parsed column, clamped.
	for ( const char* number :
		  { "7", "999999999", "1000000000", "9999999999", "4294967296", "4294967297",
			"999999999999999999", "1000000000000000000", "9999999999999999999",
			"0000000000000000000000005", "99999999999999999999999999" } ) {
		long value = std::strtol( number, nullptr, 10 );
		if ( value == LONG_MAX || value == LONG_MIN )
			value = -1;
		int argument = static_cast<int>( value );
		if ( argument == 0 )
			argument = 1;
		const int expectedColumn = std::clamp( argument - 1, 0, 79 );
		const std::string sequence = std::string( "\033[2J\033[H\033[" ) + number + "GX";
		term->write( sequence.data(), sequence.size() );
		term->update();
		int column = -1;
		for ( int x = 0; x < 80; ++x )
			if ( display->mCells[x].u == 'X' )
				column = x;
		EXPECT_EQ( expectedColumn, column );
	}
}

UTEST( eterm, synchronized_updates_publish_only_complete_frames ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	term->update();
	display->mDrawEnds = 0;
	const char partialFrame[] = "\033[?2026h\033[2Jpartial";
	term->write( partialFrame, sizeof( partialFrame ) - 1 );
	term->update();

	EXPECT_EQ( 0, display->mDrawEnds );

	const char completeFrame[] = "\033[Hcomplete\033[?2026l";
	term->write( completeFrame, sizeof( completeFrame ) - 1 );
	term->update();

	EXPECT_TRUE( display->mDrawEnds > 0 );
	term->selstart( 0, 0, 0 );
	term->selextend( 7, 0, SEL_REGULAR, false );
	EXPECT_STDSTREQ( "complete", term->getSelection() );
}

UTEST( eterm, pty_parsing_does_not_publish_temporary_bottom_viewport ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	for ( int line = 0; line < 40; ++line ) {
		const std::string output = "history " + std::to_string( line ) + "\r\n";
		term->write( output.data(), output.size() );
		term->update();
	}

	TerminalArg scroll( 5 );
	term->kscrollup( &scroll );
	ASSERT_EQ( 5, term->scrollPos() );
	display->mPublishedScrollPositions.clear();

	const char synchronizedOutput[] = "\033[?2026hnew output\r\n\033[?2026l";
	term->write( synchronizedOutput, sizeof( synchronizedOutput ) - 1 );
	term->update();

	ASSERT_FALSE( display->mPublishedScrollPositions.empty() );
	for ( int scrollPosition : display->mPublishedScrollPositions )
		EXPECT_TRUE( scrollPosition > 0 );
	EXPECT_EQ( 6, display->mPublishedScrollPositions.back() );
}

UTEST( eterm, sgr_colon_subparameters_preserve_groups_and_optional_color_space ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	const char sequence[] = "\033[58:2::255:192:185;59;1;38:2::12:34:56;48:5:42mX";
	term->write( sequence, sizeof( sequence ) - 1 );
	term->update();

	EXPECT_TRUE( display->mFirstGlyph.mode & ATTR_BOLD );
	EXPECT_EQ( 0x010C2238u, display->mFirstGlyph.fg );
	EXPECT_EQ( 42u, display->mFirstGlyph.bg );
}

UTEST( eterm, invalid_indexed_color_keeps_the_previous_color ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	const char sequence[] = "\033[31mA\033[38;5;283mB";
	term->write( sequence, sizeof( sequence ) - 1 );
	term->update();

	EXPECT_EQ( 1u, display->mFirstGlyph.fg );
	EXPECT_EQ( 1u, display->mSecondGlyph.fg );
}

UTEST( eterm, osc_palette_queries_return_each_requested_color ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033]4;0;?;1;?\a";
	pty->mLoopWrites = false;
	MockPty* ptyPtr = pty.get();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	term->update();

	EXPECT_STDSTREQ( "\033]4;0;rgb:0101/0202/0303\a\033]4;1;rgb:0404/0505/0606\a",
					 ptyPtr->mWrites );
}

UTEST( eterm, osc_color_resets_reach_palette_and_dynamic_defaults ) {
	auto pty = std::make_unique<MockPty>();
	pty->mBuffer = "\033]104;1;2\a\033]110\a\033]111\a\033]112\a\033]104\a";
	pty->mLoopWrites = false;
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	term->update();

	ASSERT_EQ( static_cast<size_t>( 5 ), display->mResetColorIndices.size() );
	EXPECT_EQ( 1u, display->mResetColorIndices[0] );
	EXPECT_EQ( 2u, display->mResetColorIndices[1] );
	EXPECT_EQ( 258u, display->mResetColorIndices[2] );
	EXPECT_EQ( 259u, display->mResetColorIndices[3] );
	EXPECT_EQ( 256u, display->mResetColorIndices[4] );
	EXPECT_EQ( 2, display->mResetColorsCount );
}

UTEST( eterm, selection_redraw_while_idle ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	term->write( "ABC", 3 );
	term->update();
	display->mDrawLines = 0;

	term->selstart( 0, 0, 0 );
	term->selextend( 2, 0, 1, 0 );
	term->update();

	EXPECT_TRUE( display->mDrawLines > 0 );
}

UTEST( eterm, saturated_read_defers_only_intermediate_draw ) {
	for ( bool slowFirstRead : { false, true } ) {
		auto pty = std::make_unique<MockPty>();
		pty->mMaxRead = 1;
		MockPty* ptyPtr = pty.get();
		auto process = std::make_unique<MockProcess>();
		auto display = std::make_shared<MockDisplay>();
		auto term =
			TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

		// Isolate read-budget deferral from the independent periodic presentation deadline.
		term->setPresentationInterval( Seconds( 60 ) );
		if ( slowFirstRead ) {
			term->setDataCb( [firstRead = true]( const char*, size_t ) mutable {
				if ( firstRead ) {
					firstRead = false;
					// Force the time budget to expire before the 1024-read limit.
					std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
				}
			} );
		}

		std::string output( 1024, 'A' );
		term->write( output.data(), output.size() );
		display->mDrawLines = 0;

		EXPECT_FALSE( term->update() );
		EXPECT_EQ( 0, display->mDrawLines );

		// A time-limited slice can leave input for later updates. Even the update that drains
		// it returns false so the producer gets a scheduling opportunity before the idle probe.
		bool idle = false;
		for ( size_t attempts = 0; !idle && attempts <= output.size(); ++attempts )
			idle = term->update();
		EXPECT_TRUE( idle );
		EXPECT_EQ( output.size(), ptyPtr->mBytesRead.load() );
		EXPECT_TRUE( display->mDrawLines > 0 );
	}
}

UTEST( eterm, sustained_saturated_reads_present_periodically ) {
	auto pty = std::make_unique<MockPty>();
	pty->mMaxRead = 1;
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	term->update();
	display->mDrawLines = 0;
	std::string output( 1024 * 1024, 'A' );
	term->write( output.data(), output.size() );

	const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds( 100 );
	while ( display->mDrawLines == 0 && std::chrono::steady_clock::now() < deadline )
		term->update();
	EXPECT_TRUE( display->mDrawLines > 0 );
}

UTEST( eterm, process_exit_drains_buffered_pty_output ) {
	auto pty = std::make_unique<MockPty>();
	pty->mMaxRead = 1;
	auto process = std::make_unique<MockProcess>();
	process->mExited = true;
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	size_t bytesRead = 0;
	term->setDataCb( [&bytesRead]( const char*, size_t size ) { bytesRead += size; } );
	std::string output( 3 * 1024, 'A' );
	term->write( output.data(), output.size() );
	display->mDrawLines = 0;

	EXPECT_FALSE( term->update() );
	EXPECT_EQ( static_cast<size_t>( 1024 ), bytesRead );
	while ( !term->update() ) {
	}

	EXPECT_EQ( output.size(), bytesRead );
	EXPECT_TRUE( display->mDrawLines > 0 );
}

UTEST( eterm, repeated_deferred_scrolls_redraw_every_row ) {
	auto pty = std::make_unique<MockPty>();
	MockPty* mockPty = pty.get();
	pty->mMaxRead = 1;
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	// Consume the initial full redraw so this burst starts with clean dirty flags.
	term->update();
	display->mDrawLines = 0;

	for ( int line = 0; line < 200; ++line )
		mockPty->mBuffer += "scrolling line " + std::to_string( line ) + "\r\n";

	EXPECT_FALSE( term->update() );
	EXPECT_EQ( 0, display->mDrawLines );
	while ( !term->update() ) {
	}
	EXPECT_TRUE( display->mDrawLines >= 24 );
	EXPECT_EQ( 0, display->mDrawLines % 24 );
}

UTEST( eterm, ascii_overwrite_clears_wide_glyph_state ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	term->write( "\xF0\x9F\x9A\x80\rA", 6 );
	term->update();

	EXPECT_EQ( 0u, display->mFirstMode & ATTR_WIDE );
	EXPECT_EQ( 0u, display->mSecondMode & ATTR_WDUMMY );
}

UTEST( eterm, ascii_respects_vt100_graphics_charset ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	term->write( "\033(0q", 4 );
	term->update();
	term->selstart( 0, 0, 0 );
	term->selextend( 0, 0, 1, 0 );

	EXPECT_STDSTREQ( "─", term->getSelection() );
}

UTEST( eterm, selection_reflow ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	// 80x24. Write 80 'A's then 80 'B's.
	std::string row0( 80, 'A' );
	std::string row1( 80, 'B' );
	term->write( row0.c_str(), row0.size() );
	term->write( row1.c_str(), row1.size() );
	term->write( " ", 1 ); // Trigger wrap on row 1 to move cursor to row 2 and preserve row 0 wrap
	term->update();

	// Selection from index 70 of row 0 to index 10 of row 1.
	term->selstart( 70, 0, 0 );
	term->selextend( 10, 1, 1, 0 );

	// ATTR_WRAP is set on row 0, so no newline should be added between A and B.
	std::string expected = std::string( 10, 'A' ) + std::string( 11, 'B' );
	std::string sel = term->getSelection();
	EXPECT_STDSTREQ( expected, sel );

	// Resize to 40 columns
	term->resize( 40, 24 );

	EXPECT_TRUE( term->hasSelection() );
	EXPECT_STDSTREQ( expected, term->getSelection() );
}

UTEST( eterm, selection_reflow_history ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	// Fill history with unique lines, each 40 chars to ensure they fit in 80.
	for ( int i = 0; i < 40; ++i ) {
		std::string line = "H" + std::to_string( i ) + " " + std::string( 30, 'x' ) + "\n";
		term->write( line.c_str(), line.size() );
		term->update();
	}

	// 40 lines total. 24 on screen. 16 in history.
	// Let's select Line 30 (which is on screen)
	// Row 0 is Line 16. Row 14 is Line 30.
	term->selstart( 0, 14, 0 );
	term->selextend( 1, 14, 1, 0 );

	std::string sel = term->getSelection();
	EXPECT_FALSE( sel.empty() );

	term->resize( 40, 24 );

	EXPECT_TRUE( term->hasSelection() );
	EXPECT_STDSTREQ( sel, term->getSelection() );
}

UTEST( eterm, selection_rectangular ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	term->write( "Line 1: ABCDEFG\r\n", 17 );
	term->write( "Line 2: HIJKLMN\r\n", 17 );
	term->write( "Line 3: OPQRSTU\r\n", 17 );
	term->update();

	// Select "ABC", "HIJ", "OPQ" area
	// "Line 1: " is 8 chars. A is at col 8.
	term->selstart( 8, 0, 0 );
	term->selextend( 10, 2, 2, 0 ); // Type 2 = SEL_RECTANGULAR

	std::string sel = term->getSelection();
	EXPECT_STDSTREQ( "ABC\nHIJ\nOPQ", sel );
}

UTEST( eterm, selection_reverse ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	term->write( "Line 1\r\nLine 2\r\nLine 3", 22 );
	term->update();

	// Select from Line 3 to Line 1
	term->selstart( 5, 2, 0 );
	term->selextend( 0, 0, 1, 0 );

	EXPECT_STDSTREQ( "Line 1\nLine 2\nLine 3", term->getSelection() );
}

UTEST( eterm, selection_wrap ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	// Terminal is 80x24.
	std::string longLine( 80, 'A' );
	longLine += "BBBB";
	term->write( longLine.c_str(), longLine.size() );
	term->update();

	// Selection should not have a newline at the wrap point
	term->selstart( 78, 0, 0 );
	term->selextend( 2, 1, 1, 0 );

	std::string sel = term->getSelection();
	EXPECT_STDSTREQ( "AABBB", sel );
}

UTEST( eterm, selection_snap_word ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	term->write( "Hello World Test", 16 );
	term->update();

	// Snap to "World"
	// World starts at index 6
	term->selstart( 7, 0, 1 ); // Type 1 = SNAP_WORD
	term->selextend( 7, 0, 1, 0 );

	EXPECT_STDSTREQ( "World", term->getSelection() );
}

UTEST( eterm, selection_snap_line ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	term->write( "Line 1\r\nLine 2\r\nLine 3", 22 );
	term->update();

	// Snap to Line 2
	term->selstart( 2, 1, 2 ); // Type 2 = SNAP_LINE
	term->selextend( 2, 1, 1, 0 );

	EXPECT_STDSTREQ( "Line 2\n", term->getSelection() );
}

UTEST( eterm, selection_alt_screen ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	term->write( "Main Screen", 11 );
	term->update();

	// Switch to alt screen and reset cursor position to (0,0)
	term->write( "\033[?1049h\033[H", 11 );
	term->update();

	term->write( "Alt Screen", 10 );
	term->update();

	term->selstart( 0, 0, 0 );
	term->selextend( 2, 0, 1, 0 );
	EXPECT_STDSTREQ( "Alt", term->getSelection() );

	// Switch back to main
	term->write( "\033[?1049l", 8 );
	term->update();

	// Selection should be cleared or at least not "Alt"
	EXPECT_FALSE( term->hasSelection() );
}

UTEST( eterm, selection_scrolling ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	term->write( "Target Line\r\n", 13 );
	term->update();

	// Select "Target"
	term->selstart( 0, 0, 0 );
	term->selextend( 5, 0, 1, 0 );
	EXPECT_STDSTREQ( "Target", term->getSelection() );

	// Push it into history by writing 30 lines
	for ( int i = 0; i < 30; ++i ) {
		term->write( "New Line\r\n", 10 );
	}
	term->update();

	// Selection should have moved with the text
	EXPECT_TRUE( term->hasSelection() );
	EXPECT_STDSTREQ( "Target", term->getSelection() );
}

UTEST( eterm, selection_tabs ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	// Default tab stop is 4
	term->write( "A\tB", 3 );
	term->update();

	// Select A[tab]B
	// A is at 0, tab is at 1,2,3, B is at 4
	term->selstart( 0, 0, 0 );
	term->selextend( 4, 0, 1, 0 );

	std::string sel = term->getSelection();
	EXPECT_STDSTREQ( "A   B", sel );
}

UTEST( eterm, selection_unicode ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	// Write some UTF-8 text: "Héllo Wörld"
	// é is C3 A9, ö is C3 B6
	term->write( "H\xC3\xA9llo W\xC3\xB6rld", 13 );
	term->update();

	term->selstart( 0, 0, 0 );
	term->selextend( 10, 0, 1, 0 ); // Select "Héllo Wörld"

	EXPECT_STDSTREQ( "Héllo Wörld", term->getSelection() );
}

UTEST( eterm, selection_wide_chars ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	// Unicode Emoji is often wide (2 columns)
	// Rocket 🚀 is F0 9F 9A 80
	term->write( "A\xF0\x9F\x9A\x80Z", 6 );
	term->update();

	// A is at 0, 🚀 is at 1-2, Z is at 3
	term->selstart( 0, 0, 0 );
	term->selextend( 3, 0, 1, 0 );

	EXPECT_STDSTREQ( "A🚀Z", term->getSelection() );

	// Test selection starting/ending in the middle of a wide char
	term->selstart( 1, 0, 0 );	   // Start at first half of rocket
	term->selextend( 2, 0, 1, 0 ); // End at second half
	EXPECT_STDSTREQ( "🚀", term->getSelection() );

	term->selstart( 1, 0, 0 );
	term->selextend( 1, 0, 1, 0 );
	EXPECT_STDSTREQ( "🚀", term->getSelection() );
}

UTEST( eterm, selection_reflow_extreme ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	// Initial 80x24. Write a long line.
	std::string text = "A VERY LONG LINE THAT WILL BE REFLOWED TO A NARROW TERMINAL";
	term->write( text.c_str(), text.size() );
	term->update();

	// Select "REFLOWED"
	// text[30] to text[37]
	term->selstart( 30, 0, 0 );
	term->selextend( 37, 0, 1, 0 );
	EXPECT_STDSTREQ( "REFLOWED", term->getSelection() );

	// Shrink to 5 columns
	term->resize( 5, 24 );

	// REFLOWED should still be selected
	EXPECT_TRUE( term->hasSelection() );
	EXPECT_STDSTREQ( "REFLOWED", term->getSelection() );

	// Expand back to 80 columns
	term->resize( 80, 24 );
	EXPECT_STDSTREQ( "REFLOWED", term->getSelection() );
}

UTEST( eterm, selection_clear_screen ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	term->write( "Test text", 9 );
	term->update();

	term->selstart( 0, 0, 0 );
	term->selextend( 3, 0, 1, 0 );
	EXPECT_TRUE( term->hasSelection() );

	// CSI 2 J - Clear Screen
	term->write( "\033[2J", 4 );
	term->update();

	EXPECT_FALSE( term->hasSelection() );
}

UTEST( eterm, selection_scroll_region ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	// Initial 80x24. Fill with some text.
	for ( int i = 0; i < 10; ++i ) {
		std::string line = "Line " + std::to_string( i ) + "\r\n";
		term->write( line.c_str(), line.size() );
	}
	term->update();

	// Select "Line 5" at Row 5.
	term->selstart( 0, 5, 0 );
	term->selextend( 5, 5, 1, 0 );
	EXPECT_STDSTREQ( "Line 5", term->getSelection() );

	// Set scrolling region: 3rd row to 8th row. (1-indexed CSI r)
	term->write( "\033[3;8r", 6 );
	// Move cursor to 8th row (bottom of scroll region)
	term->write( "\033[8;1H", 6 );
	// Write 2 more lines to push Row 5 up by 2 within the region.
	term->write( "Push 1\nPush 2\n", 14 );
	term->update();

	// Line 5 was at Row 5. Within [3,8], it should move to Row 3.
	// However, if it moves out of the region or something weird happens?
	// Let's check where it is.
	// Actually, tscrollup(top, n, copyhist) is used.
	// In our case top=2, bot=7. n=2.
	// Row 5 should move to 5-2 = 3.
	EXPECT_STDSTREQ( "Line 5", term->getSelection() );
}

UTEST( eterm, selection_trailing_spaces ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	// Terminal is 80 columns.
	// Write "Hello   " (3 spaces) then newline.
	term->write( "Hello   \r\nWorld", 15 );
	term->update();

	// Select both lines.
	term->selstart( 0, 0, 0 );
	term->selextend( 4, 1, 1, 0 ); // "Hello" to "World"

	// Trailing spaces on the first line should be stripped because it's not wrapped.
	EXPECT_STDSTREQ( "Hello\nWorld", term->getSelection() );

	// Now test with WRAPPED line.
	// Row 1 has "World" (5 chars).
	// Write 73 'A's and 2 spaces to reach 80 chars.
	std::string fill( 73, 'A' );
	term->write( fill.c_str(), fill.size() );
	term->write( "  ", 2 ); // Row 1 is now 80 chars: "World" + fill + "  "
	term->write( "BB", 2 ); // This forces a wrap. Row 2 will be "BB".
	term->update();

	// Select Row 1 and Row 2.
	// Row 1 starts at Col 0, Row 1. Row 2 starts at Col 0, Row 2.
	term->selstart( 0, 1, 0 );
	term->selextend( 1, 2, 1, 0 ); // From "World" to "BB"

	// The spaces at the end of Row 1 should be preserved because it wrapped.
	// "World" + fill + "  " + "BB"
	std::string expected = "World" + fill + "  BB";
	EXPECT_STDSTREQ( expected, term->getSelection() );
}

UTEST( eterm, selection_word_snap_unicode ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	// Write "Héllo-Wörld"
	// Word delimiters are only space ' ' and null 0 in current implementation.
	// So "Héllo-Wörld" should be one word.
	term->write( "H\xC3\xA9llo-W\xC3\xB6rld", 14 );
	term->update();

	// Snap to word starting at "ll"
	term->selstart( 2, 0, 1 ); // Index 2 is 'l'
	term->selextend( 2, 0, 1, 0 );

	EXPECT_STDSTREQ( "Héllo-Wörld", term->getSelection() );

	// Write "Test Wörld"
	term->write( "\r\nTest W\xC3\xB6rld", 13 );
	term->update();

	// Snap to "Wörld"
	term->selstart( 6, 1, 1 ); // index 6 is 'W'
	term->selextend( 6, 1, 1, 0 );
	EXPECT_STDSTREQ( "Wörld", term->getSelection() );
}

UTEST( eterm, selection_history_screen_boundary ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	term->resize( 80, 5 ); // 5 rows terminal

	// Write 5 lines.
	for ( int i = 0; i < 5; ++i ) {
		std::string line = "Line" + std::to_string( i ) + "\r\n";
		term->write( line.c_str(), line.size() );
	}
	term->update();

	// Now screen has Row 0 empty, Row 1 empty... Row 4 empty?
	// Let's check. 5 rows: 0, 1, 2, 3, 4.
	// Line0\r\n -> cursor at Row 1.
	// Line1\r\n -> cursor at Row 2.
	// Line2\r\n -> cursor at Row 3.
	// Line3\r\n -> cursor at Row 4.
	// Line4\r\n -> cursor at Row 5 -> scroll up.
	// Row 0 has Line1, Row 1 has Line2, Row 2 has Line3, Row 3 has Line4.
	// Row 4 is empty. History has Line0.

	// Let's select Line1 (Row 0) to Line4 (Row 3).
	term->selstart( 0, 0, 0 );
	term->selextend( 4, 3, 1, 0 );
	EXPECT_TRUE( term->hasSelection() );

	// Scroll down 2 more lines.
	term->write( "New1\r\nNew2\r\n", 12 );
	term->update();

	// Selection should have moved to history.
	// Line1 was at Row 0, moved up by 2 -> Row -2.
	// Line4 was at Row 3, moved up by 2 -> Row 1.
	EXPECT_TRUE( term->hasSelection() );
	std::string sel = term->getSelection();
	EXPECT_TRUE( sel.find( "Line1" ) != std::string::npos );
	EXPECT_TRUE( sel.find( "Line4" ) != std::string::npos );
}

UTEST( eterm, selection_basic_history ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	term->resize( 80, 2 ); // 2 rows terminal: Row 0 and Row 1.

	term->write( "Line0\r\n", 7 );
	term->write( "Line1\r\n", 7 );
	term->write( "Line2", 5 );
	term->update();

	// Line0 is at Row -1 (history)
	// Line1 is at Row 0 (screen)
	// Line2 is at Row 1 (screen)

	// Select Line0 (history) and Line1 (screen).
	term->selstart( 0, -1, 0 );
	term->selextend( 4, 0, 1, 0 );

	EXPECT_TRUE( term->hasSelection() );
	std::string sel = term->getSelection();
	EXPECT_TRUE( sel.find( "Line0" ) != std::string::npos );
	EXPECT_TRUE( sel.find( "Line1" ) != std::string::npos );
	EXPECT_TRUE( sel.find( "Line2" ) == std::string::npos );
}

UTEST( eterm, selection_rectangular_reflow ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	// Initial 80x24.
	term->write( "ABCDE\r\n", 7 );
	term->write( "FGHIJ\r\n", 7 );
	term->update();

	// Select BC and GH (Rectangular)
	// BC is at (1,0) to (2,0)
	// GH is at (1,1) to (2,1)
	term->selstart( 1, 0, 0 );
	term->selextend( 2, 1, 2, 0 ); // type 2 = Rectangular

	EXPECT_STDSTREQ( "BC\nGH", term->getSelection() );

	// Resize to 5 columns.
	term->resize( 5, 24 );

	// Rectangular selections should be preserved.
	EXPECT_TRUE( term->hasSelection() );
	EXPECT_STDSTREQ( "BC\nGH", term->getSelection() );
}

UTEST( eterm, selection_rectangular_resize_no_reflow ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	// Initial 80x24.
	term->write( "ABCDE\r\n", 7 );
	term->write( "FGHIJ\r\n", 7 );
	term->update();

	// Select BC and GH (Rectangular)
	term->selstart( 1, 0, 0 );
	term->selextend( 2, 1, 2, 0 ); // type 2 = Rectangular

	EXPECT_STDSTREQ( "BC\nGH", term->getSelection() );

	// Resize to 90 columns (wider, no reflow needed)
	term->resize( 90, 24 );

	// It should still be selected
	EXPECT_TRUE( term->hasSelection() );
	EXPECT_STDSTREQ( "BC\nGH", term->getSelection() );
}

UTEST( eterm, scroll_position_after_resize ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	// Initial 80x24. Fill history with enough lines to allow scrolling.
	for ( int i = 0; i < 40; ++i ) {
		std::string line = "Line " + std::to_string( i ) + "\r\n";
		term->write( line.c_str(), line.size() );
		term->update();
	}

	// Scroll up by 5 lines.
	TerminalArg arg;
	arg.i = 5;
	term->kscrollup( &arg );

	int expected_scroll = term->scrollPos();
	EXPECT_EQ( 5, expected_scroll );

	// Resize to something wider, meaning no reflow height change.
	term->resize( 90, 24 );

	// Scroll position should be preserved.
	EXPECT_EQ( 5, term->scrollPos() );
}

UTEST( eterm, scroll_position_after_ttyread ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	// Initial 80x24. Fill history with enough lines to allow scrolling.
	for ( int i = 0; i < 40; ++i ) {
		std::string line = "Line " + std::to_string( i ) + "\r\n";
		term->write( line.c_str(), line.size() );
		term->update();
	}

	// Check initial state
	EXPECT_EQ( 0, term->scrollPos() );

	// Scroll up by 5 lines.
	TerminalArg arg;
	arg.i = 5;
	term->kscrollup( &arg );
	EXPECT_EQ( 5, term->scrollPos() );

	// Write a carriage return and new line, which should add to history and push screen up.
	term->write( "New output\r\n", 12 );
	term->update();

	// The scroll position should be updated to maintain the viewport, so it should be 6.
	EXPECT_EQ( 6, term->scrollPos() );

	// Let's verify that the history content hasn't been corrupted.
	// If mTerm.scr was > 0 during twrite, and it wasn't handled correctly,
	// "New output" might have overwritten a history line instead of the screen bottom.

	// We added 40 lines (Line 0 to Line 39). Plus "New output". Total 41 lines.
	// Screen holds 24 lines.
	// Line 39 is on row 22. Row 23 is empty.
	// We scroll up 5 lines. So we are looking at 5 lines back.
	// Visual row 23 was "Line 35".
	// After adding "New output\r\n", 1 line is pushed to history.
	// The scroll position increases to 6, maintaining the viewport.
	// So visual row 23 should still be "Line 35".
	term->selstart( 0, 23, 0 );
	term->selextend( 6, 23, 1, 0 );
	EXPECT_STDSTREQ( "Line 35", term->getSelection() );

	// Scroll down to 0 to verify "New output" is indeed at the bottom.
	TerminalArg arg_down;
	arg_down.i = 6;
	term->kscrolldown( &arg_down );
	EXPECT_EQ( 0, term->scrollPos() );

	term->selstart( 0, 22, 0 ); // "New output" is at row 22 because of \r\n
	term->selextend( 9, 22, 1, 0 );
	EXPECT_STDSTREQ( "New output", term->getSelection() );
}

UTEST( eterm, ttyread_keeps_scrolled_selection_attached_to_text ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	for ( int i = 0; i < 40; ++i ) {
		std::string line = "Line " + std::to_string( i ) + "\r\n";
		term->write( line.c_str(), line.size() );
		term->update();
	}

	TerminalArg scroll( 5 );
	term->kscrollup( &scroll );
	term->selstart( 0, 23, 0 );
	term->selextend( 6, 23, SEL_REGULAR, false );
	ASSERT_STDSTREQ( "Line 35", term->getSelection() );

	term->write( "New output\r\n", 12 );
	term->update();

	EXPECT_EQ( 6, term->scrollPos() );
	EXPECT_STDSTREQ( "Line 35", term->getSelection() );
}

UTEST( eterm, ttyread_restores_viewport_after_history_ring_wrap ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 4 );

	for ( int i = 0; i < 28; ++i ) {
		std::string line = "Line " + std::to_string( i ) + "\r\n";
		term->write( line.c_str(), line.size() );
		term->update();
	}

	TerminalArg scroll( 2 );
	term->kscrollup( &scroll );
	ASSERT_EQ( 2, term->scrollPos() );

	// One PTY read pushes exactly histsize rows, wrapping histi back to its original index.
	// The viewport must still account for all four pushed rows and clamp to the oldest history.
	const char burst[] = "Burst 0\r\nBurst 1\r\nBurst 2\r\nBurst 3\r\n";
	term->write( burst, sizeof( burst ) - 1 );
	term->update();
	EXPECT_EQ( 4, term->scrollPos() );

	TerminalArg bottom( INT_MAX );
	term->kscrolldown( &bottom );
	term->selstart( 0, 22, 0 );
	term->selextend( 6, 22, SEL_REGULAR, false );
	EXPECT_STDSTREQ( "Burst 3", term->getSelection() );
}

UTEST( eterm, absolute_scrolling_keeps_selection_attached_to_text ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	for ( int i = 0; i < 40; ++i ) {
		std::string line = "Line " + std::to_string( i ) + "\r\n";
		term->write( line.c_str(), line.size() );
		term->update();
	}

	TerminalArg scroll( 5 );
	term->kscrollto( &scroll );
	term->selstart( 0, 23, 0 );
	term->selextend( 6, 23, SEL_REGULAR, false );
	ASSERT_STDSTREQ( "Line 35", term->getSelection() );

	scroll.i = 10;
	term->kscrollto( &scroll );
	EXPECT_STDSTREQ( "Line 35", term->getSelection() );

	scroll.i = 2;
	term->kscrollto( &scroll );
	EXPECT_STDSTREQ( "Line 35", term->getSelection() );
}

UTEST( eterm, history_corruption_on_resize ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 1000 );

	std::vector<std::string> expected_lines;

	// Initial 80x24. Fill history with enough lines to allow scrolling.
	for ( int i = 0; i < 500; ++i ) {
		std::string line = "Line " + std::to_string( i );
		expected_lines.push_back( line );
		std::string write_str = line + "\r\n";
		term->write( write_str.c_str(), write_str.size() );
		term->update();
	}

	// Check initial state
	EXPECT_EQ( 0, term->scrollPos() );

	// Scroll up to center. (500 lines total. 24 on screen. history is 476. scroll 250)
	TerminalArg arg;
	arg.i = 250;
	term->kscrollup( &arg );
	EXPECT_EQ( 250, term->scrollPos() );

	// Expand the terminal quite a bit (from 24 rows to 50 rows)
	term->resize( 80, 50 );

	// Let's verify EVERY SINGLE LINE in the buffer.
	arg.i = 1000;
	term->kscrolldown( &arg ); // Scroll to bottom
	EXPECT_EQ( 0, term->scrollPos() );

	int history_len = term->scrollSize();
	int screen_len = term->rowCount();
	int total_len = history_len + screen_len;

	int start_idx = 500 - total_len;
	if ( start_idx < 0 )
		start_idx = 0; // If some lines are empty

	for ( int y = 0; y < total_len; ++y ) {
		int expected_idx = start_idx + y;
		if ( expected_idx >= 500 )
			continue; // Don't check empty lines at the very bottom

		term->selstart( 0, y - history_len, 0 );
		term->selextend( 80, y - history_len, 1, 0 ); // Need to select entire row up to length

		std::string sel = term->getSelection();
		// Remove trailing newlines/spaces
		while ( !sel.empty() &&
				( sel.back() == '\n' || sel.back() == '\r' || sel.back() == ' ' ) ) {
			sel.pop_back();
		}

		EXPECT_STDSTREQ( expected_lines[expected_idx], sel );
	}
}

UTEST( eterm, repeated_resize_keeps_screen_row_ownership ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 1000 );

	term->resize( 151, 50 );
	for ( int line = 0; line < 200; ++line ) {
		const std::string text = "terminal output " + std::to_string( line ) + "\r\n";
		term->write( text.c_str(), text.size() );
		term->update();
	}
	for ( int iteration = 0; iteration < 100; ++iteration ) {
		term->resize( 151, 52 );
		term->resize( 151, 50 );
	}
	const Vector2i finalSize = term->getSize();
	EXPECT_EQ( 151, finalSize.x );
	EXPECT_EQ( 50, finalSize.y );
	EXPECT_TRUE( term->scrollSize() > 0 );
}

UTEST( eterm, history_capacity_tracks_narrow_and_wide_resize ) {
	auto pty = std::make_unique<MockPty>();
	pty->mCols = 151;
	pty->mRows = 50;
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 10 );

	for ( int line = 0; line < 100; ++line ) {
		const std::string text = "short " + std::to_string( line ) + "\r\n";
		term->write( text.c_str(), text.size() );
		term->update();
	}
	EXPECT_TRUE( term->scrollSize() > 0 );

	term->resize( 80, 50 );
	term->resize( 151, 50 );
	const Vector2i finalSize = term->getSize();
	EXPECT_EQ( 151, finalSize.x );
	EXPECT_EQ( 50, finalSize.y );
}

UTEST( eterm_search, logical_lines_options_and_cell_mapping ) {
	TerminalGlyph first[] = { { 'H' }, { 'e' }, { 'l' }, { 'l' }, { 'o', ATTR_WRAP } };
	TerminalGlyph second[] = { { 'W' }, { 'o' }, { 'r' }, { 'l' }, { 'd' } };
	TerminalGlyph wide[] = { { 0x754C, ATTR_WIDE }, { 0, ATTR_WDUMMY }, { '!' } };
	std::vector<TerminalSearchRowView> rows{
		{ first, TerminalBufferSource::MainHistory, -1, 5, 5, true },
		{ second, TerminalBufferSource::MainScreen, 0, 5, 5, false },
		{ wide, TerminalBufferSource::MainScreen, 1, 3, 3, false },
	};
	TerminalSearch search;
	EXPECT_TRUE( search.search( rows, { "o", 0, true, false } ).empty() );

	auto matches = search.search( rows, { "lowo", 1, false, false } );
	ASSERT_EQ( static_cast<size_t>( 1 ), matches.size() );
	EXPECT_EQ( static_cast<Int64>( -1 ), matches[0].start.row );
	EXPECT_EQ( 3, matches[0].start.column );
	EXPECT_EQ( static_cast<Int64>( 0 ), matches[0].end.row );
	EXPECT_EQ( 1, matches[0].end.column );

	EXPECT_TRUE( search.search( rows, { "hello", 2, true, true } ).empty() );
	EXPECT_EQ( static_cast<size_t>( 1 ),
			   search.search( rows, { "HELLOWORLD", 3, false, true } ).size() );

	String unicodeQuery;
	unicodeQuery += static_cast<String::StringBaseType>( 0x754C );
	unicodeQuery += '!';
	matches = search.search( rows, { unicodeQuery, 4, true, false } );
	ASSERT_EQ( static_cast<size_t>( 1 ), matches.size() );
	EXPECT_EQ( 0, matches[0].start.column );
	EXPECT_EQ( 2, matches[0].end.column );

	matches = search.search( rows, { "l+oW.rld", 5, true, false, TerminalSearchType::RegEx } );
	ASSERT_EQ( static_cast<size_t>( 1 ), matches.size() );
	EXPECT_EQ( 2, matches[0].start.column );
	EXPECT_EQ( static_cast<Int64>( 0 ), matches[0].end.row );
	EXPECT_EQ( 4, matches[0].end.column );

	matches = search.search( rows, { "界!", 6, true, false, TerminalSearchType::RegEx } );
	ASSERT_EQ( static_cast<size_t>( 1 ), matches.size() );
	EXPECT_EQ( 0, matches[0].start.column );
	EXPECT_EQ( 2, matches[0].end.column );

	matches = search.search( rows, { "[Ww]%a+d", 7, true, false, TerminalSearchType::LuaPattern } );
	ASSERT_EQ( static_cast<size_t>( 1 ), matches.size() );
	EXPECT_EQ( 0, matches[0].start.column );
	EXPECT_EQ( 4, matches[0].end.column );

	EXPECT_TRUE(
		search.search( rows, { "[", 8, true, false, TerminalSearchType::RegEx } ).empty() );
	EXPECT_EQ( static_cast<size_t>( 1 ),
			   search.search( rows, { "helloworld", 9, false, false, TerminalSearchType::RegEx } )
				   .size() );
}

UTEST( eterm_search, emulator_history_navigation_and_clear ) {
	auto pty = std::make_unique<MockPty>();
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );
	ASSERT_TRUE( term != nullptr );

	for ( int line = 0; line < 40; ++line ) {
		const std::string text = "line " + std::to_string( line ) + " needle\r\n";
		term->write( text.data(), text.size() );
		term->update();
	}
	term->setSearchQuery( { "n", 41, true, false } );
	EXPECT_TRUE( term->getSearchMatches().empty() );
	EXPECT_EQ( -1, term->getCurrentSearchMatch() );
	EXPECT_EQ( static_cast<Uint64>( 41 ), term->getSearchRequestId() );

	term->setSearchQuery( { "needle", 42, true, true } );
	ASSERT_EQ( static_cast<size_t>( 40 ), term->getSearchMatches().size() );
	EXPECT_EQ( 0, term->getCurrentSearchMatch() );
	EXPECT_TRUE( term->scrollPos() > 0 );

	term->navigateSearch( -1 );
	EXPECT_EQ( 39, term->getCurrentSearchMatch() );
	EXPECT_EQ( 0, term->scrollPos() );
	term->navigateSearch( 1 );
	EXPECT_EQ( 0, term->getCurrentSearchMatch() );
	term->clearSearch();
	EXPECT_TRUE( term->getSearchMatches().empty() );
	EXPECT_EQ( -1, term->getCurrentSearchMatch() );
}
