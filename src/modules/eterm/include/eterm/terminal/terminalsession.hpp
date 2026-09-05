#ifndef ETERM_TERMINALSESSION_HPP
#define ETERM_TERMINALSESSION_HPP

#include <eepp/math/vector2.hpp>
#include <eepp/window/keycodes.hpp>
#include <eterm/system/iprocess.hpp>
#include <eterm/terminal/ipseudoterminal.hpp>
#include <eterm/terminal/terminalemulator.hpp>
#include <eterm/terminal/terminalgraphics.hpp>
#include <eterm/terminal/terminaltypes.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <variant>
#include <vector>

using namespace EE;
using namespace EE::Math;
using namespace EE::Window;
using namespace eterm::System;

namespace eterm { namespace Terminal {

/** Immutable worker-to-UI presentation state. Cell selection is already applied as ATTR_REVERSE. */
struct TerminalSnapshot {
	std::shared_ptr<const TerminalGraphicsPresentation> graphics;
	std::vector<TerminalGlyph> cells;
	std::vector<Uint8> dirtyRows;
	std::string title;
	std::string currentWorkingDirectory;
	std::string selection;
	Uint64 generation{ 0 };
	Uint64 lastAppliedScrollCommand{ 0 };
	Vector2i cursor;
	TerminalGlyph cursorGlyph;
	int columns{ 0 };
	int rows{ 0 };
	int historyLength{ 0 };
	int scrollPosition{ 0 };
	int windowMode{ MODE_VISIBLE | MODE_FOCUSED };
	int processId{ 0 };
	int exitCode{ 0 };
	Uint32 presentationRate{ 60 };
	TerminalCursorMode cursorMode{ SteadyUnderline };
	TerminalSelectionMode selectionMode{ SEL_IDLE };
	PromptState promptState{ PromptState::Unknown };
	bool cursorVisible{ false };
	bool cursorSelected{ false };
	bool hasSelection{ false };
	bool altScreen{ false };
	bool processExited{ false };

	/** Dirty rows describe only the transition from the immediately preceding generation. */
	bool dirtyRowsFollow( Uint64 previousGeneration ) const {
		return generation == previousGeneration + 1;
	}
};

struct TerminalColorPalette {
	std::vector<Uint32> colors;
	Uint32 cursor{ 0 };
	Uint32 foreground{ 0 };
	Uint32 background{ 0 };
};

/**
 * Per-terminal worker/session.
 *
 * After create() returns, the worker thread exclusively owns the emulator, PTY, process, parser,
 * history, selection, and cursor. UI code may only enqueue commands, drain events, or retain an
 * immutable snapshot returned by snapshot(). No session callback is invoked by the worker.
 */
class TerminalSession final : public std::enable_shared_from_this<TerminalSession> {
  public:
	using PtyPtr = std::unique_ptr<IPseudoTerminal>;
	using ProcPtr = std::unique_ptr<IProcess>;

	enum class EventType : Uint8 {
		Title,
		IconTitle,
		HistoryLength,
		ScrollPosition,
		Bell,
		Clipboard,
		ProcessExit,
		RestartFailure,
		SnapshotReady,
		Data,
		PromptState,
		Color,
		Error
	};

	struct Event {
		EventType type{ EventType::Error };
		std::string data;
		Uint64 generation{ 0 };
		int value{ 0 };
		PromptState promptState{ PromptState::Unknown };
	};

	static std::shared_ptr<TerminalSession> create( PtyPtr&& pty, ProcPtr&& process,
													size_t historySize,
													TerminalColorPalette palette = {} );

	~TerminalSession();

	TerminalSession( const TerminalSession& ) = delete;
	TerminalSession( TerminalSession&& ) = delete;
	TerminalSession& operator=( const TerminalSession& ) = delete;
	TerminalSession& operator=( TerminalSession&& ) = delete;

	void write( std::string data, bool mayEcho = true );
	void writeRaw( std::string data );
	void keyEvent( KittyKeyEvent event );
	void textInput( Uint32 codepoint );
	void resize( int columns, int rows );
	void resize( int columns, int rows, int pixelWidth, int pixelHeight );
	void scrollUp( int amount );
	void scrollDown( int amount );
	/** Returns an ordered command id that is copied into snapshots after the scroll is applied. */
	Uint64 scrollTo( int position );
	void selectionStart( int column, int row, int snap );
	void selectionExtend( int column, int row, int type, bool done );
	void selectionClear();
	void mouseReport( TerminalMouseEventType type, Vector2i cellPosition, Vector2i pixelPosition,
					  Uint32 flags, Uint32 modifiers );
	void setFocus( bool focus );
	void setCursorMode( TerminalCursorMode mode );
	void setColorPalette( TerminalColorPalette palette );
	void setAllowMemoryTrimming( bool allow );
	void setPresentationRate( Uint32 framesPerSecond );
	void setDataEventsEnabled( bool enabled );
	void setPromptEventsEnabled( bool enabled );
	void reset();
	void terminate();
	void restart( PtyPtr&& pty, ProcPtr&& process );
	void shutdown();

