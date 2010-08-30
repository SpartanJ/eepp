#ifndef EE_GAMINGCGLOBALBATCHRENDERER_H
#define EE_GAMINGCGLOBALBATCHRENDERER_H

#include "base.hpp"
#include "cbatchrenderer.hpp"

namespace EE { namespace Graphics {

/** @brief The global Batch Renderer class. This class will be used by the engine for the rendering. */
class EE_API cGlobalBatchRenderer : public tSingleton<cGlobalBatchRenderer>, public cBatchRenderer {
	friend class tSingleton<cGlobalBatchRenderer>;
	public:
    	cGlobalBatchRenderer();
    	~cGlobalBatchRenderer();
};

}}

#endif
