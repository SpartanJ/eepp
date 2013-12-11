#include <eepp/window/ccursormanager.hpp>

namespace EE { namespace Window {

cCursorManager::cCursorManager( cWindow * window ) :
	mWindow( window ),
	mCurrent( NULL ),
	mSysCursor( SYS_CURSOR_NONE ),
	mCursors(),
	mCurSysCursor( SYS_CURSOR_NONE ),
	mVisible( true )
{
	InitGlobalCursors();
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
	Remove( String::Hash( name ), Delete );
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
	return GetById( String::Hash( name ) );
}

cCursor * cCursorManager::GetById( const Uint32& id ) {
	for ( CursorsList::iterator it = mCursors.begin(); it != mCursors.end(); ++it ) {
		if ( (*it)->Id() == id ) {
			return (*it);
		}
	}
	
	return NULL;
}

void cCursorManager::Set( const std::string& name ) {
	SetById( String::Hash( name ) );
}

void cCursorManager::SetById( const Uint32& id ) {
	for ( CursorsList::iterator it = mCursors.begin(); it != mCursors.end(); ++it ) {
		if ( (*it)->Id() == id ) {
			Set( *it );
			break;	
		}
	}
}

void cCursorManager::SetGlobalCursor( EE_CURSOR_TYPE cursor, cCursor * fromCursor ) {
	if ( cursor < EE_CURSOR_COUNT ) {
		mGlobalCursors[ cursor ].SysCur	= SYS_CURSOR_NONE;
		mGlobalCursors[ cursor ].Cur	= fromCursor;
	}
}

void cCursorManager::SetGlobalCursor( EE_CURSOR_TYPE cursor, EE_SYSTEM_CURSOR fromCursor ) {
	if ( cursor < EE_CURSOR_COUNT ) {
		mGlobalCursors[ cursor ].SysCur	= fromCursor;
		mGlobalCursors[ cursor ].Cur	= NULL;
	}
}

void cCursorManager::Set( EE_CURSOR_TYPE cursor ) {
	if ( cursor < EE_CURSOR_COUNT ) {
		GlobalCursor& Cursor = mGlobalCursors[ cursor ];

		if ( SYS_CURSOR_NONE != Cursor.SysCur ) {
			Set( Cursor.SysCur );
		} else if ( NULL != Cursor.Cur ) {
			Set( Cursor.Cur );
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

void cCursorManager::InitGlobalCursors() {
	for ( int i = 0; i < EE_CURSOR_COUNT; i++ ) {
		mGlobalCursors[ i ].SysCur = static_cast<EE_SYSTEM_CURSOR>( i );
	}
}

}}
 
