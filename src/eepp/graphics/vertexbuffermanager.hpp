#ifndef EE_GRAPHICSCVERTEXBUFFERMANAGER_HPP
#define EE_GRAPHICSCVERTEXBUFFERMANAGER_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/vertexbuffer.hpp>

namespace EE { namespace Graphics { namespace Private {

class EE_API VertexBufferManager : public Container<VertexBuffer> {
	SINGLETON_DECLARE_HEADERS(VertexBufferManager)

	public:
		virtual ~VertexBufferManager();

		void reload();
	protected:
		VertexBufferManager();
};

}}}

#endif
