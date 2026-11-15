#include <eepp/window/runtime.hpp>

#include <cstdlib>
#include <cstring>
#include <eepp/core/string.hpp>
#include <limits>

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

bool Runtime::forceHiddenWindows() {
	const char* value = std::getenv( "EEPP_WINDOW_HIDDEN" );
	return value && 0 == std::strcmp( value, "1" );
}

bool Runtime::forceSoftwareRendering() {
	const char* value = std::getenv( "EEPP_FORCE_SOFTWARE_RENDERING" );
	return value && *value && 0 != std::strcmp( value, "0" );
}

Uint32 Runtime::resolveMultisamples( Uint32 requested ) {
	const char* value = std::getenv( "EEPP_MSAA" );
	Uint32 samples = 0;
	if ( value && String::fromString( samples, std::string_view( value ) ) &&
		 samples <= static_cast<Uint32>( std::numeric_limits<Int32>::max() ) )
		return samples;
	return requested;
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
