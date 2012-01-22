#ifndef EE_GRAPHICSCFRAMEBUFFERMANAGER_HPP
#define EE_GRAPHICSCFRAMEBUFFERMANAGER_HPP

#include "base.hpp"
#include "cframebuffer.hpp"

namespace EE { namespace Graphics { namespace Private {

class EE_API cFrameBufferManager : public tContainer<cFrameBuffer> {
	SINGLETON_DECLARE_HEADERS(cFrameBufferManager)

	public:
		cFrameBufferManager();

		virtual ~cFrameBufferManager();

		void Reload();
	protected:
};

}}}

#endif
