#include <eepp/graphics/globalbatchrenderer.hpp>

namespace EE { namespace Graphics {

SINGLETON_DECLARE_IMPLEMENTATION( GlobalBatchRenderer )

GlobalBatchRenderer::GlobalBatchRenderer() {
	allocVertices( 4096 );
}

GlobalBatchRenderer::~GlobalBatchRenderer() {}

}} // namespace EE::Graphics
