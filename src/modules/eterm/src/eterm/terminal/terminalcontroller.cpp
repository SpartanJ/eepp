#include <eterm/terminal/terminalcontroller.hpp>

#include <eepp/system/color.hpp>
#include <eepp/system/log.hpp>

#include <algorithm>
#include <cstring>

using namespace EE::System;

namespace eterm { namespace Terminal {

struct TerminalController::SelectionResponse {
	std::mutex mutex;
	std::condition_variable condition;
	std::string selection;
	bool ready{ false };
};

class TerminalController::WorkerDisplay final : public ITerminalDisplay {
  public:
	WorkerDisplay( TerminalController& controller, TerminalColorPalette palette ) :
		mController( controller ),
		mInitialPalette( std::move( palette ) ),
		mPalette( mInitialPalette ) {
		mMode |= MODE_FOCUSED;
	}

	bool drawBegin( Uint32 columns, Uint32 rows ) {
		const size_t cellCount = static_cast<size_t>( columns ) * rows;
		if ( columns != static_cast<Uint32>( mColumns ) || rows != static_cast<Uint32>( mRows ) ) {
			mColumns = columns;
			mRows = rows;
			mCells.assign( cellCount, TerminalGlyph{} );
			mDirtyRows.assign( rows, 1 );
		} else {
			mDirtyRows.assign( rows, 0 );
		}
		mCursorVisible = false;
		return getMode( MODE_VISIBLE );
	}

	void drawLine( Line line, int x1, int y, int x2 ) {
		if ( y < 0 || y >= mRows || x1 < 0 || x2 > mColumns || x1 >= x2 )
			return;
		TerminalGlyph* destination = mCells.data() + static_cast<size_t>( y ) * mColumns + x1;
		std::memcpy( destination, line + x1, static_cast<size_t>( x2 - x1 ) * sizeof( *line ) );
		if ( mEmulator ) {
			for ( int column = x1; column < x2; ++column ) {
				if ( mEmulator->isSelected( column, y ) )
					mCells[static_cast<size_t>( y ) * mColumns + column].mode |= ATTR_REVERSE;
			}
		}
		mDirtyRows[y] = 1;
	}

	void drawCursor( int cx, int cy, TerminalGlyph glyph, int, int, TerminalGlyph ) {
		mCursor = { cx, cy };
		mCursorGlyph = glyph;
		mCursorVisible = true;
	}

	void drawEnd() {
		auto snapshot = std::make_shared<TerminalSnapshot>();
		snapshot->cells = mCells;
		snapshot->dirtyRows = mDirtyRows;
		snapshot->title = mTitle;
		snapshot->generation = ++mGeneration;
		snapshot->lastAppliedScrollCommand = mLastAppliedScrollCommand;
		snapshot->cursor = mCursor;
		snapshot->cursorGlyph = mCursorGlyph;
		snapshot->columns = mColumns;
		snapshot->rows = mRows;
		snapshot->windowMode = mMode;
		snapshot->presentationRate = mPresentationRate;
		snapshot->cursorMode = mCursorMode;
		snapshot->cursorVisible = mCursorVisible;
		if ( mEmulator ) {
			snapshot->historyLength = mEmulator->scrollSize();
			snapshot->scrollPosition = mEmulator->scrollPos();
			snapshot->hasSelection = mEmulator->hasSelection();
			snapshot->selectionMode = mEmulator->getSelectionMode();
			snapshot->cursorSelected =
				snapshot->cursorVisible && mEmulator->isSelected( mCursor.x, mCursor.y );
			if ( snapshot->hasSelection )
				snapshot->selection = mEmulator->getSelection();
			snapshot->altScreen = mEmulator->tisaltscr();
			snapshot->processExited = mEmulator->hasExited();
			snapshot->exitCode = mEmulator->getExitCode();
			snapshot->currentWorkingDirectory = mEmulator->getCurrentWorkingDirectory();
			snapshot->promptState = mEmulator->getPromptState();
			if ( auto* process = mEmulator->getProcess() )
				snapshot->processId = process->pid();
		}
		if ( snapshot->historyLength != mLastHistoryLength ) {
			mLastHistoryLength = snapshot->historyLength;
			Event event{ EventType::HistoryLength };
			event.value = mLastHistoryLength;
			mController.enqueueEvent( std::move( event ), true );
		}
		mController.publishSnapshot( std::move( snapshot ) );
	}

