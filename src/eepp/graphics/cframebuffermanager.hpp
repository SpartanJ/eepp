#ifndef EE_GRAPHICSCFRAMEBUFFERMANAGER_HPP
#define EE_GRAPHICSCFRAMEBUFFERMANAGER_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/cframebuffer.hpp>

namespace EE { namespace Graphics { namespace Private {

class EE_API cFrameBufferManager : public Container<cFrameBuffer> {
	SINGLETON_DECLARE_HEADERS(cFrameBufferManager)

	public:
		virtual ~cFrameBufferManager();

		void Reload();
	protected:
		cFrameBufferManager();
};

}}}

#endif
