#include <eterm/terminal/programstatus.hpp>

#include <eepp/core/string.hpp>
#include <eepp/system/base64.hpp>

#include <algorithm>

using namespace EE;
using namespace EE::System;

namespace eterm { namespace Terminal {

namespace {

constexpr std::string_view ReportPrefix = "7501;";
constexpr std::string_view LegacyPrefix = "9;4;";

bool isIdentifierCharacter( char c ) {
	return ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) ||
		   c == '_' || c == '.' || c == '+' || c == '-';
}

bool isValueCharacter( char c ) {
	return isIdentifierCharacter( c ) || c == ',' || c == '/' || c == '=';
}

bool isIdentifierSegment( std::string_view segment ) {
	return !segment.empty() && segment.size() <= ProgramStatusTracker::MaxIdentifierSegmentSize &&
		   std::all_of( segment.begin(), segment.end(), isIdentifierCharacter );
}

bool isValidId( std::string_view id ) {
	if ( id.size() > ProgramStatusTracker::MaxIdSize )
		return false;
	size_t segments = 0;
	while ( true ) {
		const size_t separator = id.find( '/' );
		if ( ++segments > ProgramStatusTracker::MaxIdSegments ||
			 !isIdentifierSegment( id.substr( 0, separator ) ) )
			return false;
		if ( separator == std::string_view::npos )
			return true;
		id.remove_prefix( separator + 1 );
	}
}

std::string_view trim( std::string_view text ) {
	const auto isSpace = []( char c ) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
	while ( !text.empty() && isSpace( text.front() ) )
		text.remove_prefix( 1 );
	while ( !text.empty() && isSpace( text.back() ) )
		text.remove_suffix( 1 );
	return text;
}

/** Decimal 0-100 without a sign or surrounding text: no atoi(), which accepts all of those. */
std::optional<Uint8> parsePercentage( std::string_view text ) {
	if ( text.empty() || text.size() > 3 )
		return std::nullopt;
	int value = 0;
	for ( char c : text ) {
		if ( c < '0' || c > '9' )
			return std::nullopt;
		value = value * 10 + ( c - '0' );
	}
	if ( value > 100 )
		return std::nullopt;
	return static_cast<Uint8>( value );
}

/** One line of valid UTF-8 without C0 controls, DEL or C1 controls. */
bool isValidText( std::string_view text ) {
	if ( text.empty() )
		return true;
	// The largest input is 2 KiB, so this stays well within the worker's stack.
	String::StringBaseType codepoints[ProgramStatusTracker::MaxMessageSize];
	const size_t count =
		String::tryToUtf32( text, codepoints, ProgramStatusTracker::MaxMessageSize );
	if ( count == 0 )
		return false; // Malformed UTF-8, including overlong forms and surrogates.
	return std::none_of( codepoints, codepoints + count, []( String::StringBaseType codepoint ) {
		return codepoint < 0x20 || ( codepoint >= 0x7F && codepoint <= 0x9F );
	} );
}

bool decodeText( std::string_view encoded, size_t maxEncoded, size_t maxDecoded,
				 std::string& decoded ) {
	if ( encoded.size() > maxEncoded )
		return false;
	if ( encoded.empty() )
		return true;
	const size_t size = Base64::decode( encoded, decoded, Base64::DecodeMode::NoWhitespaceStrict );
	return size != static_cast<size_t>( -1 ) && size <= maxDecoded && isValidText( decoded );
}

std::optional<ProgramState> parseState( std::string_view value ) {
	if ( value == "idle" )
		return ProgramState::Idle;
	if ( value == "working" )
		return ProgramState::Working;
	if ( value == "blocked" )
		return ProgramState::Blocked;
	if ( value == "done" )
		return ProgramState::Done;
	if ( value == "error" )
		return ProgramState::Error;
	return std::nullopt;
}

ProgramBlockKind parseKind( std::string_view value ) {
	if ( value == "permission" )
		return ProgramBlockKind::Permission;
	if ( value == "question" )
		return ProgramBlockKind::Question;
	if ( value == "auth" )
		return ProgramBlockKind::Auth;
	return ProgramBlockKind::None;
}

/** Which record a terminal's single indicator shows. A policy of eterm, not of the protocol. */
int presentationRank( ProgramState state ) {
	switch ( state ) {
		case ProgramState::Blocked:
			return 4;
		case ProgramState::Error:
			return 3;
		case ProgramState::Working:
			return 2;
		case ProgramState::Done:
			return 1;
		case ProgramState::Idle:
			break;
	}
	return 0;
}

bool isInvisibleFormatting( String::StringBaseType codepoint ) {
	return codepoint == 0x00AD || codepoint == 0x061C || codepoint == 0x180E ||
		   codepoint == 0x200B || codepoint == 0x200E || codepoint == 0x200F ||
		   ( codepoint >= 0x202A && codepoint <= 0x202E ) ||
		   ( codepoint >= 0x2060 && codepoint <= 0x206F ) || codepoint == 0xFEFF ||
		   ( codepoint >= 0xFFF9 && codepoint <= 0xFFFB ) ||
		   ( codepoint >= 0xE0000 && codepoint <= 0xE0FFF );
}

bool isSameRecord( const ProgramStatusRecord& left, const ProgramStatusRecord& right ) {
	return left.state == right.state && left.kind == right.kind &&
		   left.progress == right.progress && left.app == right.app && left.title == right.title &&
		   left.message == right.message;
}

} // namespace

bool ProgramStatusScanner::scan( const char* data, size_t size ) {
	for ( size_t index = 0; index < size; ++index ) {
		const char c = data[index];
		switch ( mState ) {
			case State::Ground:
				if ( c != 033 )
					return false;
				mState = State::Escape;
				break;
			case State::Escape:
				if ( c != ']' ) {
					mState = State::Ground;
					return false;
				}
				mPrefix = {};
				mPrefixMatched = 0;
				mState = State::Prefix;
				break;
			case State::Prefix:
				if ( mPrefix.empty() )
					mPrefix = c == ReportPrefix[0] ? ReportPrefix : LegacyPrefix;
				if ( c != mPrefix[mPrefixMatched] ) {
					mState = State::Ground;
					return false;
				}
				if ( ++mPrefixMatched == mPrefix.size() )
					mState = State::Body;
				break;
			case State::Body:
				if ( c == '\a' ) {
					mState = State::Ground;
				} else if ( c == 033 ) {
					mState = State::Terminator;
				} else if ( c < 0x20 || c > 0x7E ) {
					mState = State::Ground;
					return false;
				}
				break;
			case State::Terminator:
				mState = State::Ground;
				if ( c != '\\' )
					return false;
				break;
		}
	}
	return true;
}

std::optional<std::string_view> ProgramStatusTracker::reportBody( std::string_view sequence ) {
	if ( sequence.substr( 0, ReportPrefix.size() ) != ReportPrefix )
		return std::nullopt;
	return sequence.substr( ReportPrefix.size() );
}

std::optional<std::string_view>
ProgramStatusTracker::legacyProgressBody( std::string_view sequence ) {
	if ( sequence.substr( 0, LegacyPrefix.size() ) != LegacyPrefix )
		return std::nullopt;
	return sequence.substr( LegacyPrefix.size() );
}

bool ProgramStatusTracker::isOversized( std::string_view sequence ) {
	return ( sequence.size() > MaxBufferedLegacySequenceSize &&
			 legacyProgressBody( sequence ).has_value() ) ||
		   ( sequence.size() > MaxBufferedSequenceSize && reportBody( sequence ).has_value() );
}

std::optional<ProgramStatusReport> ProgramStatusTracker::parseReport( std::string_view body ) {
	if ( body.size() > MaxReportSize )
		return std::nullopt;

	ProgramStatusReport report;
	auto& record = report.record;
	std::string_view stateValue, idValue, kindValue, progressValue, appValue;
	bool hasId = false;
	while ( !body.empty() ) {
		const size_t separator = body.find( ':' );
		const std::string_view pair = body.substr( 0, separator );
		body.remove_prefix( separator == std::string_view::npos ? body.size() : separator + 1 );

		const size_t equals = pair.find( '=' );
		if ( equals == std::string_view::npos )
			continue;
		const std::string_view key = trim( pair.substr( 0, equals ) );
		const std::string_view value = trim( pair.substr( equals + 1 ) );
		if ( key.size() > MaxKeySize )
			return std::nullopt;
		if ( key.empty() ||
			 !std::all_of( key.begin(), key.end(), []( char c ) { return c >= 'a' && c <= 'z'; } ) )
			continue;

		// A bad byte makes other pairs skippable, but these may not degrade: a skipped id would
		// retarget the report to the root record, and a skipped msg would blank a valid one.
		if ( key == "id" ) {
			if ( !isValidId( value ) )
				return std::nullopt;
			idValue = value;
			hasId = true;
			continue;
		}
		// Validated here, not after the loop: the last value wins, but every one must be valid.
		if ( key == "msg" ) {
			record.message.clear();
			if ( !decodeText( value, MaxEncodedMessageSize, MaxMessageSize, record.message ) )
				return std::nullopt;
			continue;
		}
		if ( key == "title" ) {
			record.title.clear();
			if ( !decodeText( value, MaxEncodedTitleSize, MaxTitleSize, record.title ) )
				return std::nullopt;
			continue;
		}

		// Its length is a hard limit, unlike its characters, which only make it absent.
		if ( key == "app" && value.size() > MaxIdentifierSegmentSize )
			return std::nullopt;

		if ( !std::all_of( value.begin(), value.end(), isValueCharacter ) )
			continue;
		if ( key == "state" )
			stateValue = value;
		else if ( key == "kind" )
			kindValue = value;
		else if ( key == "progress" )
			progressValue = value;
		else if ( key == "app" )
			appValue = value;
	}

	report.clear = stateValue == "clear";
	const auto state = parseState( stateValue );
	if ( !state && !report.clear )
		return std::nullopt;

	if ( hasId )
		record.id = idValue;
	if ( report.clear )
		return report;

	record.state = *state;
	if ( isIdentifierSegment( appValue ) )
		record.app = appValue;
	if ( record.state == ProgramState::Blocked )
		record.kind = parseKind( kindValue );
	if ( record.state == ProgramState::Working || record.state == ProgramState::Blocked )
		record.progress = parsePercentage( progressValue );
	return report;
}

std::optional<LegacyProgress> ProgramStatusTracker::parseLegacyProgress( std::string_view body ) {
	const size_t separator = body.find( ';' );
	const std::string_view state = body.substr( 0, separator );
	if ( state.size() != 1 || state[0] < '0' || state[0] > '4' )
		return std::nullopt;

	LegacyProgress progress;
	progress.state = static_cast<LegacyProgressState>( state[0] - '0' );
	// A trailing separator without a value ("9;4;3;") reports no percentage.
	const std::string_view percentage =
		separator == std::string_view::npos ? std::string_view{} : body.substr( separator + 1 );
	if ( !percentage.empty() ) {
		progress.percentage = parsePercentage( percentage );
		if ( !progress.percentage )
			return std::nullopt;
	}
	return progress;
}

std::string_view
ProgramStatusTracker::effectiveApp( const std::vector<ProgramStatusRecord>& records,
									const ProgramStatusRecord& record ) {
	if ( !record.app.empty() )
		return record.app;
	std::string_view id = record.id;
	while ( !id.empty() ) {
		const size_t separator = id.rfind( '/' );
		id = separator == std::string_view::npos ? std::string_view{} : id.substr( 0, separator );
		const auto ancestor =
			std::find_if( records.begin(), records.end(),
						  [id]( const ProgramStatusRecord& other ) { return other.id == id; } );
		if ( ancestor != records.end() && !ancestor->app.empty() )
			return ancestor->app;
	}
	return {};
}

std::string ProgramStatusTracker::sanitizeForDisplay( std::string_view text ) {
	String sanitized;
	for ( const auto codepoint : String::fromUtf8( text ) ) {
		if ( !isInvisibleFormatting( codepoint ) )
			sanitized.push_back( codepoint );
	}
	return sanitized.toUtf8();
}

void ProgramStatusTracker::handleReport( std::string_view body ) {
	auto report = parseReport( body );
	if ( !report )
		return;
	const bool wasPresentingLegacy = isPresentingLegacy();
	mLegacyEligible = false;
	if ( report->clear )
		applyClear( report->record.id, wasPresentingLegacy );
	else
		applyRecord( std::move( report->record ) );
}

void ProgramStatusTracker::applyClear( const std::string& id, bool wasPresentingLegacy ) {
	const size_t previousSize = mRecords.size();
	if ( id.empty() ) {
		mRecords.clear();
	} else {
		const std::string_view prefix = id;
		mRecords.erase( std::remove_if( mRecords.begin(), mRecords.end(),
										[prefix]( const ProgramStatusRecord& record ) {
											const std::string_view other = record.id;
											// The separator is what keeps `builder` out of `build`.
											return other == prefix ||
												   ( other.size() > prefix.size() &&
													 other[prefix.size()] == '/' &&
													 other.substr( 0, prefix.size() ) == prefix );
										} ),
						mRecords.end() );
	}
	if ( mRecords.size() != previousSize )
		markRecordsChanged();
	else if ( wasPresentingLegacy )
		mPendingChange = true;
}

void ProgramStatusTracker::applyRecord( ProgramStatusRecord&& record ) {
	record.updateSerial = record.revision = ++mUpdateSerial;
	auto existing = std::find_if(
		mRecords.begin(), mRecords.end(),
		[&record]( const ProgramStatusRecord& other ) { return other.id == record.id; } );
	if ( existing == mRecords.end() ) {
		if ( mRecords.size() == MaxRecords ) {
			mRecords.erase( std::min_element( mRecords.begin(), mRecords.end(),
											  []( const auto& left, const auto& right ) {
												  return left.updateSerial < right.updateSerial;
											  } ) );
		}
		mRecords.emplace_back( std::move( record ) );
		markRecordsChanged();
		return;
	}
	if ( !isSameRecord( *existing, record ) ) {
		*existing = std::move( record );
		markRecordsChanged();
		return;
	}
	// A repeated report still refreshes recency, which matters only if it changes the selection.
	const auto* selected = selectRecord();
	existing->updateSerial = record.updateSerial;
	if ( selectRecord() != selected )
		markRecordsChanged();
}

bool ProgramStatusTracker::handleLegacyProgress( std::string_view body ) {
	const auto report = parseLegacyProgress( body );
	if ( !report )
		return false;
	if ( report->state == LegacyProgressState::Inactive ) {
		clearLegacyProgress();
		return false;
	}

	auto percentage = mLegacy.percentage;
	switch ( report->state ) {
		case LegacyProgressState::Normal:
			percentage = report->percentage;
			break;
		case LegacyProgressState::Indeterminate:
			break; // Keeps the value, like Ghostty, but it is not presented.
		default:
			if ( report->percentage )
				percentage = report->percentage;
			break;
	}
	const bool eligible = mRecords.empty();
	if ( mLegacy.state != report->state || mLegacy.percentage != percentage ||
		 mLegacyEligible != eligible )
		mPendingChange = true;
	mLegacy.state = report->state;
	mLegacy.percentage = percentage;
	mLegacyEligible = eligible;
	return true;
}

void ProgramStatusTracker::dropTransientStatus() {
	const size_t previousSize = mRecords.size();
	mRecords.erase( std::remove_if( mRecords.begin(), mRecords.end(),
									[]( const ProgramStatusRecord& record ) {
										return record.state != ProgramState::Done &&
											   record.state != ProgramState::Error;
									} ),
					mRecords.end() );
	if ( mRecords.size() != previousSize )
		markRecordsChanged();
	clearLegacyProgress();
}

void ProgramStatusTracker::clearLegacyProgress() {
	if ( !isLegacyProgressActive() )
		return;
	mLegacy = {};
	mLegacyEligible = false;
	mPendingChange = true;
}

void ProgramStatusTracker::reset() {
	clearLegacyProgress();
	mLegacyEligible = false;
	if ( !mRecords.empty() ) {
		mRecords.clear();
		markRecordsChanged();
	}
}

void ProgramStatusTracker::markRecordsChanged() {
	mRecordsSnapshot.reset();
	mPendingChange = true;
}

const ProgramStatusRecord* ProgramStatusTracker::selectRecord() const {
	const ProgramStatusRecord* selected = nullptr;
	for ( const auto& record : mRecords ) {
		if ( !selected ) {
			selected = &record;
			continue;
		}
		const int rank = presentationRank( record.state );
		const int selectedRank = presentationRank( selected->state );
		if ( rank > selectedRank ||
			 ( rank == selectedRank && record.updateSerial > selected->updateSerial ) )
			selected = &record;
	}
	return selected;
}

bool ProgramStatusTracker::isPresentingLegacy() const {
	return mRecords.empty() && isLegacyProgressActive() && mLegacyEligible;
}

PresentedProgramStatus ProgramStatusTracker::resolvePresentation() const {
	PresentedProgramStatus presentation;
	if ( const auto* record = selectRecord() ) {
		presentation.source = ProgramStatusSource::Osc7501;
		presentation.recordId = record->id;
		presentation.revision = record->revision;
		presentation.state = record->state;
		presentation.kind = record->kind;
		presentation.percentage = record->progress;
		presentation.app = effectiveApp( mRecords, *record );
		presentation.title = sanitizeForDisplay( record->title );
		presentation.message = sanitizeForDisplay( record->message );
	} else if ( isPresentingLegacy() ) {
		presentation.source = ProgramStatusSource::Osc9_4;
		presentation.legacyState = mLegacy.state;
		if ( mLegacy.state != LegacyProgressState::Indeterminate )
			presentation.percentage = mLegacy.percentage;
	}
	return presentation;
}

std::shared_ptr<const ProgramStatusSnapshot> ProgramStatusTracker::takeSnapshot() {
	if ( !mRecordsSnapshot )
		mRecordsSnapshot = std::make_shared<const std::vector<ProgramStatusRecord>>( mRecords );
	auto snapshot = std::make_shared<ProgramStatusSnapshot>();
	snapshot->records = mRecordsSnapshot;
	snapshot->presentation = resolvePresentation();
	snapshot->generation = ++mGeneration;
	snapshot->legacy = mLegacy;
	mPendingChange = false;
	return snapshot;
}

}} // namespace eterm::Terminal