	void bell() { mController.enqueueEvent( { EventType::Bell }, false ); }

	void resetColors() {
		mPalette = mInitialPalette;
		Event event{ EventType::Color };
		event.value = -1;
		mController.enqueueEvent( std::move( event ), false );
	}

	int resetColor( const Uint32& index, const char* name ) {
		Uint32 color = 0;
		bool parsed = false;
		if ( name && String::startsWith( name, "rgb:" ) ) {
			auto components = String::split( std::string( name + 4 ), '/' );
			if ( components.size() == 3 ) {
				char* ends[3]{};
				long rgb[3]{};
				for ( size_t i = 0; i < 3; ++i )
					rgb[i] = std::strtol( components[i].c_str(), &ends[i], 16 );
				if ( ends[0] && ends[1] && ends[2] ) {
					color = Color( rgb[0], rgb[1], rgb[2] ).getValue();
					parsed = true;
				}
			}
		} else if ( name && Color::isColorString( std::string_view{ name }, true ) ) {
			color = Color::fromString( name ).getValue();
			parsed = true;
		} else if ( !name || String::iequals( "default", name ) ) {
			if ( index < mInitialPalette.colors.size() ) {
				color = mInitialPalette.colors[index];
				parsed = true;
			} else if ( index == 256 || index == 257 ) {
				color = mInitialPalette.cursor;
				parsed = true;
			} else if ( index == 258 ) {
				color = mInitialPalette.foreground;
				parsed = true;
			} else if ( index == 259 ) {
				color = mInitialPalette.background;
				parsed = true;
			}
		}
		if ( !parsed )
			return 1;
		setPaletteColor( index, color );
		Event event{ EventType::Color };
		event.data = name ? name : "";
		event.value = static_cast<int>( index );
		mController.enqueueEvent( std::move( event ), false );
		return 0;
	}

	bool getColor( const Uint32& index, unsigned char* red, unsigned char* green,
				   unsigned char* blue ) {
		Uint32 color = 0;
		if ( index < mPalette.colors.size() )
			color = mPalette.colors[index];
		else if ( index == 256 || index == 257 )
			color = mPalette.cursor;
		else if ( index == 258 )
			color = mPalette.foreground;
		else if ( index == 259 )
			color = mPalette.background;
		else
			return false;
		*red = ( color >> 24 ) & 0xFF;
		*green = ( color >> 16 ) & 0xFF;
		*blue = ( color >> 8 ) & 0xFF;
		return true;
	}

	void setTitle( const char* title ) {
		mTitle = title ? title : "";
		mController.enqueueEvent( { EventType::Title, mTitle }, true );
	}

	void setIconTitle( const char* title ) {
		mController.enqueueEvent( { EventType::IconTitle, title ? title : "" }, true );
	}

	void setClipboard( const char* text ) {
		if ( text )
			mController.enqueueEvent( { EventType::Clipboard, text }, false );
	}

	void onProcessExit( int exitCode ) {
		Event event{ EventType::ProcessExit };
		event.value = exitCode;
		mController.enqueueEvent( std::move( event ), false );
	}

	void onScrollPositionChange() {
		mController.enqueueEvent( { EventType::ScrollPosition }, true );
	}

	void setPalette( TerminalColorPalette palette ) {
		mInitialPalette = palette;
		mPalette = std::move( palette );
	}

	void setFocused( bool focused ) {
		if ( focused )
			mMode |= MODE_FOCUSED;
		else
			mMode &= ~MODE_FOCUSED;
	}

	void setLastAppliedScrollCommand( Uint64 commandId ) { mLastAppliedScrollCommand = commandId; }

	void setPresentationRate( Uint32 framesPerSecond ) { mPresentationRate = framesPerSecond; }

  private:
	void setPaletteColor( Uint32 index, Uint32 color ) {
		if ( index < mPalette.colors.size() )
			mPalette.colors[index] = color;
		else if ( index == 256 || index == 257 )
			mPalette.cursor = color;
		else if ( index == 258 )
			mPalette.foreground = color;
		else if ( index == 259 )
			mPalette.background = color;
	}

