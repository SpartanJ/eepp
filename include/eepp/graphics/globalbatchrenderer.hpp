#ifndef EE_MAPS_CGLOBALBATCHRENDERER_H
#define EE_MAPS_CGLOBALBATCHRENDERER_H

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/batchrenderer.hpp>

#include <eepp/system/singleton.hpp>
using namespace EE::System;

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
