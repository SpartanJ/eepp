#include <eepp/core/debug.hpp>
#include <eepp/core/memorymanager.hpp>
#include <eepp/ui/undostack.hpp>

namespace EE { namespace UI {

UndoCommand::UndoCommand( const std::string& text, UndoCommand* parent ) : UndoCommand( parent ) {
	setText( text );
}

UndoCommand::UndoCommand( UndoCommand* parent ) {
	if ( parent != nullptr )
		parent->mChilds.push_back( this );
}

UndoCommand::~UndoCommand() {
	for ( const auto& child : mChilds )
		delete child;
}

bool UndoCommand::isObsolete() const {
	return mObsolete;
}

void UndoCommand::setObsolete( bool obsolete ) {
	mObsolete = obsolete;
}

int UndoCommand::id() const {
	return -1;
}

bool UndoCommand::mergeWith( const UndoCommand* ) {
	return false;
}

void UndoCommand::redo() {
	for ( std::size_t i = 0; i < mChilds.size(); ++i )
		mChilds.at( i )->redo();
}

void UndoCommand::undo() {
	for ( int i = mChilds.size() - 1; i >= 0; --i )
		mChilds.at( i )->undo();
}

std::string UndoCommand::text() const {
	return mText;
}

std::string UndoCommand::actionText() const {
	return mActionText;
}

void UndoCommand::setText( const std::string& text ) {
	auto cdpos = text.find_first_of( '\n' );
	if ( cdpos != std::string::npos ) {
		mText = text.substr( 0, cdpos );
		mActionText = text.substr( cdpos + 1 );
	} else {
		mText = text;
		mActionText = text;
	}
}

int UndoCommand::childCount() const {
	return mChilds.size();
}

const UndoCommand* UndoCommand::child( int index ) const {
	if ( index < 0 || index >= static_cast<int>( mChilds.size() ) )
		return nullptr;
	return mChilds.at( index );
}

void UndoStack::setIndex( int idx, bool clean ) {
	bool wasClean = mIndex == mCleanIndex;

	if ( idx != mIndex ) {
		mIndex = idx;
		indexChanged( mIndex );
		canUndoChanged( canUndo() );
		undoTextChanged( undoText() );
		canRedoChanged( canRedo() );
		redoTextChanged( redoText() );
	}

	if ( clean )
		mCleanIndex = mIndex;

	bool isClean = mIndex == mCleanIndex;
	if ( isClean != wasClean )
		cleanChanged( isClean );
}

bool UndoStack::checkUndoLimit() {
	if ( mUndoLimit <= 0 || !mMacroStack.empty() ||
		 mUndoLimit >= static_cast<int>( mCommands.size() ) )
		return false;

	int delCount = mCommands.size() - mUndoLimit;

	for ( int i = 0; i < delCount; ++i ) {
		delete mCommands.front();
		mCommands.pop_front();
	}

	mIndex -= delCount;
	if ( mCleanIndex != -1 ) {
		if ( mCleanIndex < delCount )
			mCleanIndex = -1; // we've deleted the clean command
		else
			mCleanIndex -= delCount;
	}

	return true;
}

UndoStack::~UndoStack() {
	clear();
}

void UndoStack::clear() {
	if ( mCommands.empty() )
		return;

	bool wasClean = isClean();

	mMacroStack.clear();
	for ( const auto& cmd : mCommands )
		delete cmd;
	mCommands.clear();

	mIndex = 0;
	mCleanIndex = 0;

	indexChanged( 0 );
	canUndoChanged( false );
	undoTextChanged( std::string() );
	canRedoChanged( false );
	redoTextChanged( std::string() );

	if ( !wasClean )
		cleanChanged( true );
}

void UndoStack::push( UndoCommand* cmd ) {
	if ( !cmd->isObsolete() )
		cmd->redo();

	bool macro = !mMacroStack.empty();

	UndoCommand* cur = nullptr;
	if ( macro ) {
		UndoCommand* macroCmd = mMacroStack.back();
		if ( !macroCmd->mChilds.empty() )
			cur = macroCmd->mChilds.back();
	} else {
		if ( mIndex > 0 )
			cur = mCommands.at( mIndex - 1 );
		while ( mIndex < static_cast<int>( mCommands.size() ) ) {
			delete mCommands.back();
			mCommands.pop_back();
		}
		if ( mCleanIndex > mIndex )
			mCleanIndex = -1; // we've deleted the clean state
	}

	bool tryMerge = cur != nullptr && cur->id() != -1 && cur->id() == cmd->id() &&
					( macro || mIndex != mCleanIndex );

	if ( tryMerge && cur->mergeWith( cmd ) ) {
		delete cmd;

		if ( macro ) {
			if ( cur->isObsolete() ) {
				delete mMacroStack.back()->mChilds.back();
				mMacroStack.back()->mChilds.pop_back();
			}
		} else {
			if ( cur->isObsolete() ) {
				delete mCommands.back();
				mCommands.pop_back();

				setIndex( mIndex - 1, false );
			} else {
				indexChanged( mIndex );
				canUndoChanged( canUndo() );
				undoTextChanged( undoText() );
				canRedoChanged( canRedo() );
				redoTextChanged( redoText() );
			}
		}
	} else if ( cmd->isObsolete() ) {
		delete cmd; // command should be deleted and NOT added to the stack
	} else {
		if ( macro ) {
			mMacroStack.back()->mChilds.push_back( cmd );
		} else {
			mCommands.push_back( cmd );
			checkUndoLimit();
			setIndex( mIndex + 1, false );
		}
	}
}

void UndoStack::setClean() {
	if ( unlikely( !mMacroStack.empty() ) ) {
		eePRINTL( "UndoStack::setClean(): cannot set clean in the middle of a macro" );
		return;
	}

	setIndex( mIndex, true );
}

void UndoStack::resetClean() {
	const bool wasClean = isClean();
	mCleanIndex = -1;
	if ( wasClean )
		cleanChanged( false );
}

bool UndoStack::isClean() const {
	if ( !mMacroStack.empty() )
		return false;
	return mCleanIndex == mIndex;
}

int UndoStack::cleanIndex() const {
	return mCleanIndex;
}

void UndoStack::undo() {
	if ( mIndex == 0 )
		return;

	if ( unlikely( !mMacroStack.empty() ) ) {
		eePRINTL( "UndoStack::undo(): cannot undo in the middle of a macro" );
		return;
	}

	int idx = mIndex - 1;
	UndoCommand* cmd = mCommands.at( idx );

	if ( !cmd->isObsolete() )
		cmd->undo();

	if ( cmd->isObsolete() ) { // A separate check is done b/c the undo command may set obsolete
							   // flag
		delete mCommands[idx];
		mCommands.erase( mCommands.begin() + idx );

		if ( mCleanIndex > idx )
			resetClean();
	}

	setIndex( idx, false );
}

void UndoStack::redo() {
	if ( mIndex == static_cast<int>( mCommands.size() ) )
		return;

	if ( unlikely( !mMacroStack.empty() ) ) {
		eePRINTL( "UndoStack::redo(): cannot redo in the middle of a macro" );
		return;
	}

	int idx = mIndex;
	UndoCommand* cmd = mCommands.at( idx );

	if ( !cmd->isObsolete() )
		cmd->redo(); // A separate check is done b/c the undo command may set obsolete flag

	if ( cmd->isObsolete() ) {
		delete mCommands[idx];
		mCommands.erase( mCommands.begin() + idx );

		if ( mCleanIndex > idx )
			resetClean();
	} else {
		setIndex( mIndex + 1, false );
	}
}

UndoStack::EventId UndoStack::addEventListener( EventType type, EventCb cb ) {
	auto eventId = ++mNextEventId;
	mEventsCbs[type].insert( { eventId, std::move( cb ) } );
	return eventId;
}

UndoStack::EventId UndoStack::on( EventType type, EventCb cb ) {
	return addEventListener( type, std::move( cb ) );
}

void UndoStack::removeEventListener( EventId id ) {
	for ( auto& eventType : mEventsCbs )
		if ( eventType.second.erase( id ) > 0 )
			break;
}

int UndoStack::count() const {
	return mCommands.size();
}

int UndoStack::index() const {
	return mIndex;
}

void UndoStack::setIndex( int idx ) {
	if ( unlikely( !mMacroStack.empty() ) ) {
		eePRINTL( "UndoStack::setIndex(): cannot set index in the middle of a macro" );
		return;
	}

	if ( idx < 0 )
		idx = 0;
	else if ( idx > static_cast<int>( mCommands.size() ) )
		idx = mCommands.size();

	int i = mIndex;
	while ( i < idx ) {
		UndoCommand* cmd = mCommands.at( i );

		if ( !cmd->isObsolete() )
			cmd->redo(); // A separate check is done b/c the undo command may set obsolete flag

		if ( cmd->isObsolete() ) {
			delete mCommands[i];
			mCommands.erase( mCommands.begin() + i );

			if ( mCleanIndex > i )
				resetClean();

			idx--; // Subtract from idx because we removed a command
		} else {
			i++;
		}
	}

	while ( i > idx ) {
		UndoCommand* cmd = mCommands.at( --i );

		cmd->undo();
		if ( cmd->isObsolete() ) {
			delete mCommands[i];
			mCommands.erase( mCommands.begin() + i );

			if ( mCleanIndex > i )
				resetClean();
		}
	}

	setIndex( idx, false );
}

bool UndoStack::canUndo() const {
	if ( !mMacroStack.empty() )
		return false;
	return mIndex > 0;
}

bool UndoStack::canRedo() const {
	if ( !mMacroStack.empty() )
		return false;
	return mIndex < static_cast<int>( mCommands.size() );
}

std::string UndoStack::undoText() const {
	if ( !mMacroStack.empty() )
		return std::string();
	if ( mIndex > 0 )
		return mCommands.at( mIndex - 1 )->actionText();
	return std::string();
}

std::string UndoStack::redoText() const {
	if ( !mMacroStack.empty() )
		return std::string();
	if ( mIndex < static_cast<int>( mCommands.size() ) )
		return mCommands.at( mIndex )->actionText();
	return std::string();
}

void UndoStack::beginMacro( const std::string& text ) {
	UndoCommand* cmd = new UndoCommand();
	cmd->setText( text );

	if ( mMacroStack.empty() ) {
		while ( mIndex < static_cast<int>( mCommands.size() ) ) {
			delete mCommands.back();
			mCommands.pop_back();
		}
		if ( mCleanIndex > mIndex )
			mCleanIndex = -1; // we've deleted the clean state
		mCommands.push_back( cmd );
	} else {
		mMacroStack.back()->mChilds.push_back( cmd );
	}
	mMacroStack.push_back( cmd );

	if ( mMacroStack.size() == 1 ) {
		canUndoChanged( false );
		undoTextChanged( std::string() );
		canRedoChanged( false );
		redoTextChanged( std::string() );
	}
}

void UndoStack::endMacro() {
	if ( unlikely( mMacroStack.empty() ) ) {
		eePRINTL( "UndoStack::endMacro(): no matching beginMacro()" );
		return;
	}

	mMacroStack.pop_back();

	if ( mMacroStack.empty() ) {
		checkUndoLimit();
		setIndex( mIndex + 1, false );
	}
}

const UndoCommand* UndoStack::command( int index ) const {
	if ( index < 0 || index >= static_cast<int>( mCommands.size() ) )
		return nullptr;
	return mCommands.at( index );
}

std::string UndoStack::text( int idx ) const {
	if ( idx < 0 || idx >= static_cast<int>( mCommands.size() ) )
		return std::string();
	return mCommands.at( idx )->text();
}

void UndoStack::setUndoLimit( int limit ) {
	if ( unlikely( !mCommands.empty() ) ) {
		eePRINTL(
			"UndoStack::setUndoLimit(): an undo limit can only be set when the stack is empty" );
		return;
	}

	if ( limit == mUndoLimit )
		return;
	mUndoLimit = limit;
	checkUndoLimit();
}

int UndoStack::undoLimit() const {
	return mUndoLimit;
}

void UndoStack::sendEvent( const Event* event ) {
	if ( 0 != mEventsCbs.count( event->getType() ) ) {
		auto eventMap = mEventsCbs[event->getType()];
		if ( eventMap.begin() != eventMap.end() ) {
			for ( const auto& e : eventMap ) {
				const_cast<Event*>( event )->cbId = e.first;
				e.second( event );
			}
		}
	}
}

void UndoStack::indexChanged( int idx ) {
	EventIndexChanged event( idx );
	sendEvent( &event );
}

void UndoStack::cleanChanged( bool clean ) {
	EventCleanChanged event( clean );
	sendEvent( &event );
}

void UndoStack::canUndoChanged( bool canUndo ) {
	EventCanUndoChanged event( canUndo );
	sendEvent( &event );
}

void UndoStack::canRedoChanged( bool canRedo ) {
	EventCanRedoChanged event( canRedo );
	sendEvent( &event );
}

void UndoStack::undoTextChanged( const std::string& undoText ) {
	EventUndoTextChanged event( undoText );
	sendEvent( &event );
}

void UndoStack::redoTextChanged( const std::string& redoText ) {
	EventRedoTextChanged event( redoText );
	sendEvent( &event );
}

}} // namespace EE::UI
