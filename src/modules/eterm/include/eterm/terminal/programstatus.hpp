#ifndef ETERM_PROGRAMSTATUS_HPP
#define ETERM_PROGRAMSTATUS_HPP

#include <eepp/config.hpp>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace EE;

namespace eterm { namespace Terminal {

/** OSC 7501 record state. `clear` is an action, never a stored state. */
enum class ProgramState : Uint8 { Idle, Working, Blocked, Done, Error };

enum class ProgramBlockKind : Uint8 { None, Permission, Question, Auth };

/** OSC 9;4 state, numbered as on the wire. State 4 is "paused" in Ghostty and "warning" in Windows
 * Terminal: it does not mean that the user must act. */
enum class LegacyProgressState : Uint8 {
	Inactive = 0,
	Normal = 1,
	Error = 2,
	Indeterminate = 3,
	PausedOrWarning = 4
};

enum class ProgramStatusSource : Uint8 { None, Osc7501, Osc9_4 };

/** One OSC 7501 record. `title` and `message` are validated UTF-8 and remain untrusted data. */
struct ProgramStatusRecord {
	std::string id;	 // Empty for the root record.
	std::string app; // As reported: resolve inheritance with ProgramStatusTracker::effectiveApp().
	std::string title;
	std::string message;
	Uint64 updateSerial{ 0 }; // Orders records by their last update, for eviction and selection.
	Uint64 revision{ 0 };	  // Changes with the content, unlike updateSerial: repeats keep it.
	ProgramState state{ ProgramState::Idle };
	ProgramBlockKind kind{ ProgramBlockKind::None };
	std::optional<Uint8> progress; // Absent means indeterminate, not zero.
};

struct LegacyProgress {
	std::optional<Uint8> percentage;
	LegacyProgressState state{ LegacyProgressState::Inactive };
};

/** A complete, validated OSC 7501 report, not applied to any stored state yet. */
struct ProgramStatusReport {
	ProgramStatusRecord record;
	bool clear{ false };
};

/** What a UI should show for a terminal, derived from the stores and never a store itself. Check
 * `source` first. Text is safe to show outside the grid: see sanitizeForDisplay(). */
struct PresentedProgramStatus {
	std::string recordId;
	std::string app;
	std::string title;
	std::string message;
	/** Tells apart results with the same text, whose report in between may have been coalesced. */
	Uint64 revision{ 0 };
	std::optional<Uint8> percentage;
	ProgramStatusSource source{ ProgramStatusSource::None };
	ProgramState state{ ProgramState::Idle };
	ProgramBlockKind kind{ ProgramBlockKind::None };
	LegacyProgressState legacyState{ LegacyProgressState::Inactive };

	bool operator==( const PresentedProgramStatus& ) const = default;
};

/** Immutable state handed to the UI, unrelated to the cell snapshots. Both source models stay
 * available whichever one the presentation chose. */
struct ProgramStatusSnapshot {
	/** Shared between snapshots that differ only in `legacy`. Never null when published. */
	std::shared_ptr<const std::vector<ProgramStatusRecord>> records;
	PresentedProgramStatus presentation;
	Uint64 generation{ 0 };
	LegacyProgress legacy;
};

/**
 * Recognizes PTY reads made only of status OSCs, which change nothing on screen.
 *
 * Conservative by construction: it proves that bytes are exactly an OSC 7501 or OSC 9;4 sequence of
 * printable ASCII, from a parser at ground state, and anything else ends the proof. A false result
 * only costs the snapshot that every PTY read used to publish. Its state survives reads, so a
 * sequence fragmented across them is still recognized.
 */
class ProgramStatusScanner {
  public:
	/** @param parserAtGround The parser holds no partial sequence or buffered bytes. */
	bool isMetadataOnly( const char* data, size_t size, bool parserAtGround ) {
		// Ordinary output pays this one check per read.
		if ( mState == State::Ground && ( !parserAtGround || size == 0 || data[0] != 033 ) )
			return false;
		return scan( data, size );
	}

	void reset() { mState = State::Ground; }

  private:
	enum class State : Uint8 { Ground, Escape, Prefix, Body, Terminator };

	bool scan( const char* data, size_t size );

