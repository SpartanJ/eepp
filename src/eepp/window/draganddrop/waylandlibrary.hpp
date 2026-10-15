#ifndef EE_WINDOW_WAYLAND_LIBRARY_HPP
#define EE_WINDOW_WAYLAND_LIBRARY_HPP

#include <eepp/config.hpp>

// Missing or older protocol headers disable the backend without adding a build dependency.
#if ( EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD ) && \
	( __has_include( <wayland-client.h> ) || __has_include( <wayland/wayland-client.h> ) )
#if __has_include( <wayland-client.h> )
#include <wayland-client.h>
#else
#include <wayland/wayland-client.h>
#endif

// Generated marshal flags and their dispatch entry point require client headers >= 1.20.
#ifdef WL_MARSHAL_FLAG_DESTROY
#define EE_HAS_WAYLAND_DRAG_SOURCE

#include <algorithm>
#include <dlfcn.h>

namespace EE { namespace Window { namespace Private {

/** Dynamic core protocol dispatch. Generated Wayland inline functions otherwise introduce direct
 * references to libwayland-client's functions and interface descriptors. */
class WaylandLibrary {
  public:
	WaylandLibrary() = default;

	WaylandLibrary( const WaylandLibrary& ) = delete;

	WaylandLibrary& operator=( const WaylandLibrary& ) = delete;

	~WaylandLibrary() {
		if ( handle )
			dlclose( handle );
	}

