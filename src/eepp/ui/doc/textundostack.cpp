#include <eepp/core/core.hpp>
#include <eepp/system/log.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <eepp/ui/doc/textundostack.hpp>

#include <nlohmann/json.hpp>

using namespace EE::System;

namespace EE { namespace UI { namespace Doc {

using json = nlohmann::json;

class TextUndoCommand {
  public:
	TextUndoCommand( const Uint64& id, const TextUndoCommandType& type, const Time& timestamp );

	virtual ~TextUndoCommand();

	const Uint64& getId() const;

	const TextUndoCommandType& getType() const;

	const Time& getTimestamp() const;

	virtual json toJSON() = 0;

  protected:
	Uint64 mId;
	TextUndoCommandType mType;
	Time mTimestamp;

	json baseJSON() {
		json j;
		j["type"] = mType;
		j["timestamp"] = mTimestamp.toString();
		return j;
	}
};

class TextUndoCommandInsert : public TextUndoCommand {
  public:
	TextUndoCommandInsert( const Uint64& id, const size_t& cursorIdx, const String& text,
						   const TextPosition& position, const Time& timestamp );

	const String& getText() const;

	const TextPosition& getPosition() const;

	size_t getCursorIdx() const;

	json toJSON() {
		auto j = baseJSON();
		j["text"] = mText.toUtf8();
		j["position"] = mPosition.toString();
		j["cursorIdx"] = mCursorIdx;
		return j;
	}

	static TextUndoCommandInsert* fromJSON( json j, Uint64 id ) {
		auto timestamp = Time::fromString( j["timestamp"].get<std::string>() );
		auto text = String::fromUtf8( j["text"].get<std::string>() );
		auto position = TextPosition::fromString( j["position"].get<std::string>() );
		auto cursorIdx = j["cursorIdx"].get<size_t>();
		return eeNew( TextUndoCommandInsert, ( id, cursorIdx, text, position, timestamp ) );
	}

  protected:
	String mText;
	TextPosition mPosition;
	size_t mCursorIdx;
};

class TextUndoCommandRemove : public TextUndoCommand {
  public:
	TextUndoCommandRemove( const Uint64& id, const size_t& cursorIdx, const TextRange& range,
						   const Time& timestamp );

	const TextRange& getRange() const;

	size_t getCursorIdx() const;

	json toJSON() {
		auto j = baseJSON();
		j["range"] = mRange.toString();
		j["cursorIdx"] = mCursorIdx;
		return j;
	}

	static TextUndoCommandRemove* fromJSON( json j, Uint64 id ) {
		auto timestamp = Time::fromString( j["timestamp"].get<std::string>() );
		auto range = TextRange::fromString( j["range"].get<std::string>() );
		auto cursorIdx = j["cursorIdx"].get<size_t>();
		return eeNew( TextUndoCommandRemove, ( id, cursorIdx, range, timestamp ) );
	}

  protected:
	TextRange mRange;
	size_t mCursorIdx;
};

class TextUndoCommandSelection : public TextUndoCommand {
  public:
	TextUndoCommandSelection( const Uint64& id, const size_t& cursorIdx,
							  const TextRanges& selection, const Time& timestamp );

	const TextRanges& getSelection() const;

	size_t getCursorIdx() const;

	json toJSON() {
		auto j = baseJSON();
		j["range"] = mSelection.toString();
		j["cursorIdx"] = mCursorIdx;
		return j;
	}

	static TextUndoCommandSelection* fromJSON( json j, Uint64 id ) {
		auto timestamp = Time::fromString( j["timestamp"].get<std::string>() );
		auto range = TextRange::fromString( j["range"].get<std::string>() );
		auto cursorIdx = j["cursorIdx"].get<size_t>();
		return eeNew( TextUndoCommandSelection, ( id, cursorIdx, range, timestamp ) );
	}

