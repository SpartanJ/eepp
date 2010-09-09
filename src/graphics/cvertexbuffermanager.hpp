#ifndef EE_GRAPHICSCVERTEXBUFFERMANAGER_HPP
#define EE_GRAPHICSCVERTEXBUFFERMANAGER_HPP

#include "base.hpp"
#include "cvertexbuffer.hpp"

namespace EE { namespace Graphics { namespace Private {

class EE_API cVertexBufferManager : public tContainer<cVertexBuffer>, public tSingleton<cVertexBufferManager> {
	friend class tSingleton<cVertexBufferManager>;
	public:
		cVertexBufferManager();

		virtual ~cVertexBufferManager();

		void Reload();
	protected:
};

}}}

#endif
