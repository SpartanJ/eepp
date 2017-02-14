#include <eepp/window/cursormanager.hpp>

namespace EE { namespace Window {

CursorManager::CursorManager( EE::Window::Window * window ) :
	mWindow( window ),
	mCurrent( NULL ),
	mSysCursor( SYS_CURSOR_NONE ),
	mCursors(),
	mCurSysCursor( SYS_CURSOR_NONE ),
	mVisible( true )
{
	InitGlobalCursors();
}

CursorManager::~CursorManager() {
	for ( CursorsList::iterator it = mCursors.begin(); it != mCursors.end(); ++it ) {
		Cursor * tCursor = *it;
		eeSAFE_DELETE( tCursor );
	}
}

Cursor * CursorManager::Add( Cursor * cursor ) {
	mCursors.insert( cursor );
	return cursor;
}

void CursorManager::Remove( Cursor * cursor, bool Delete ) {
	mCursors.erase( cursor );

	if ( Delete )
		eeSAFE_DELETE( cursor );
}

void CursorManager::Remove( const std::string& name, bool Delete ) {
	Remove( String::hash( name ), Delete );
}

void CursorManager::Remove( const Uint32& id, bool Delete ) {
	for ( CursorsList::iterator it = mCursors.begin(); it != mCursors.end(); ++it ) {
		if ( (*it)->Id() == id ) {
			Remove( (*it), Delete );
			break;	
		}
	}
}

Cursor * CursorManager::Get( const std::string& name ) {
	return GetById( String::hash( name ) );
}

Cursor * CursorManager::GetById( const Uint32& id ) {
	for ( CursorsList::iterator it = mCursors.begin(); it != mCursors.end(); ++it ) {
		if ( (*it)->Id() == id ) {
			return (*it);
		}
	}
	
	return NULL;
}

void CursorManager::Set( const std::string& name ) {
	SetById( String::hash( name ) );
}

void CursorManager::SetById( const Uint32& id ) {
	for ( CursorsList::iterator it = mCursors.begin(); it != mCursors.end(); ++it ) {
		if ( (*it)->Id() == id ) {
			Set( *it );
			break;	
		}
	}
}

void CursorManager::SetGlobalCursor( EE_CURSOR_TYPE cursor, Cursor * fromCursor ) {
	if ( cursor < EE_CURSOR_COUNT ) {
		mGlobalCursors[ cursor ].SysCur	= SYS_CURSOR_NONE;
		mGlobalCursors[ cursor ].Cur	= fromCursor;
	}
}

void CursorManager::SetGlobalCursor( EE_CURSOR_TYPE cursor, EE_SYSTEM_CURSOR fromCursor ) {
	if ( cursor < EE_CURSOR_COUNT ) {
		mGlobalCursors[ cursor ].SysCur	= fromCursor;
		mGlobalCursors[ cursor ].Cur	= NULL;
	}
}

void CursorManager::Set( EE_CURSOR_TYPE cursor ) {
	if ( cursor < EE_CURSOR_COUNT ) {
		GlobalCursor& Cursor = mGlobalCursors[ cursor ];

		if ( SYS_CURSOR_NONE != Cursor.SysCur ) {
			Set( Cursor.SysCur );
		} else if ( NULL != Cursor.Cur ) {
			Set( Cursor.Cur );
		}
	}
}

bool CursorManager::Visible() {
	return mVisible;	
}

Cursor * CursorManager::Current() const {
	return mCurrent;
}

EE_SYSTEM_CURSOR CursorManager::CurrentSysCursor() const {
	return mSysCursor;
}

bool CursorManager::CurrentIsSysCursor() const {
	return mCurSysCursor;
}

void CursorManager::InitGlobalCursors() {
	for ( int i = 0; i < EE_CURSOR_COUNT; i++ ) {
		mGlobalCursors[ i ].SysCur = static_cast<EE_SYSTEM_CURSOR>( i );
	}
}

}}
 
