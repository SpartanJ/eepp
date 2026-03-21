#ifndef EE_WINDOWCBACKENDSDL3_HPP
#define EE_WINDOWCBACKENDSDL3_HPP

#include <eepp/window/backend.hpp>
#include <eepp/window/backend/SDL3/base.hpp>

#ifdef EE_BACKEND_SDL3

#include <eepp/window/backend/SDL3/displaymanagersdl3.hpp>
#include <eepp/window/backend/SDL3/windowsdl3.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL3 {

class EE_API WindowBackendSDL3 : public WindowBackendLibrary {
  public:
	WindowBackendSDL3();

	~WindowBackendSDL3();
};

}}}} // namespace EE::Window::Backend::SDL3

#endif

#endif
