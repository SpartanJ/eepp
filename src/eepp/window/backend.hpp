#ifndef EE_WINDOWCBACKEND_HPP
#define EE_WINDOWCBACKEND_HPP

#include <eepp/core.hpp>

namespace EE { namespace Window { namespace Backend {

class EE_API WindowBackendLibrary {
  public:
	inline WindowBackendLibrary() {}

	inline virtual ~WindowBackendLibrary() {}
};

}}} // namespace EE::Window::Backend

#endif