	std::string_view mPrefix;
	size_t mPrefixMatched{ 0 };
	State mState{ State::Ground };
};

/**
 * The status of the programs in one terminal: the OSC 7501 records and the OSC 9;4 progress are
 * separate stores, and neither protocol ever writes into the other's. Owned by the worker-side
 * emulator; the UI only receives snapshots. Keeps no clock: the owner decides when OSC 9;4 expired.
 */
class ProgramStatusTracker {
  public:
	/** The limit includes the ESC ] introducer and the terminator. Both terminators get the room
	 * ST needs, so a BEL-terminated report can be one byte short of the protocol's limit, which
	 * allows terminals to choose lower ones. */
	static constexpr size_t MaxSequenceSize = 4096;
	static constexpr size_t MaxBufferedSequenceSize = MaxSequenceSize - 4;
	static constexpr size_t MaxReportSize = MaxBufferedSequenceSize - 5; // Without "7501;"
	static constexpr size_t MaxKeySize = 16;
	static constexpr size_t MaxEncodedMessageSize = 2732;
	static constexpr size_t MaxMessageSize = 2048;
	static constexpr size_t MaxEncodedTitleSize = 256;
	static constexpr size_t MaxTitleSize = 192;
	static constexpr size_t MaxIdentifierSegmentSize = 32; // Also the `app` limit.
	static constexpr size_t MaxIdSize = 128;
	static constexpr size_t MaxIdSegments = 8;
	static constexpr size_t MaxRecords = 256;
	/** No known emitter comes close: "9;4;" plus a state and a percentage is at most 10 bytes. */
	static constexpr size_t MaxBufferedLegacySequenceSize = 64;
	static constexpr std::string_view ProbeReply = "\033]7501;?\033\\";

	/** @return The body of an OSC 7501 report, or nullopt if @p sequence is something else. */
	static std::optional<std::string_view> reportBody( std::string_view sequence );

	/** @return The body of an OSC 9;4 report, or nullopt if @p sequence is something else. */
	static std::optional<std::string_view> legacyProgressBody( std::string_view sequence );

	/** True if a partially received OSC already exceeds its protocol's limit. */
	static bool isOversized( std::string_view sequence );

	/** Validates a whole report without touching any state.
	 * @param body The sequence after "7501;".
	 * @return nullopt if the report must be discarded whole. */
	static std::optional<ProgramStatusReport> parseReport( std::string_view body );

	/** @param body The sequence after "9;4;".
	 * @return The state and percentage as reported, or nullopt if the sequence is malformed. */
	static std::optional<LegacyProgress> parseLegacyProgress( std::string_view body );

	/** The `app` of @p record, or that of its nearest stored ancestor. */
	static std::string_view effectiveApp( const std::vector<ProgramStatusRecord>& records,
										  const ProgramStatusRecord& record );

	/** Removes bidi overrides and invisible formatting from valid UTF-8. A program's text could
	 * otherwise reorder or hide what a UI shows around it. Keeps the joiners that emoji and some
	 * scripts need. */
	static std::string sanitizeForDisplay( std::string_view text );

	void handleReport( std::string_view body );

	/** @return True if a report of active progress was accepted, even an unchanged one: its owner
	 * must then restart the expiry timer. */
	bool handleLegacyProgress( std::string_view body );

	/** OSC 133 A: a prompt means that the programs that reported transient states have ended. */
	void onPromptStart() { dropTransientStatus(); }

	void onProcessExit() { dropTransientStatus(); }

	void expireLegacyProgress() { clearLegacyProgress(); }

	/** Full reset, or a new process: starts a new epoch. */
	void reset();

	bool isLegacyProgressActive() const { return mLegacy.state != LegacyProgressState::Inactive; }

	bool hasPendingChange() const { return mPendingChange; }

	const std::vector<ProgramStatusRecord>& getRecords() const { return mRecords; }

	const LegacyProgress& getLegacyProgress() const { return mLegacy; }

	/** The arbitration between both stores. */
	PresentedProgramStatus resolvePresentation() const;

	/** Builds the snapshot of the current state and consumes the pending change. */
	std::shared_ptr<const ProgramStatusSnapshot> takeSnapshot();

  private:
	void applyClear( const std::string& id, bool wasPresentingLegacy );

	void applyRecord( ProgramStatusRecord&& record );

	void dropTransientStatus();

	void clearLegacyProgress();

	void markRecordsChanged();

	const ProgramStatusRecord* selectRecord() const;

	bool isPresentingLegacy() const;

	std::vector<ProgramStatusRecord> mRecords;
	std::shared_ptr<const std::vector<ProgramStatusRecord>> mRecordsSnapshot;
	Uint64 mUpdateSerial{ 0 };
	Uint64 mGeneration{ 0 };
	LegacyProgress mLegacy;
	/** OSC 9;4 is shown only if reported while no record existed and no OSC 7501 report arrived
	 * since. So a clear never resurrects older progress, but a later fresh report can fall back. */
	bool mLegacyEligible{ false };
	bool mPendingChange{ false };
};

}} // namespace eterm::Terminal

#endif