	bool load() {
		if ( handle )
			return true;
		handle = dlopen( "libwayland-client.so.0", RTLD_NOW | RTLD_LOCAL );
		if ( !handle )
			handle = dlopen( "libwayland-client.so", RTLD_NOW | RTLD_LOCAL );
		if ( !handle )
			return false;
#define EE_WL_FUNCTION( name )                                           \
	name = reinterpret_cast<decltype( name )>( dlsym( handle, #name ) ); \
	if ( !name ) {                                                       \
		dlclose( handle );                                               \
		handle = nullptr;                                                \
		return false;                                                    \
	}
		EE_WL_FUNCTION( wl_proxy_marshal_flags )
		EE_WL_FUNCTION( wl_proxy_add_listener )
		EE_WL_FUNCTION( wl_proxy_get_version )
		EE_WL_FUNCTION( wl_proxy_destroy )
		EE_WL_FUNCTION( wl_proxy_set_queue )
		EE_WL_FUNCTION( wl_proxy_create_wrapper )
		EE_WL_FUNCTION( wl_proxy_wrapper_destroy )
		EE_WL_FUNCTION( wl_display_roundtrip )
		EE_WL_FUNCTION( wl_display_flush )
		EE_WL_FUNCTION( wl_display_get_error )
		EE_WL_FUNCTION( wl_display_create_queue )
		EE_WL_FUNCTION( wl_event_queue_destroy )
		EE_WL_FUNCTION( wl_display_dispatch_queue_pending )
		EE_WL_FUNCTION( wl_display_prepare_read_queue )
		EE_WL_FUNCTION( wl_display_read_events )
		EE_WL_FUNCTION( wl_display_cancel_read )
		EE_WL_FUNCTION( wl_display_get_fd )
#undef EE_WL_FUNCTION
#define EE_WL_INTERFACE( name )                                                           \
	name = static_cast<const wl_interface*>( dlsym( handle, "wl_" #name "_interface" ) ); \
	if ( !name ) {                                                                        \
		dlclose( handle );                                                                \
		handle = nullptr;                                                                 \
		return false;                                                                     \
	}
		EE_WL_INTERFACE( registry )
		EE_WL_INTERFACE( callback )
		EE_WL_INTERFACE( compositor )
		EE_WL_INTERFACE( subcompositor )
		EE_WL_INTERFACE( subsurface )
		EE_WL_INTERFACE( region )
		EE_WL_INTERFACE( surface )
		EE_WL_INTERFACE( shm )
		EE_WL_INTERFACE( shm_pool )
		EE_WL_INTERFACE( buffer )
		EE_WL_INTERFACE( seat )
		EE_WL_INTERFACE( pointer )
		EE_WL_INTERFACE( data_device_manager )
		EE_WL_INTERFACE( data_device )
		EE_WL_INTERFACE( data_source )
#undef EE_WL_INTERFACE
		return true;
	}

	template <typename T> uint32_t version( T* object ) const {
		return wl_proxy_get_version( reinterpret_cast<wl_proxy*>( object ) );
	}

	template <typename T, typename... A>
	T* create( void* object, uint32_t opcode, const wl_interface* interface, A... args ) const {
		auto* proxy = static_cast<wl_proxy*>( object );
		return reinterpret_cast<T*>( wl_proxy_marshal_flags(
			proxy, opcode, interface,
			std::min( version( proxy ), static_cast<uint32_t>( interface->version ) ), 0, nullptr,
			args... ) );
	}

	template <typename T>
	T* bind( wl_registry* registry, uint32_t name, const wl_interface* interface,
			 uint32_t version ) const {
		return reinterpret_cast<T*>( wl_proxy_marshal_flags(
			reinterpret_cast<wl_proxy*>( registry ), WL_REGISTRY_BIND, interface, version, 0, name,
			interface->name, version, nullptr ) );
	}

	template <typename T, typename... A>
	void request( T* object, uint32_t opcode, A... args ) const {
		wl_proxy_marshal_flags( reinterpret_cast<wl_proxy*>( object ), opcode, nullptr,
								version( object ), 0, args... );
	}

	template <typename T> void destroy( T*& object, uint32_t opcode ) const {
		if ( object ) {
			wl_proxy_marshal_flags( reinterpret_cast<wl_proxy*>( object ), opcode, nullptr,
									version( object ), WL_MARSHAL_FLAG_DESTROY );
			object = nullptr;
		}
	}

	template <typename T> void releaseProxy( T*& object ) const {
		if ( object ) {
			wl_proxy_destroy( reinterpret_cast<wl_proxy*>( object ) );
			object = nullptr;
		}
	}

	template <typename T, typename L>
	bool listen( T* object, const L& listener, void* data ) const {
		return wl_proxy_add_listener(
				   reinterpret_cast<wl_proxy*>( object ),
				   reinterpret_cast<void ( ** )( void )>( const_cast<L*>( &listener ) ),
				   data ) == 0;
	}

	decltype( &::wl_proxy_marshal_flags ) wl_proxy_marshal_flags{ nullptr };
	decltype( &::wl_proxy_add_listener ) wl_proxy_add_listener{ nullptr };
	decltype( &::wl_proxy_get_version ) wl_proxy_get_version{ nullptr };
	decltype( &::wl_proxy_destroy ) wl_proxy_destroy{ nullptr };
	decltype( &::wl_proxy_set_queue ) wl_proxy_set_queue{ nullptr };
	decltype( &::wl_proxy_create_wrapper ) wl_proxy_create_wrapper{ nullptr };
	decltype( &::wl_proxy_wrapper_destroy ) wl_proxy_wrapper_destroy{ nullptr };
	decltype( &::wl_display_roundtrip ) wl_display_roundtrip{ nullptr };
	decltype( &::wl_display_flush ) wl_display_flush{ nullptr };
	decltype( &::wl_display_get_error ) wl_display_get_error{ nullptr };
	decltype( &::wl_display_create_queue ) wl_display_create_queue{ nullptr };
	decltype( &::wl_event_queue_destroy ) wl_event_queue_destroy{ nullptr };
	decltype( &::wl_display_dispatch_queue_pending ) wl_display_dispatch_queue_pending{ nullptr };
	decltype( &::wl_display_prepare_read_queue ) wl_display_prepare_read_queue{ nullptr };
	decltype( &::wl_display_read_events ) wl_display_read_events{ nullptr };
	decltype( &::wl_display_cancel_read ) wl_display_cancel_read{ nullptr };
	decltype( &::wl_display_get_fd ) wl_display_get_fd{ nullptr };
	const wl_interface *registry{}, *callback{}, *compositor{}, *subcompositor{}, *subsurface{},
		*region{}, *surface{}, *shm{}, *shm_pool{}, *buffer{}, *seat{}, *pointer{},
		*data_device_manager{}, *data_device{}, *data_source{};

  private:
	void* handle{ nullptr };
};

}}} // namespace EE::Window::Private
#endif
#endif
#endif
