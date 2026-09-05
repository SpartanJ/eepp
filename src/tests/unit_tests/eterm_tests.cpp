#include "utest.hpp"
#include <atomic>
#include <chrono>
#include <deque>
#include <eepp/system/base64.hpp>
#include <eepp/system/compression.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eterm/system/iprocess.hpp>
#include <eterm/terminal/ipseudoterminal.hpp>
#include <eterm/terminal/iterminaldisplay.hpp>
#include <eterm/terminal/terminalemulator.hpp>
#include <eterm/terminal/terminalgraphics.hpp>
#include <eterm/terminal/terminalsession.hpp>
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
	int mDrawEnds{ 0 };
	uint32_t mFirstMode{ 0 };
	uint32_t mSecondMode{ 0 };
	TerminalGlyph mFirstGlyph;
	TerminalGlyph mSecondGlyph;
	std::vector<Uint32> mResetColorIndices;
	int mResetColorsCount{ 0 };
	Uint32 mBackground{ 0x101010FF };
	std::shared_ptr<TerminalGraphicsPresentation> mGraphics;
	bool drawBegin( Uint32, Uint32 ) override { return true; }
	void drawLine( Line line, int, int y, int ) override {
		++mDrawLines;
		if ( y == 0 ) {
			mFirstMode = line[0].mode;
			mSecondMode = line[1].mode;
			mFirstGlyph = line[0];
			mSecondGlyph = line[1];
		}
	}
	void drawCursor( int, int, TerminalGlyph, int, int, TerminalGlyph ) override {}
	void drawEnd() override { ++mDrawEnds; }
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

	EXPECT_STDSTREQ( "\033[<0;21;31M", ptyPtr->mWrites );
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