	std::shared_ptr<const TerminalSnapshot> snapshot() const;
	std::vector<Event> drainEvents();
	std::vector<TerminalGraphicsUpdate> drainGraphicsUpdates();
	void requestGraphicsResync();

	/** Bounded exact-selection request. Returns no value on timeout or during shutdown. */
	std::optional<std::string>
	requestSelection( std::chrono::milliseconds timeout = std::chrono::milliseconds( 50 ) );

  private:
	class WorkerDisplay;
	struct SelectionResponse;

	struct WriteCommand {
		std::string data;
		bool mayEcho{ true };
	};
	struct WriteRawCommand {
		std::string data;
	};
	struct KeyCommand {
		KittyKeyEvent event;
	};
	struct TextInputCommand {
		Uint32 codepoint{ 0 };
	};
	struct ResizeCommand {
		int columns{ 0 };
		int rows{ 0 };
		int pixelWidth{ 0 };
		int pixelHeight{ 0 };
	};
	struct ScrollCommand {
		int amount{ 0 };
		int direction{ 0 };
		Uint64 commandId{ 0 };
	};
	struct SelectionStartCommand {
		int column{ 0 };
		int row{ 0 };
		int snap{ 0 };
	};
	struct SelectionExtendCommand {
		int column{ 0 };
		int row{ 0 };
		int type{ 0 };
		bool done{ false };
	};
	struct MouseCommand {
		TerminalMouseEventType type{ TerminalMouseEventType::MouseMotion };
		Vector2i cellPosition;
		Vector2i pixelPosition;
		Uint32 flags{ 0 };
		Uint32 modifiers{ 0 };
	};
	struct BoolCommand {
		bool value{ false };
	};
	struct CursorModeCommand {
		TerminalCursorMode mode{ SteadyUnderline };
	};
	struct PaletteCommand {
		TerminalColorPalette palette;
	};
	struct PresentationRateCommand {
		Uint32 framesPerSecond{ 60 };
	};
	struct RestartCommand {
		PtyPtr pty;
		ProcPtr process;
	};
	struct SelectionRequestCommand {
		std::shared_ptr<SelectionResponse> response;
	};
	struct SelectionClearCommand {};
	struct GraphicsResyncCommand {};
	struct ResetCommand {};
	struct TerminateCommand {};
	struct AllowTrimCommand : BoolCommand {};
	struct DataEventsCommand : BoolCommand {};
	struct PromptEventsCommand : BoolCommand {};
	struct FocusCommand : BoolCommand {};

	using Command =
		std::variant<WriteCommand, WriteRawCommand, KeyCommand, TextInputCommand, ResizeCommand,
					 ScrollCommand, SelectionStartCommand, SelectionExtendCommand,
					 SelectionClearCommand, MouseCommand, FocusCommand, CursorModeCommand,
					 PaletteCommand, PresentationRateCommand, AllowTrimCommand, DataEventsCommand,
					 PromptEventsCommand, TerminateCommand, RestartCommand, ResetCommand,
					 SelectionRequestCommand, GraphicsResyncCommand>;

	TerminalSession( PtyPtr&& pty, ProcPtr&& process, size_t historySize,
					 TerminalColorPalette palette );

	void start();
	bool enqueue( Command&& command );
	void workerLoop();
	void processCommands();
	void processCommand( Command&& command );
	void enqueueEvent( Event event, bool coalescable );
	void publishSnapshot( std::shared_ptr<const TerminalSnapshot> snapshot );
	Uint64 enqueueGraphicsUpdate( TerminalGraphicsUpdate update );

	std::shared_ptr<WorkerDisplay> mWorkerDisplay;
	std::unique_ptr<TerminalEmulator> mEmulator;
	std::thread mWorker;
	std::mutex mShutdownMutex;
	mutable std::mutex mCommandMutex;
	std::condition_variable mCommandCondition;
	std::deque<Command> mCommands;
	std::mutex mEventMutex;
	std::deque<Event> mEvents;
	TerminalGraphicsUpdateQueue mGraphicsUpdates;
	mutable std::mutex mPublishedSnapshotMutex;
	std::shared_ptr<const TerminalSnapshot> mPublishedSnapshot;
	std::atomic<bool> mShutdownRequested{ false };
	std::atomic<Uint64> mNextScrollCommand{ 0 };
};

}} // namespace eterm::Terminal

#endif
