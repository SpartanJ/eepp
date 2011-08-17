#include "ccursormanager.hpp"

namespace EE { namespace Window {

cCursorManager::cCursorManager( cWindow * window ) :
	mWindow( window ),
	mCurrent( NULL ),
	mSysCursor( SYS_CURSOR_NONE ),
	mCursors(),
	mCurSysCursor( SYS_CURSOR_NONE ),
	mVisible( true )
{
}

cCursorManager::~cCursorManager() {
	for ( CursorsList::iterator it = mCursors.begin(); it != mCursors.end(); ++it ) {
		cCursor * tCursor = *it;
		eeSAFE_DELETE( tCursor );
	}
}

cCursor * cCursorManager::Add( cCursor * cursor ) {
	mCursors.insert( cursor );
	return cursor;
}

void cCursorManager::Remove( cCursor * cursor, bool Delete ) {
	mCursors.erase( cursor );

	if ( Delete )
		eeSAFE_DELETE( cursor );
}

void cCursorManager::Remove( const std::string& name, bool Delete ) {
	Remove( MakeHash( name ), Delete );
}

void cCursorManager::Remove( const Uint32& id, bool Delete ) {
	for ( CursorsList::iterator it = mCursors.begin(); it != mCursors.end(); ++it ) {
		if ( (*it)->Id() == id ) {
			Remove( (*it), Delete );
			break;	
		}
	}
}

cCursor * cCursorManager::Get( const std::string& name ) {
	return Get( MakeHash( name ) );
}

cCursor * cCursorManager::Get( const Uint32& id ) {
	for ( CursorsList::iterator it = mCursors.begin(); it != mCursors.end(); ++it ) {
		if ( (*it)->Id() == id ) {
			return (*it);
		}
	}
	
	return NULL;
}

void cCursorManager::Set( const std::string& name ) {
	Set( MakeHash( name ) );
}

void cCursorManager::Set( const Uint32& id ) {
	for ( CursorsList::iterator it = mCursors.begin(); it != mCursors.end(); ++it ) {
		if ( (*it)->Id() == id ) {
			Set( *it );
			break;	
		}
	}
}

bool cCursorManager::Visible() {
	return mVisible;	
}

cCursor * cCursorManager::Current() const {
	return mCurrent;
}

EE_SYSTEM_CURSOR cCursorManager::CurrentSysCursor() const {
	return mSysCursor;
}

bool cCursorManager::CurrentIsSysCursor() const {
	return mCurSysCursor;
}

}}
 
