#ifndef EE_GAMINGCGLOBALBATCHRENDERER_H
#define EE_GAMINGCGLOBALBATCHRENDERER_H

#include "base.hpp"
#include "cbatchrenderer.hpp"

namespace EE { namespace Graphics {

/** @brief The global Batch Renderer class. This class will be used by the engine for the rendering. */
class cGlobalBatchRenderer : public cSingleton<cGlobalBatchRenderer>, public cBatchRenderer {
	friend class cSingleton<cGlobalBatchRenderer>;
	public:
    	cGlobalBatchRenderer();
    	~cGlobalBatchRenderer();
};

}}

#endif
