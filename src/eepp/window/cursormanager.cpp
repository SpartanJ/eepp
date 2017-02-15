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
	initGlobalCursors();
}

CursorManager::~CursorManager() {
	for ( CursorsList::iterator it = mCursors.begin(); it != mCursors.end(); ++it ) {
		Cursor * tCursor = *it;
		eeSAFE_DELETE( tCursor );
	}
}

Cursor * CursorManager::add( Cursor * cursor ) {
	mCursors.insert( cursor );
	return cursor;
}

void CursorManager::remove( Cursor * cursor, bool Delete ) {
	mCursors.erase( cursor );

	if ( Delete )
		eeSAFE_DELETE( cursor );
}

void CursorManager::remove( const std::string& name, bool Delete ) {
	remove( String::hash( name ), Delete );
}

void CursorManager::remove( const Uint32& id, bool Delete ) {
	for ( CursorsList::iterator it = mCursors.begin(); it != mCursors.end(); ++it ) {
		if ( (*it)->getId() == id ) {
			remove( (*it), Delete );
			break;	
		}
	}
}

Cursor * CursorManager::get( const std::string& name ) {
	return getById( String::hash( name ) );
}

Cursor * CursorManager::getById( const Uint32& id ) {
	for ( CursorsList::iterator it = mCursors.begin(); it != mCursors.end(); ++it ) {
		if ( (*it)->getId() == id ) {
			return (*it);
		}
	}
	
	return NULL;
}

void CursorManager::set( const std::string& name ) {
	setById( String::hash( name ) );
}

void CursorManager::setById( const Uint32& id ) {
	for ( CursorsList::iterator it = mCursors.begin(); it != mCursors.end(); ++it ) {
		if ( (*it)->getId() == id ) {
			set( *it );
			break;	
		}
	}
}

void CursorManager::setGlobalCursor( EE_CURSOR_TYPE cursor, Cursor * fromCursor ) {
	if ( cursor < EE_CURSOR_COUNT ) {
		mGlobalCursors[ cursor ].SysCur	= SYS_CURSOR_NONE;
		mGlobalCursors[ cursor ].Cur	= fromCursor;
	}
}

void CursorManager::setGlobalCursor( EE_CURSOR_TYPE cursor, EE_SYSTEM_CURSOR fromCursor ) {
	if ( cursor < EE_CURSOR_COUNT ) {
		mGlobalCursors[ cursor ].SysCur	= fromCursor;
		mGlobalCursors[ cursor ].Cur	= NULL;
	}
}

void CursorManager::set( EE_CURSOR_TYPE cursor ) {
	if ( cursor < EE_CURSOR_COUNT ) {
		GlobalCursor& Cursor = mGlobalCursors[ cursor ];

		if ( SYS_CURSOR_NONE != Cursor.SysCur ) {
			set( Cursor.SysCur );
		} else if ( NULL != Cursor.Cur ) {
			set( Cursor.Cur );
		}
	}
}

bool CursorManager::visible() {
	return mVisible;	
}

Cursor * CursorManager::getCurrent() const {
	return mCurrent;
}

EE_SYSTEM_CURSOR CursorManager::getCurrentSysCursor() const {
	return mSysCursor;
}

bool CursorManager::currentIsSysCursor() const {
	return mCurSysCursor;
}

void CursorManager::initGlobalCursors() {
	for ( int i = 0; i < EE_CURSOR_COUNT; i++ ) {
		mGlobalCursors[ i ].SysCur = static_cast<EE_SYSTEM_CURSOR>( i );
	}
}

}}
 
