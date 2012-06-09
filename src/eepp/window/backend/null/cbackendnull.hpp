#ifndef EE_WINDOWCBACKENDNULL_HPP
#define EE_WINDOWCBACKENDNULL_HPP

#include <eepp/window/cbackend.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

class EE_API cBackendNull : public cBackend {
	public:
		inline cBackendNull() : cBackend()
		{
		}

		inline ~cBackendNull()
		{
		}
};

}}}}

#endif
