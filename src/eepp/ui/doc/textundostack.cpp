#include <eepp/core/core.hpp>
#include <eepp/system/log.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <eepp/ui/doc/textundostack.hpp>

#include <nlohmann/json.hpp>

using namespace EE::System;

namespace EE { namespace UI { namespace Doc {

nlohmann::json TextUndoCommand::baseJSON() const {
	nlohmann::json j;
	j["type"] = mType;
	j["timestamp"] = mTimestamp.toString();
	return j;
}

nlohmann::json TextUndoCommandInsert::toJSON() const {
	auto j = baseJSON();
	j["text"] = mText.toUtf8();
	j["position"] = mPosition.toString();
	j["cursorIdx"] = mCursorIdx;
	return j;
}

TextUndoCommandInsert TextUndoCommandInsert::fromJSON( nlohmann::json j, Uint64 id ) {
	auto timestamp = Time::fromString( j["timestamp"].get<std::string>() );
	auto text = String::fromUtf8( j["text"].get<std::string>() );
	auto position = TextPosition::fromString( j["position"].get<std::string>() );
	auto cursorIdx = j["cursorIdx"].get<size_t>();
	return TextUndoCommandInsert( id, cursorIdx, text, position, timestamp );
}

nlohmann::json TextUndoCommandRemove::toJSON() const {
	auto j = baseJSON();
	j["range"] = mRange.toString();
	j["cursorIdx"] = mCursorIdx;
	return j;
}

TextUndoCommandRemove TextUndoCommandRemove::fromJSON( nlohmann::json j, Uint64 id ) {
	auto timestamp = Time::fromString( j["timestamp"].get<std::string>() );
	auto range = TextRange::fromString( j["range"].get<std::string>() );
	auto cursorIdx = j["cursorIdx"].get<size_t>();
	return TextUndoCommandRemove( id, cursorIdx, range, timestamp );
}

nlohmann::json TextUndoCommandSelection::toJSON() const {
	auto j = baseJSON();
	j["range"] = mSelection.toString();
	j["cursorIdx"] = mCursorIdx;
	return j;
}

TextUndoCommandSelection TextUndoCommandSelection::fromJSON( nlohmann::json j, Uint64 id ) {
	auto timestamp = Time::fromString( j["timestamp"].get<std::string>() );
	auto range = TextRange::fromString( j["range"].get<std::string>() );
	auto cursorIdx = j["cursorIdx"].get<size_t>();
	return TextUndoCommandSelection( id, cursorIdx, range, timestamp );
}

TextUndoCommand::TextUndoCommand( const Uint64& id, const TextUndoCommandType& type,
								  const Time& timestamp ) :
	mId( id ), mType( type ), mTimestamp( timestamp ) {}

TextUndoCommand::~TextUndoCommand() {}

const Uint64& TextUndoCommand::getId() const {
	return mId;
}

const TextUndoCommandType& TextUndoCommand::getType() const {
	return mType;
}

const Time& TextUndoCommand::getTimestamp() const {
	return mTimestamp;
}

TextUndoCommandInsert::TextUndoCommandInsert( const Uint64& id, const size_t& cursorIdx,
											  const String& text, const TextPosition& position,
											  const Time& timestamp ) :
	TextUndoCommand( id, TextUndoCommandType::Insert, timestamp ),
	mText( text ),
	mPosition( position ),
	mCursorIdx( cursorIdx ) {}

const String& TextUndoCommandInsert::getText() const {
	return mText;
}

const TextPosition& TextUndoCommandInsert::getPosition() const {
	return mPosition;
}

size_t TextUndoCommandInsert::getCursorIdx() const {
	return mCursorIdx;
}

TextUndoCommandRemove::TextUndoCommandRemove( const Uint64& id, const size_t& cursorIdx,
											  const TextRange& range, const Time& timestamp ) :
	TextUndoCommand( id, TextUndoCommandType::Remove, timestamp ),
	mRange( range ),
	mCursorIdx( cursorIdx ) {}

const TextRange& TextUndoCommandRemove::getRange() const {
	return mRange;
}

size_t TextUndoCommandRemove::getCursorIdx() const {
	return mCursorIdx;
}

TextUndoCommandSelection::TextUndoCommandSelection( const Uint64& id, const size_t& cursorIdx,
													const TextRanges& selection,
													const Time& timestamp ) :
	TextUndoCommand( id, TextUndoCommandType::Selection, timestamp ),
	mSelection( selection ),
	mCursorIdx( cursorIdx ) {}

const TextRanges& TextUndoCommandSelection::getSelection() const {
	return mSelection;
}

size_t TextUndoCommandSelection::getCursorIdx() const {
	return mCursorIdx;
}

TextUndoStack::TextUndoStack( TextDocument* owner, const Uint32& maxStackSize ) :
	mDoc( owner ),
	mMaxStackSize( maxStackSize ),
	mChangeIdCounter( 0 ),
	mMergeTimeout( Milliseconds( 300.f ) ) {}

TextUndoStack::~TextUndoStack() {
	clear();
}

void TextUndoStack::clear() {
	clearUndoStack();
	clearRedoStack();
}

void TextUndoStack::clearUndoStack() {
	mUndoStack.clear();
}

void TextUndoStack::clearRedoStack() {
	mRedoStack.clear();
}

void TextUndoStack::limitStackSize( UndoStackContainer& stack ) {
	while ( stack.size() > mMaxStackSize )
		stack.pop_front();
}

void TextUndoStack::pushUndo( UndoStackContainer& undoStack, UndoCommandVariant&& cmd ) {
	undoStack.emplace_back( std::move( cmd ) );
	limitStackSize( undoStack );
}

void TextUndoStack::pushInsert( UndoStackContainer& undoStack, const String& string,
								const size_t& cursorIdx, const TextPosition& position,
								const Time& time ) {
	pushUndo( undoStack,
			  TextUndoCommandInsert( ++mChangeIdCounter, cursorIdx, string, position, time ) );
}

void TextUndoStack::pushRemove( UndoStackContainer& undoStack, const size_t& cursorIdx,
								const TextRange& range, const Time& time ) {
	pushUndo( undoStack, TextUndoCommandRemove( ++mChangeIdCounter, cursorIdx, range, time ) );
}

void TextUndoStack::pushSelection( UndoStackContainer& undoStack, const size_t& cursorIdx,
								   const TextRanges& selection, const Time& time ) {
	pushUndo( undoStack,
			  TextUndoCommandSelection( ++mChangeIdCounter, cursorIdx, selection, time ) );
}

void TextUndoStack::popUndo( UndoStackContainer& undoStack, UndoStackContainer& redoStack ) {
	if ( undoStack.empty() )
		return;

	UndoCommandVariant cmdVariant = std::move( undoStack.back() );
	undoStack.pop_back();

	Time lastTimestamp;

	std::visit(
		[&]( auto& cmd ) {
			lastTimestamp = cmd.getTimestamp();

			using T = std::decay_t<decltype( cmd )>;

			if constexpr ( std::is_same_v<T, TextUndoCommandInsert> ) {
				mDoc->insert( cmd.getCursorIdx(), cmd.getPosition(), cmd.getText(), redoStack,
							  cmd.getTimestamp(), true );
			} else if constexpr ( std::is_same_v<T, TextUndoCommandRemove> ) {
				mDoc->remove( cmd.getCursorIdx(), cmd.getRange(), redoStack, cmd.getTimestamp(),
							  true );
			} else if constexpr ( std::is_same_v<T, TextUndoCommandSelection> ) {
				mDoc->resetSelection( cmd.getSelection() );
			}
		},
		cmdVariant );

	if ( !undoStack.empty() ) {
		Time prevTimestamp;
		std::visit(
			[&prevTimestamp]( const auto& prevCmd ) { prevTimestamp = prevCmd.getTimestamp(); },
			undoStack.back() );

		if ( eeabs( ( lastTimestamp - prevTimestamp ).asMilliseconds() ) <
			 mMergeTimeout.asMilliseconds() ) {
			popUndo( undoStack, redoStack );
		}
	}
}

void TextUndoStack::undo() {
	popUndo( mUndoStack, mRedoStack );
}

void TextUndoStack::redo() {
	popUndo( mRedoStack, mUndoStack );
}

bool TextUndoStack::hasUndo() const {
	return !mUndoStack.empty();
}

bool TextUndoStack::hasRedo() const {
	return !mRedoStack.empty();
}

const Uint32& TextUndoStack::getMaxStackSize() const {
	return mMaxStackSize;
}

const Time& TextUndoStack::getMergeTimeout() const {
	return mMergeTimeout;
}

void TextUndoStack::setMergeTimeout( const Time& mergeTimeout ) {
	mMergeTimeout = mergeTimeout;
}

Uint64 TextUndoStack::getCurrentChangeId() const {
	if ( mUndoStack.empty() )
		return 0;
	return std::visit( []( const auto& cmd ) { return cmd.getId(); }, mUndoStack.back() );
}

std::string TextUndoStack::toJSON( bool inverted ) {
	nlohmann::json j = nlohmann::json::array();
	if ( inverted ) {
		while ( hasUndo() )
			undo();

		for ( auto it = mRedoStack.rbegin(); it != mRedoStack.rend(); it++ ) {
			const auto& cmdVariant = *it;
			std::visit( [&]( const auto& cmd ) { j.push_back( cmd.toJSON() ); }, cmdVariant );
		}

		while ( hasRedo() )
			redo();
	} else {
		for ( auto it = mUndoStack.rbegin(); it != mUndoStack.rend(); it++ ) {
			const auto& cmdVariant = *it;
			std::visit( [&]( const auto& cmd ) { j.push_back( cmd.toJSON() ); }, cmdVariant );
		}
	}
	return j.dump();
}

void TextUndoStack::fromJSON( const std::string& jsonString ) {
	nlohmann::json j;
	try {
		j = nlohmann::json::parse( jsonString, nullptr, true, true );
		if ( !j.is_array() )
			return;
		for ( auto it = j.rbegin(); it != j.rend(); it++ ) {
			const auto& jobj = *it;
			auto type = static_cast<TextUndoCommandType>( jobj["type"].get<int>() );
			switch ( type ) {
				case TextUndoCommandType::Insert:
					pushUndo( mRedoStack,
							  TextUndoCommandInsert::fromJSON( jobj, ++mChangeIdCounter ) );
					break;
				case TextUndoCommandType::Remove:
					pushUndo( mRedoStack,
							  TextUndoCommandRemove::fromJSON( jobj, ++mChangeIdCounter ) );
					break;
				case TextUndoCommandType::Selection:
					pushUndo( mRedoStack,
							  TextUndoCommandSelection::fromJSON( jobj, ++mChangeIdCounter ) );
					break;
			}
		}
	} catch ( const nlohmann::json::exception& ) {
		Log::error( "TextUndoStack::fromJSON - Error parsing json string:\n%s", jsonString );
	}
}

UndoStackContainer& TextUndoStack::getUndoStackContainer() {
	return mUndoStack;
}

UndoStackContainer& TextUndoStack::getRedoStackContainer() {
	return mRedoStack;
}

}}} // namespace EE::UI::Doc
