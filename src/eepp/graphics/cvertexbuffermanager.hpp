#ifndef EE_GRAPHICSCVERTEXBUFFERMANAGER_HPP
#define EE_GRAPHICSCVERTEXBUFFERMANAGER_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/cvertexbuffer.hpp>

namespace EE { namespace Graphics { namespace Private {

class EE_API cVertexBufferManager : public Container<cVertexBuffer> {
	SINGLETON_DECLARE_HEADERS(cVertexBufferManager)

	public:
		virtual ~cVertexBufferManager();

		void Reload();
	protected:
		cVertexBufferManager();
};

}}}

#endif
