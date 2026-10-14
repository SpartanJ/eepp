#ifndef EE_WINDOW_X11_DRAG_LIBRARY_HPP
#define EE_WINDOW_X11_DRAG_LIBRARY_HPP

#include <dlfcn.h>

namespace EE { namespace Window { namespace Private {

/** Owns only a loader reference, never SDL's display. Each native session keeps its functions
 * available until its resources have been released. No library is loaded at process startup. */
class X11Library {
  public:
	X11Library() = default;

	X11Library( const X11Library& ) = delete;

	X11Library& operator=( const X11Library& ) = delete;

	~X11Library() {
		if ( handle )
			dlclose( handle );
	}

	bool load() {
		if ( !handle )
			handle = dlopen( "libX11.so.6", RTLD_NOW | RTLD_LOCAL );
		if ( !handle )
			handle = dlopen( "libX11.so", RTLD_NOW | RTLD_LOCAL );
		return handle != nullptr;
	}

	template <typename Function> Function loadFunction( const char* name ) const {
		return reinterpret_cast<Function>( dlsym( handle, name ) );
	}

  private:
	void* handle{ nullptr };
};

}}} // namespace EE::Window::Private

#endif
