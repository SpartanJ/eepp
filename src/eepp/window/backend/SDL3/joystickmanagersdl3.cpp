#include <eepp/window/backend/SDL3/joystickmanagersdl3.hpp>
#include <eepp/window/backend/SDL3/joysticksdl3.hpp>

#ifdef EE_BACKEND_SDL3

namespace EE { namespace Window { namespace Backend { namespace SDL3 {

namespace {
void closeSubsystem() {
#if EE_PLATFORM != EE_PLATFORM_MACOS && EE_PLATFORM != EE_PLATFORM_IOS
	if ( SDL_WasInit( SDL_INIT_JOYSTICK ) )
		SDL_QuitSubSystem( SDL_INIT_JOYSTICK );
#endif
}
} // namespace

JoystickManagerSDL::JoystickManagerSDL() :
	JoystickManager(), mAsyncInit( &JoystickManagerSDL::openAsync, this ) {}

JoystickManagerSDL::~JoystickManagerSDL() {
	for ( Uint32 i = 0; i < mCount; i++ )
		eeSAFE_DELETE( mJoysticks[i] );
	closeSubsystem();
	mInit = false;
}

void JoystickManagerSDL::update() {
	if ( mInit ) {
		SDL_UpdateJoysticks();
		for ( Uint32 i = 0; i < mCount; i++ )
			if ( nullptr != mJoysticks[i] )
				mJoysticks[i]->update();
	}
}

void JoystickManagerSDL::openAsync() {
	Sys::sleep( Milliseconds( 500 ) );

	int error = SDL_InitSubSystem( SDL_INIT_JOYSTICK );

	if ( !error ) {
		int count = 0;
		SDL_JoystickID* ids = SDL_GetJoysticks( &count );
		if ( ids ) {
			mCount = static_cast<Uint32>( count );
			if ( mCount > MAX_JOYSTICKS ) {
				Log::warning( "Too many joysticks detected (%d), only the first %d will be used.",
							  mCount, MAX_JOYSTICKS );
				mCount = MAX_JOYSTICKS;
			}
			mIds.assign( ids, ids + mCount );
			SDL_free( ids );
		} else {
			mCount = 0;
		}

		// Build ID -> index mapping
		mIdToIndex.clear();
		for ( Uint32 i = 0; i < mCount; i++ ) {
			mIdToIndex[mIds[i]] = i;
			create( i );
		}

		mInit = true;

		if ( mOpenCb )
			mOpenCb();
	}
}

void JoystickManagerSDL::open( OpenCb openCb ) {
	mOpenCb = openCb;
	mAsyncInit.launch();
}

void JoystickManagerSDL::close() {
	closeSubsystem();
	mInit = false;
}

void JoystickManagerSDL::rescan() {
	if ( !mInit )
		return;

	int count = 0;
	SDL_JoystickID* ids = SDL_GetJoysticks( &count );
	if ( ids ) {
		for ( int i = 0; i < count; i++ ) {
			if ( mIdToIndex.find( ids[i] ) == mIdToIndex.end() ) {
				addJoystick( ids[i] );
			}
		}
		SDL_free( ids );
	}
}

void JoystickManagerSDL::addJoystick( SDL_JoystickID id ) {
	if ( mCount >= MAX_JOYSTICKS ) {
		Log::warning( "Cannot add more joysticks, MAX_JOYSTICKS limit reached." );
		return;
	}

	if ( mIdToIndex.find( id ) != mIdToIndex.end() )
		return;

	mIds.push_back( id );
	Uint32 index = mCount;
	mIdToIndex[id] = index;
	mCount++;
	create( index );

	Log::info( "Joystick added: %d", id );
}

void JoystickManagerSDL::removeJoystick( SDL_JoystickID id ) {
	auto it = mIdToIndex.find( id );
	if ( it != mIdToIndex.end() ) {
		Uint32 index = it->second;
		Log::info( "Joystick removed: %d (index %d)", id, index );

		eeSAFE_DELETE( mJoysticks[index] );
		mIdToIndex.erase( it );

		// We do not re-order the remaining joysticks to avoid breaking indices
		// But we need to handle the hole.
		// Actually, standard JoystickManager might not handle holes well if it uses a simple array.
	}
}

void JoystickManagerSDL::create( const Uint32& index ) {
	if ( nullptr != mJoysticks[index] )
		mJoysticks[index]->reOpen();
	else
		mJoysticks[index] = eeNew( JoystickSDL, ( index, mIds[index] ) );
}

Uint32 JoystickManagerSDL::getIndexFromID( SDL_JoystickID id ) const {
	auto it = mIdToIndex.find( id );
	if ( it != mIdToIndex.end() )
		return it->second;
	return UINT32_MAX;
}

}}}} // namespace EE::Window::Backend::SDL3

#endif
