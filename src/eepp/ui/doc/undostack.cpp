#include <eepp/ui/doc/textdocument.hpp>
#include <eepp/ui/doc/undostack.hpp>

using namespace EE::System;

namespace EE { namespace UI { namespace Doc {

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

TextUndoCommandInsert::TextUndoCommandInsert( const Uint64& id, const String& text,
											  const TextPosition& position,
											  const Time& timestamp ) :
	TextUndoCommand( id, TextUndoCommandType::Insert, timestamp ),
	mText( text ),
	mPosition( position ) {}

const String& TextUndoCommandInsert::getText() const {
	return mText;
}

const TextPosition& TextUndoCommandInsert::getPosition() const {
	return mPosition;
}

TextUndoCommandRemove::TextUndoCommandRemove( const Uint64& id, const TextRange& range,
											  const Time& timestamp ) :
	TextUndoCommand( id, TextUndoCommandType::Remove, timestamp ), mRange( range ) {}

const TextRange& TextUndoCommandRemove::getRange() const {
	return mRange;
}

TextUndoCommandSelection::TextUndoCommandSelection( const Uint64& id, const TextRange& selection,
													const Time& timestamp ) :
	TextUndoCommand( id, TextUndoCommandType::Selection, timestamp ), mSelection( selection ) {}

const TextRange& TextUndoCommandSelection::getSelection() const {
	return mSelection;
}

UndoStack::UndoStack( TextDocument* owner, const Uint32& maxStackSize ) :
	mDoc( owner ),
	mMaxStackSize( maxStackSize ),
	mChangeIdCounter( 0 ),
	mMergeTimeout( Milliseconds( 300.f ) ) {}

void UndoStack::clear() {
	mUndoStack.clear();
	clearRedoStack();
}

void UndoStack::clearRedoStack() {
	mRedoStack.clear();
}

void UndoStack::pushUndo( UndoStackContainer& undoStack, std::unique_ptr<TextUndoCommand>&& cmd ) {
	undoStack.push_back( std::move( cmd ) );
	while ( undoStack.size() > mMaxStackSize ) {
		undoStack.pop_front();
	}
}

void UndoStack::pushInsert( UndoStackContainer& undoStack, const String& string,
							const TextPosition& position, const Time& time ) {
	pushUndo( undoStack, std::make_unique<TextUndoCommandInsert>( ++mChangeIdCounter, string,
																  position, time ) );
}

void UndoStack::pushRemove( UndoStackContainer& undoStack, const TextRange& range,
							const Time& time ) {
	pushUndo( undoStack,
			  std::make_unique<TextUndoCommandRemove>( ++mChangeIdCounter, range, time ) );
}

void UndoStack::pushSelection( UndoStackContainer& undoStack, const TextRange& selection,
							   const Time& time ) {
	pushUndo( undoStack,
			  std::make_unique<TextUndoCommandSelection>( ++mChangeIdCounter, selection, time ) );
}

void UndoStack::popUndo( UndoStackContainer& undoStack, UndoStackContainer& redoStack ) {
	if ( undoStack.empty() )
		return;

	auto cmd = std::move( undoStack.back() );
	undoStack.pop_back();

	switch ( cmd->getType() ) {
		case TextUndoCommandType::Insert: {
			TextUndoCommandInsert* insert = static_cast<TextUndoCommandInsert*>( cmd.get() );
			mDoc->insert( insert->getPosition(), insert->getText(), redoStack,
						  cmd->getTimestamp() );
			break;
		}
		case TextUndoCommandType::Remove: {
			TextUndoCommandRemove* remove = static_cast<TextUndoCommandRemove*>( cmd.get() );
			mDoc->remove( remove->getRange(), redoStack, cmd->getTimestamp() );
			break;
		}
		case TextUndoCommandType::Selection: {
			TextUndoCommandSelection* selection =
				static_cast<TextUndoCommandSelection*>( cmd.get() );
			mDoc->setSelection( selection->getSelection() );
			break;
		}
	}

	if ( !undoStack.empty() &&
		 eeabs( ( cmd->getTimestamp() - undoStack.back()->getTimestamp() ).asMilliseconds() ) <
			 mMergeTimeout.asMilliseconds() ) {
		popUndo( undoStack, redoStack );
	}
}

void UndoStack::undo() {
	popUndo( mUndoStack, mRedoStack );
}

void UndoStack::redo() {
	popUndo( mRedoStack, mUndoStack );
}

const Uint32& UndoStack::getMaxStackSize() const {
	return mMaxStackSize;
}

const Time& UndoStack::getMergeTimeout() const {
	return mMergeTimeout;
}

void UndoStack::setMergeTimeout( const Time& mergeTimeout ) {
	mMergeTimeout = mergeTimeout;
}

Uint64 UndoStack::getCurrentChangeId() const {
	if ( mUndoStack.empty() )
		return 0;
	return mUndoStack.back()->getId();
}

UndoStackContainer& UndoStack::getUndoStackContainer() {
	return mUndoStack;
}

UndoStackContainer& UndoStack::getRedoStackContainer() {
	return mRedoStack;
}

}}} // namespace EE::UI::Doc
