#ifndef EE_WINDOWCBACKENDNULL_HPP
#define EE_WINDOWCBACKENDNULL_HPP

#include <eepp/window/backend.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

class EE_API WindowBackendNull : public WindowBackend {
	public:
		inline WindowBackendNull() : WindowBackend()
		{
		}

		inline ~WindowBackendNull()
		{
		}
};

}}}}

#endif
