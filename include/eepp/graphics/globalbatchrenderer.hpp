#ifndef EE_GAMINGCGLOBALBATCHRENDERER_H
#define EE_GAMINGCGLOBALBATCHRENDERER_H

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/batchrenderer.hpp>

namespace EE { namespace Graphics {

/** @brief The global Batch Renderer class. This class will be used by the engine for the rendering. */
class EE_API GlobalBatchRenderer : public BatchRenderer {
	SINGLETON_DECLARE_HEADERS(GlobalBatchRenderer)

	public:
		~GlobalBatchRenderer();
	protected:
		GlobalBatchRenderer();
};

}}

#endif
