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
			mIds.assign( ids, ids + count );
			mCount = static_cast<Uint32>( count );
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
