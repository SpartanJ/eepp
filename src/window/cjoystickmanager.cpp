#include "cjoystickmanager.hpp"

namespace EE { namespace Window {

cJoystickManager::cJoystickManager() :
	mInit(false),
	mCount(0)
{
	for ( eeUint i = 0; i < MAX_JOYSTICKS; i++ )
		mJoysticks[i] = NULL;

	Open();
}

cJoystickManager::~cJoystickManager() {
	for ( eeUint i = 0; i < Count(); i++ )
		eeSAFE_DELETE( mJoysticks[i] );

	Close();
}

void cJoystickManager::Update() {
	if ( mInit ) {
		SDL_JoystickUpdate();

		for ( eeUint i = 0; i < mCount; i++ )
			if ( NULL != mJoysticks[i] )
				mJoysticks[i]->Update();
	}
}

Uint32 cJoystickManager::Count() {
	return SDL_NumJoysticks();
}

void cJoystickManager::Open() {
	eeInt error = SDL_InitSubSystem( SDL_INIT_JOYSTICK );

	if ( !error ) {
		mCount = Count();

		for ( eeUint i = 0; i < mCount; i++ )
			Create(i);

		mInit = true;
	}
}

void cJoystickManager::Create( const Uint32& index ) {
	if ( NULL != mJoysticks[ index ] )
		mJoysticks[ index ]->ReOpen();
	else
		mJoysticks[ index ] = new cJoystick( index );
}

void cJoystickManager::Close() {
	if ( SDL_WasInit( SDL_INIT_JOYSTICK ) )
		SDL_QuitSubSystem( SDL_INIT_JOYSTICK );
}

cJoystick * cJoystickManager::GetJoystick( const Uint32& index ) {
	if ( index < MAX_JOYSTICKS )
		return mJoysticks[ index ];

	return NULL;
}

void cJoystickManager::Rescan() {
	Close();

	Open();
}

}}
