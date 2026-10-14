#include "native_drag_and_drop.hpp"
#include <eepp/system/sys.hpp>

EE_MAIN_FUNC int main( int argc, char** argv ) {
	return EE::Examples::runNativeDragDemo( EE::System::Sys::parseArguments( argc, argv ) );
}
