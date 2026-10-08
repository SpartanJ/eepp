#include <eepp/window/runtime.hpp>

#include <cstdlib>
#include <cstring>

namespace EE { namespace Window {

namespace {

RuntimeMode resolveRuntimeMode() {
	const char* value = std::getenv( "EEPP_RUNTIME" );
	if ( nullptr == value )
		return RuntimeMode::Native;
	if ( 0 == std::strcmp( value, "headless" ) )
		return RuntimeMode::Headless;
	if ( 0 == std::strcmp( value, "terminal" ) )
		return RuntimeMode::Terminal;
	return RuntimeMode::Native;
}

} // namespace

RuntimeMode Runtime::mode() {
	static const RuntimeMode runtimeMode = resolveRuntimeMode();
	return runtimeMode;
}

bool Runtime::isOffscreen() {
	return mode() != RuntimeMode::Native;
}

const char* Runtime::modeName() {
	switch ( mode() ) {
		case RuntimeMode::Headless:
			return "headless";
		case RuntimeMode::Terminal:
			return "terminal";
		case RuntimeMode::Native:
		default:
			return "native";
	}
}

}} // namespace EE::Window
