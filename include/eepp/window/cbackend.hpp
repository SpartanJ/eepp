#ifndef EE_WINDOWCBACKEND_HPP
#define EE_WINDOWCBACKEND_HPP

#include <eepp/base.hpp>

namespace EE { namespace Window { namespace Backend {

class EE_API cBackend {
	public:
		inline cBackend() {};
		
		inline virtual ~cBackend() {};
};

}}}

#endif
