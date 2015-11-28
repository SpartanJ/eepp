#ifndef EE_WINDOWCBACKEND_HPP
#define EE_WINDOWCBACKEND_HPP

#include <eepp/core.hpp>

namespace EE { namespace Window { namespace Backend {

class EE_API WindowBackend {
	public:
		inline WindowBackend() {}
		
		inline virtual ~WindowBackend() {}
};

}}}

#endif
