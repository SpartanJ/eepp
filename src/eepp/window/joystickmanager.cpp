#include <eepp/window/joystickmanager.hpp>

namespace EE { namespace Window {

JoystickManager::JoystickManager() :
	mInit(false),
	mCount(0)
{
	for ( Uint32 i = 0; i < MAX_JOYSTICKS; i++ )
		mJoysticks[i] = NULL;

	Open();
}

JoystickManager::~JoystickManager() {
	for ( Uint32 i = 0; i < Count(); i++ )
		eeSAFE_DELETE( mJoysticks[i] );

	Close();
}

Uint32 JoystickManager::Count() {
	return mCount;
}

Joystick * JoystickManager::GetJoystick( const Uint32& index ) {
	if ( index < MAX_JOYSTICKS )
		return mJoysticks[ index ];

	return NULL;
}

void JoystickManager::Rescan() {
	Close();

	Open();
}

void JoystickManager::Close() {
}

void JoystickManager::Open() {
}

}}