  protected:
	TextRanges mSelection;
	size_t mCursorIdx;
};

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
	for ( TextUndoCommand* cmd : mUndoStack ) {
		eeDelete( cmd );
	}
	mUndoStack.clear();
}

void TextUndoStack::clearRedoStack() {
	for ( TextUndoCommand* cmd : mRedoStack ) {
		eeDelete( cmd );
	}
	mRedoStack.clear();
}

void TextUndoStack::pushUndo( UndoStackContainer& undoStack, TextUndoCommand* cmd ) {
	undoStack.push_back( cmd );
	while ( undoStack.size() > mMaxStackSize ) {
		eeDelete( undoStack.front() );
		undoStack.pop_front();
	}
}

void TextUndoStack::pushInsert( UndoStackContainer& undoStack, const String& string,
								const size_t& cursorIdx, const TextPosition& position,
								const Time& time ) {
	pushUndo( undoStack, eeNew( TextUndoCommandInsert,
								( ++mChangeIdCounter, cursorIdx, string, position, time ) ) );
}

void TextUndoStack::pushRemove( UndoStackContainer& undoStack, const size_t& cursorIdx,
								const TextRange& range, const Time& time ) {
	pushUndo( undoStack,
			  eeNew( TextUndoCommandRemove, ( ++mChangeIdCounter, cursorIdx, range, time ) ) );
}

void TextUndoStack::pushSelection( UndoStackContainer& undoStack, const size_t& cursorIdx,
								   const TextRanges& selection, const Time& time ) {
	pushUndo( undoStack, eeNew( TextUndoCommandSelection,
								( ++mChangeIdCounter, cursorIdx, selection, time ) ) );
}

void TextUndoStack::popUndo( UndoStackContainer& undoStack, UndoStackContainer& redoStack ) {
	if ( undoStack.empty() )
		return;

	TextUndoCommand* cmd = undoStack.back();
	Time lastTimestamp = cmd->getTimestamp();
	undoStack.pop_back();

	switch ( cmd->getType() ) {
		case TextUndoCommandType::Insert: {
			TextUndoCommandInsert* insert = static_cast<TextUndoCommandInsert*>( cmd );
			mDoc->insert( insert->getCursorIdx(), insert->getPosition(), insert->getText(),
						  redoStack, cmd->getTimestamp(), true );
			break;
		}
		case TextUndoCommandType::Remove: {
			TextUndoCommandRemove* remove = static_cast<TextUndoCommandRemove*>( cmd );
			mDoc->remove( remove->getCursorIdx(), remove->getRange(), redoStack,
						  cmd->getTimestamp(), true );
			break;
		}
		case TextUndoCommandType::Selection: {
			TextUndoCommandSelection* selection = static_cast<TextUndoCommandSelection*>( cmd );
			mDoc->resetSelection( selection->getSelection() );
			break;
		}
	}

	eeSAFE_DELETE( cmd );

	if ( !undoStack.empty() &&
		 eeabs( ( lastTimestamp - undoStack.back()->getTimestamp() ).asMilliseconds() ) <
			 mMergeTimeout.asMilliseconds() ) {
		popUndo( undoStack, redoStack );
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
	return mUndoStack.back()->getId();
}

std::string TextUndoStack::toJSON( bool inverted ) {
	json j = json::array();
	if ( inverted ) {
		while ( hasUndo() )
			undo();

		for ( auto it = mRedoStack.rbegin(); it != mRedoStack.rend(); it++ ) {
			auto cmd = *it;
			j.push_back( cmd->toJSON() );
		}

		while ( hasRedo() )
			redo();
	} else {
		for ( auto it = mUndoStack.rbegin(); it != mUndoStack.rend(); it++ ) {
			auto cmd = *it;
			j.push_back( cmd->toJSON() );
		}
	}
	return j.dump();
}

void TextUndoStack::fromJSON( const std::string& jsonString ) {
	json j;
	try {
		j = json::parse( jsonString, nullptr, true, true );
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
	} catch ( const json::exception& e ) {
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
