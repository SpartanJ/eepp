#ifndef EE_GRAPHICSCFRAMEBUFFERREGISTRY_HPP
#define EE_GRAPHICSCFRAMEBUFFERREGISTRY_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/framebuffer.hpp>

#include <eepp/system/container.hpp>
#include <eepp/system/singleton.hpp>
using namespace EE::System;

namespace EE { namespace Graphics { namespace Private {

/** Non-owning registry of framebuffers visible to the active graphics context. */
class EE_API FrameBufferRegistry : public Container<FrameBuffer> {
	SINGLETON_DECLARE_HEADERS( FrameBufferRegistry )

  public:
	virtual ~FrameBufferRegistry();

	FrameBuffer* getCurrentlyBound();

	FrameBuffer* getFromName( const std::string& name );

	FrameBuffer* getFromId( const String::HashType& id );

  protected:
	FrameBufferRegistry();
};

}}} // namespace EE::Graphics::Private

#endif
