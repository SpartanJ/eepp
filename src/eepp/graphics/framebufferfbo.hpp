#ifndef EE_GRAPHICSCFRAMEBUFFERFBO_HPP
#define EE_GRAPHICSCFRAMEBUFFERFBO_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/framebuffer.hpp>
#include <eepp/graphics/texture.hpp>

namespace EE { namespace Graphics {

class EE_API FrameBufferFBO : public FrameBuffer {
	public:
		FrameBufferFBO( EE::Window::Window * window = NULL );

		~FrameBufferFBO();

		FrameBufferFBO( const Uint32& Width, const Uint32& Height, bool DepthBuffer = false, EE::Window::Window * window = NULL );

		void bind();

		void unbind();

		void reload();

		static bool isSupported();
	protected:
		Int32 		mFrameBuffer;
		Uint32 		mDepthBuffer;
		Int32		mLastFB;
		Int32		mLastRB;

		bool create( const Uint32& Width, const Uint32& Height );

		bool create( const Uint32& Width, const Uint32& Height, bool DepthBuffer );

		void bindFrameBuffer();

		void bindRenderBuffer();
};

}}

#endif

