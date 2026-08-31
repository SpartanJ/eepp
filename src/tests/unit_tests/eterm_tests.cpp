#include "utest.hpp"
#include <atomic>
#include <chrono>
#include <eterm/system/iprocess.hpp>
#include <eterm/terminal/ipseudoterminal.hpp>
#include <eterm/terminal/iterminaldisplay.hpp>
#include <eterm/terminal/terminalemulator.hpp>
#include <eterm/terminal/terminalsession.hpp>
#include <limits>
#include <thread>

using namespace eterm::Terminal;
using namespace eterm::System;

class MockPty : public IPseudoTerminal {
  public:
	std::string mBuffer;
	std::string mWrites;
	bool mLoopWrites{ true };
	size_t mMaxRead{ std::numeric_limits<size_t>::max() };
	size_t mReadOffset{ 0 };
	std::atomic<size_t> mBytesRead{ 0 };
	int mCols = 80;
	int mRows = 24;
	int getNumColumns() const override { return mCols; }
	int getNumRows() const override { return mRows; }
	bool resize( int columns, int rows ) override {
		mCols = columns;
		mRows = rows;
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
		size_t toRead = std::min( { n, mBuffer.size() - mReadOffset, mMaxRead } );
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
	pty->mLoopWrites = false;
	MockPty* ptyPtr = pty.get();
	auto process = std::make_unique<MockProcess>();
	auto session = TerminalSession::create( std::move( pty ), std::move( process ), 100 );
	auto enabled = waitForSnapshot( session, []( const TerminalSnapshot& snapshot ) {
		return snapshot.windowMode & MODE_FOCUS;
	} );
	ASSERT_TRUE( enabled != nullptr );

	session->setFocus( false );
	auto unfocused = waitForSnapshot( session, [enabled]( const TerminalSnapshot& snapshot ) {
		return snapshot.generation > enabled->generation && !( snapshot.windowMode & MODE_FOCUSED );
	} );
	ASSERT_TRUE( unfocused != nullptr );
	ASSERT_TRUE( ptyPtr->mWrites.size() >= 3 );
	EXPECT_STDSTREQ( "\033[O", ptyPtr->mWrites.substr( ptyPtr->mWrites.size() - 3 ) );

	session->setFocus( true );
	ASSERT_TRUE( waitForSnapshot( session, [unfocused]( const TerminalSnapshot& snapshot ) {
					 return snapshot.generation > unfocused->generation &&
							snapshot.windowMode & MODE_FOCUSED;
				 } ) != nullptr );
	ASSERT_TRUE( ptyPtr->mWrites.size() >= 3 );
	EXPECT_STDSTREQ( "\033[I", ptyPtr->mWrites.substr( ptyPtr->mWrites.size() - 3 ) );
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
	int mDrawLines{ 0 };
	uint32_t mFirstMode{ 0 };
	uint32_t mSecondMode{ 0 };
	bool drawBegin( Uint32, Uint32 ) override { return true; }
	void drawLine( Line line, int, int, int ) override {
		++mDrawLines;
		mFirstMode = line[0].mode;
		mSecondMode = line[1].mode;
	}
	void drawCursor( int, int, TerminalGlyph, int, int, TerminalGlyph ) override {}
	void drawEnd() override {}
};

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
	auto pty = std::make_unique<MockPty>();
	pty->mMaxRead = 1;
	auto process = std::make_unique<MockProcess>();
	auto display = std::make_shared<MockDisplay>();
	auto term = TerminalEmulator::create( std::move( pty ), std::move( process ), display, 100 );

	std::string output( 1024, 'A' );
	term->write( output.data(), output.size() );
	display->mDrawLines = 0;

	EXPECT_FALSE( term->update() );
	EXPECT_EQ( 0, display->mDrawLines );
	EXPECT_TRUE( term->update() );
	EXPECT_TRUE( display->mDrawLines > 0 );
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
