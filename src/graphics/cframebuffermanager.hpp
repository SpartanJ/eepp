#ifndef EE_GRAPHICSCFRAMEBUFFERMANAGER_HPP
#define EE_GRAPHICSCFRAMEBUFFERMANAGER_HPP

#include "base.hpp"
#include "cframebuffer.hpp"

namespace EE { namespace Graphics { namespace Private {

class EE_API cFrameBufferManager : public tContainer<cFrameBuffer>, public tSingleton<cFrameBufferManager> {
	friend class tSingleton<cFrameBufferManager>;
	public:
		cFrameBufferManager();

		virtual ~cFrameBufferManager();

		void Reload();
	protected:
};

}}}

#endif
