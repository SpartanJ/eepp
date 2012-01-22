#ifndef EE_GRAPHICSCFRAMEBUFFERMANAGER_HPP
#define EE_GRAPHICSCFRAMEBUFFERMANAGER_HPP

#include "base.hpp"
#include "cframebuffer.hpp"

namespace EE { namespace Graphics { namespace Private {

class EE_API cFrameBufferManager : public tContainer<cFrameBuffer>, public tSingleton<cFrameBufferManager> {
	public:
		SINGLETON_DECLARE_HEADERS(cFrameBufferManager)

		cFrameBufferManager();

		virtual ~cFrameBufferManager();

		void Reload();
	protected:
};

}}}

#endif
