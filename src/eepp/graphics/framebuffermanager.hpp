#ifndef EE_GRAPHICSCFRAMEBUFFERMANAGER_HPP
#define EE_GRAPHICSCFRAMEBUFFERMANAGER_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/framebuffer.hpp>

namespace EE { namespace Graphics { namespace Private {

class EE_API FrameBufferManager : public Container<FrameBuffer> {
	SINGLETON_DECLARE_HEADERS(FrameBufferManager)

	public:
		virtual ~FrameBufferManager();

		void reload();
	protected:
		FrameBufferManager();
};

}}}

#endif