	TerminalController& mController;
	TerminalColorPalette mInitialPalette;
	TerminalColorPalette mPalette;
	std::vector<TerminalGlyph> mCells;
	std::vector<Uint8> mDirtyRows;
	std::string mTitle;
	Uint64 mGeneration{ 0 };
	Uint64 mLastAppliedScrollCommand{ 0 };
	Vector2i mCursor;
	TerminalGlyph mCursorGlyph;
	int mColumns{ 0 };
	int mRows{ 0 };
	int mLastHistoryLength{ -1 };
	Uint32 mPresentationRate{ 60 };
	bool mCursorVisible{ false };
};

std::shared_ptr<TerminalController> TerminalController::create( PtyPtr&& pty, ProcPtr&& process,
																size_t historySize,
																TerminalColorPalette palette ) {
	if ( !pty || !process )
		return nullptr;
	auto controller = std::shared_ptr<TerminalController>( new TerminalController(
		std::move( pty ), std::move( process ), historySize, std::move( palette ) ) );
	controller->start();
	return controller;
}

TerminalController::TerminalController( PtyPtr&& pty, ProcPtr&& process, size_t historySize,
										TerminalColorPalette palette ) {
	mWorkerDisplay = std::make_shared<WorkerDisplay>( *this, std::move( palette ) );
	mEmulator = TerminalEmulator::create( std::move( pty ), std::move( process ), mWorkerDisplay,
										  historySize );
	// Establish a complete generation before the controller becomes concurrently visible.
	mEmulator->redraw();
}

TerminalController::~TerminalController() {
	shutdown();
}

void TerminalController::start() {
	mWorker = std::thread( [this] { workerLoop(); } );
}

void TerminalController::shutdown() {
	std::lock_guard<std::mutex> shutdownLock( mShutdownMutex );
	if ( !mShutdownRequested.exchange( true, std::memory_order_acq_rel ) )
		mCommandCondition.notify_all();
	if ( mWorker.joinable() && mWorker.get_id() != std::this_thread::get_id() )
		mWorker.join();
}

void TerminalController::enqueue( Command&& command ) {
	if ( mShutdownRequested.load( std::memory_order_acquire ) )
		return;
	{
		std::lock_guard<std::mutex> lock( mCommandMutex );
		if ( mShutdownRequested.load( std::memory_order_relaxed ) )
			return;
		mCommands.emplace_back( std::move( command ) );
	}
	mCommandCondition.notify_one();
}

void TerminalController::write( std::string data, bool mayEcho ) {
	enqueue( WriteCommand{ std::move( data ), mayEcho } );
}

void TerminalController::writeRaw( std::string data ) {
	enqueue( WriteRawCommand{ std::move( data ) } );
}

void TerminalController::resize( int columns, int rows ) {
	enqueue( ResizeCommand{ columns, rows } );
}

void TerminalController::scrollUp( int amount ) {
	enqueue( ScrollCommand{ amount, -1 } );
}

void TerminalController::scrollDown( int amount ) {
	enqueue( ScrollCommand{ amount, 1 } );
}

Uint64 TerminalController::scrollTo( int position ) {
	if ( mShutdownRequested.load( std::memory_order_acquire ) )
		return 0;
	const Uint64 commandId = mNextScrollCommand.fetch_add( 1, std::memory_order_relaxed ) + 1;
	enqueue( ScrollCommand{ position, 0, commandId } );
	return commandId;
}

void TerminalController::selectionStart( int column, int row, int snap ) {
	enqueue( SelectionStartCommand{ column, row, snap } );
}

void TerminalController::selectionExtend( int column, int row, int type, bool done ) {
	enqueue( SelectionExtendCommand{ column, row, type, done } );
}

void TerminalController::selectionClear() {
	enqueue( SelectionClearCommand{} );
}

void TerminalController::mouseReport( TerminalMouseEventType type, Vector2i position, Uint32 flags,
									  Uint32 modifiers ) {
	enqueue( MouseCommand{ type, position, flags, modifiers } );
}

void TerminalController::setFocus( bool focus ) {
	enqueue( FocusCommand{ { focus } } );
}

void TerminalController::setCursorMode( TerminalCursorMode mode ) {
	enqueue( CursorModeCommand{ mode } );
}

void TerminalController::setColorPalette( TerminalColorPalette palette ) {
	enqueue( PaletteCommand{ std::move( palette ) } );
}

void TerminalController::setAllowMemoryTrimming( bool allow ) {
	enqueue( AllowTrimCommand{ { allow } } );
}

void TerminalController::setPresentationRate( Uint32 framesPerSecond ) {
	enqueue( PresentationRateCommand{ framesPerSecond } );
}

void TerminalController::setDataEventsEnabled( bool enabled ) {
	enqueue( DataEventsCommand{ { enabled } } );
}

void TerminalController::setPromptEventsEnabled( bool enabled ) {
	enqueue( PromptEventsCommand{ { enabled } } );
}

void TerminalController::reset() {
	enqueue( ResetCommand{} );
}

void TerminalController::terminate() {
	enqueue( TerminateCommand{} );
}

void TerminalController::restart( PtyPtr&& pty, ProcPtr&& process ) {
	if ( !pty || !process ) {
		enqueueEvent( { EventType::RestartFailure, "Invalid PTY or process" }, false );
		return;
	}
	enqueue( RestartCommand{ std::move( pty ), std::move( process ) } );
}

std::shared_ptr<const TerminalSnapshot> TerminalController::snapshot() const {
	return mPublishedSnapshot.load( std::memory_order_acquire );
}

std::optional<std::string>
TerminalController::requestSelection( std::chrono::milliseconds timeout ) {
	if ( mShutdownRequested.load( std::memory_order_acquire ) )
		return std::nullopt;
	auto response = std::make_shared<SelectionResponse>();
	enqueue( SelectionRequestCommand{ response } );
	std::unique_lock<std::mutex> lock( response->mutex );
	if ( !response->condition.wait_for( lock, timeout, [&response] { return response->ready; } ) )
		return std::nullopt;
	return std::move( response->selection );
}

std::vector<TerminalController::Event> TerminalController::drainEvents() {
	std::vector<Event> events;
	std::lock_guard<std::mutex> lock( mEventMutex );
	events.reserve( mEvents.size() );
	while ( !mEvents.empty() ) {
		events.emplace_back( std::move( mEvents.front() ) );
		mEvents.pop_front();
	}
	return events;
}

void TerminalController::enqueueEvent( Event event, bool coalescable ) {
	std::lock_guard<std::mutex> lock( mEventMutex );
	if ( coalescable ) {
		for ( auto it = mEvents.rbegin(); it != mEvents.rend(); ++it ) {
			const bool replaceable =
				it->type == EventType::Title || it->type == EventType::IconTitle ||
				it->type == EventType::HistoryLength || it->type == EventType::ScrollPosition ||
				it->type == EventType::SnapshotReady;
			if ( !replaceable )
				break;
			if ( it->type == event.type ) {
				*it = std::move( event );
				return;
			}
		}
	}
	mEvents.emplace_back( std::move( event ) );
}

void TerminalController::publishSnapshot( std::shared_ptr<const TerminalSnapshot> snapshot ) {
	const Uint64 generation = snapshot->generation;
	mPublishedSnapshot.store( std::move( snapshot ), std::memory_order_release );
	Event event{ EventType::SnapshotReady };
	event.generation = generation;
	enqueueEvent( std::move( event ), true );
}

void TerminalController::workerLoop() {
	while ( !mShutdownRequested.load( std::memory_order_acquire ) ) {
		processCommands();
		if ( mShutdownRequested.load( std::memory_order_acquire ) )
			break;

		const bool inputDrained = mEmulator->update();
		if ( !inputDrained )
			continue;

		std::unique_lock<std::mutex> lock( mCommandMutex );
		if ( mCommands.empty() && !mShutdownRequested.load( std::memory_order_relaxed ) )
			mCommandCondition.wait_for( lock, std::chrono::milliseconds( 8 ) );
	}
	mShutdownRequested.store( true, std::memory_order_release );
	mEmulator.reset();
	mWorkerDisplay.reset();
}

void TerminalController::processCommands() {
	std::deque<Command> commands;
	{
		std::lock_guard<std::mutex> lock( mCommandMutex );
		commands.swap( mCommands );
	}
	while ( !commands.empty() && !mShutdownRequested.load( std::memory_order_relaxed ) ) {
		processCommand( std::move( commands.front() ) );
		commands.pop_front();
	}
}

void TerminalController::processCommand( Command&& command ) {
	std::visit(
		[this]( auto&& value ) {
			using T = std::decay_t<decltype( value )>;
			if constexpr ( std::is_same_v<T, WriteCommand> ) {
				mEmulator->ttywrite( value.data.data(), value.data.size(), value.mayEcho );
			} else if constexpr ( std::is_same_v<T, WriteRawCommand> ) {
				mEmulator->write( value.data.data(), value.data.size() );
			} else if constexpr ( std::is_same_v<T, ResizeCommand> ) {
				mEmulator->resize( value.columns, value.rows );
			} else if constexpr ( std::is_same_v<T, ScrollCommand> ) {
				TerminalArg argument( value.amount );
				if ( value.direction < 0 )
					mEmulator->kscrollup( &argument );
				else if ( value.direction > 0 )
					mEmulator->kscrolldown( &argument );
				else {
					mEmulator->kscrollto( &argument );
					mWorkerDisplay->setLastAppliedScrollCommand( value.commandId );
				}
				mEmulator->redraw();
			} else if constexpr ( std::is_same_v<T, SelectionStartCommand> ) {
				mEmulator->selstart( value.column, value.row, value.snap );
				mEmulator->redraw();
			} else if constexpr ( std::is_same_v<T, SelectionExtendCommand> ) {
				mEmulator->selextend( value.column, value.row, value.type, value.done );
				mEmulator->redraw();
			} else if constexpr ( std::is_same_v<T, SelectionClearCommand> ) {
				mEmulator->selclear();
				mEmulator->redraw();
			} else if constexpr ( std::is_same_v<T, MouseCommand> ) {
				mEmulator->mousereport( value.type, value.position, value.flags, value.modifiers );
			} else if constexpr ( std::is_same_v<T, FocusCommand> ) {
				if ( mWorkerDisplay->getMode( MODE_FOCUS ) )
					mEmulator->ttywrite( value.value ? "\033[I" : "\033[O", 3, false );
				mWorkerDisplay->setFocused( value.value );
				mEmulator->redraw();
			} else if constexpr ( std::is_same_v<T, CursorModeCommand> ) {
				mWorkerDisplay->setCursorMode( value.mode );
				mEmulator->redraw();
			} else if constexpr ( std::is_same_v<T, PaletteCommand> ) {
				mWorkerDisplay->setPalette( std::move( value.palette ) );
				mEmulator->redraw();
			} else if constexpr ( std::is_same_v<T, PresentationRateCommand> ) {
				const Uint32 framesPerSecond = eeclamp<Uint32>( value.framesPerSecond, 1, 1000 );
				mEmulator->setPresentationInterval(
					Microseconds( 1000000.0 / static_cast<double>( framesPerSecond ) ) );
				mWorkerDisplay->setPresentationRate( framesPerSecond );
				mEmulator->redraw();
			} else if constexpr ( std::is_same_v<T, AllowTrimCommand> ) {
				mEmulator->setAllowMemoryTrimnming( value.value );
			} else if constexpr ( std::is_same_v<T, DataEventsCommand> ) {
				if ( value.value ) {
					mEmulator->setDataCb( [this]( const char* data, size_t size ) {
						enqueueEvent( { EventType::Data, std::string( data, size ) }, false );
					} );
				} else {
					mEmulator->setDataCb( {} );
				}
			} else if constexpr ( std::is_same_v<T, PromptEventsCommand> ) {
				if ( value.value ) {
					mEmulator->setPromptStateChangedCb(
						[this]( PromptState state, std::string_view data ) {
							Event event{ EventType::PromptState, std::string( data ) };
							event.promptState = state;
							enqueueEvent( std::move( event ), false );
						} );
				} else {
					mEmulator->setPromptStateChangedCb( {} );
				}
			} else if constexpr ( std::is_same_v<T, TerminateCommand> ) {
				mEmulator->terminate();
				mEmulator->redraw();
			} else if constexpr ( std::is_same_v<T, ResetCommand> ) {
				mEmulator->reset();
			} else if constexpr ( std::is_same_v<T, RestartCommand> ) {
				mEmulator->clearHistory();
				mEmulator->setPtyAndProcess( std::move( value.pty ), std::move( value.process ) );
				mEmulator->redraw();
			} else if constexpr ( std::is_same_v<T, SelectionRequestCommand> ) {
				std::lock_guard<std::mutex> lock( value.response->mutex );
				value.response->selection = mEmulator->getSelection();
				value.response->ready = true;
				value.response->condition.notify_one();
			}
		},
		std::move( command ) );
}

}} // namespace eterm::Terminal
