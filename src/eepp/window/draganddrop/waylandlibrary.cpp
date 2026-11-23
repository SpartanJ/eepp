#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
#include <eepp/window/draganddrop/waylandlibrary.hpp>

#ifdef EE_HAS_WAYLAND_CLIENT

#include <bitset>
#include <chrono>
#include <cstring>
#include <memory>
#include <poll.h>

namespace EE { namespace Window { namespace Private {

bool WaylandLibrary::hasGlobals( std::initializer_list<const char*> names ) {
	struct RegistryState {
		std::initializer_list<const char*> names;
		std::bitset<32> found;
		bool done{ false };
	} state{ names, {} };

	WaylandLibrary library;
	if ( names.size() > state.found.size() || !library.open() )
		return false;

#define EE_WL_SYMBOL( name )                                                             \
	auto name = reinterpret_cast<decltype( &::name )>( dlsym( library.handle, #name ) ); \
	if ( !name )                                                                         \
		return false;
	EE_WL_SYMBOL( wl_display_connect )
	EE_WL_SYMBOL( wl_display_disconnect )
	EE_WL_SYMBOL( wl_display_dispatch )
	EE_WL_SYMBOL( wl_display_flush )
	EE_WL_SYMBOL( wl_display_get_fd )
	EE_WL_SYMBOL( wl_proxy_add_listener )
	EE_WL_SYMBOL( wl_proxy_destroy )
	EE_WL_SYMBOL( wl_proxy_marshal_constructor )
#undef EE_WL_SYMBOL
	auto* registryInterface =
		static_cast<const wl_interface*>( dlsym( library.handle, "wl_registry_interface" ) );
	auto* callbackInterface =
		static_cast<const wl_interface*>( dlsym( library.handle, "wl_callback_interface" ) );
	if ( !registryInterface || !callbackInterface )
		return false;

	std::unique_ptr<wl_display, decltype( wl_display_disconnect )> display(
		wl_display_connect( nullptr ), wl_display_disconnect );
	if ( !display )
		return false;
	auto* displayProxy = reinterpret_cast<wl_proxy*>( display.get() );
	std::unique_ptr<wl_proxy, decltype( wl_proxy_destroy )> registry(
		wl_proxy_marshal_constructor( displayProxy, WL_DISPLAY_GET_REGISTRY, registryInterface,
									  nullptr ),
		wl_proxy_destroy );
	std::unique_ptr<wl_proxy, decltype( wl_proxy_destroy )> sync(
		wl_proxy_marshal_constructor( displayProxy, WL_DISPLAY_SYNC, callbackInterface, nullptr ),
		wl_proxy_destroy );
	if ( !registry || !sync )
		return false;
	static constexpr wl_registry_listener registryListener{
		[]( void* data, wl_registry*, uint32_t, const char* name, uint32_t ) {
			auto& state = *static_cast<RegistryState*>( data );
			size_t index = 0;
			for ( const char* required : state.names ) {
				if ( std::strcmp( required, name ) == 0 )
					state.found.set( index );
				++index;
			}
		},
		[]( void*, wl_registry*, uint32_t ) {},
	};
	static constexpr wl_callback_listener syncListener{ []( void* data, wl_callback*, uint32_t ) {
		static_cast<RegistryState*>( data )->done = true;
	} };
	if ( wl_proxy_add_listener( registry.get(),
								reinterpret_cast<void ( ** )()>(
									const_cast<wl_registry_listener*>( &registryListener ) ),
								&state ) < 0 ||
		 wl_proxy_add_listener(
			 sync.get(),
			 reinterpret_cast<void ( ** )()>( const_cast<wl_callback_listener*>( &syncListener ) ),
			 &state ) < 0 ||
		 wl_display_flush( display.get() ) < 0 )
		return false;

	// A stale socket may accept a connection and never answer the registry request.
	using namespace std::chrono;
	const auto deadline = steady_clock::now() + milliseconds( 250 );
	pollfd socket{ wl_display_get_fd( display.get() ), POLLIN, 0 };
	while ( !state.done ) {
		const auto remaining = ceil<milliseconds>( deadline - steady_clock::now() ).count();
		if ( remaining <= 0 || poll( &socket, 1, static_cast<int>( remaining ) ) <= 0 ||
			 socket.revents != POLLIN || wl_display_dispatch( display.get() ) < 0 )
			return false;
	}
	return state.found.count() == names.size();
}

}}} // namespace EE::Window::Private

#endif
#endif
