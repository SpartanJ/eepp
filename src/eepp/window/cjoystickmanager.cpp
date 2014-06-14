#include <eepp/window/cjoystickmanager.hpp>

namespace EE { namespace Window {

cJoystickManager::cJoystickManager() :
	mInit(false),
	mCount(0)
{
	for ( Uint32 i = 0; i < MAX_JOYSTICKS; i++ )
		mJoysticks[i] = NULL;

	Open();
}

cJoystickManager::~cJoystickManager() {
	for ( Uint32 i = 0; i < Count(); i++ )
		eeSAFE_DELETE( mJoysticks[i] );

	Close();
}

Uint32 cJoystickManager::Count() {
	return mCount;
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

void cJoystickManager::Close() {
}

void cJoystickManager::Open() {
}

}}
