#include <eepp/window/joystickmanager.hpp>

namespace EE { namespace Window {

JoystickManager::JoystickManager() : mInit( false ), mCount( 0 ) {
	for ( Uint32 i = 0; i < MAX_JOYSTICKS; i++ )
		mJoysticks[i] = NULL;

	open();
}

JoystickManager::~JoystickManager() {
	for ( Uint32 i = 0; i < getCount(); i++ )
		eeSAFE_DELETE( mJoysticks[i] );

	close();
}

Uint32 JoystickManager::getCount() {
	return mCount;
}

Joystick* JoystickManager::getJoystick( const Uint32& index ) {
	if ( index < MAX_JOYSTICKS )
		return mJoysticks[index];

	return NULL;
}

void JoystickManager::rescan() {
	close();

	open();
}

void JoystickManager::close() {}

void JoystickManager::open() {}

}} // namespace EE::Window
